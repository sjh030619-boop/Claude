import Foundation

struct UsageSnapshot {
    var inputTokens: Int
    var outputTokens: Int
    var periodStart: Date
    var resetAt: Date
    var fetchedAt: Date

    var total: Int { inputTokens + outputTokens }
}

enum UsageError: LocalizedError {
    case noKey, http(Int), decode

    var errorDescription: String? {
        switch self {
        case .noKey: return "Admin 키 없음"
        case .http(let c): return c == 401 || c == 403 ? "키 권한 오류" : "서버 오류 \(c)"
        case .decode: return "응답 해석 실패"
        }
    }
}

/// Anthropic Admin API `usage_report/messages` 호출.
/// 주의: Admin API 키(sk-ant-admin...)가 필요하며 조직(Organization) 계정에서만 발급됩니다.
enum UsageClient {
    private struct Page: Decodable {
        struct Bucket: Decodable {
            struct Result: Decodable {
                struct CacheCreation: Decodable {
                    let ephemeral_1h_input_tokens: Int?
                    let ephemeral_5m_input_tokens: Int?
                }
                let uncached_input_tokens: Int?
                let cache_read_input_tokens: Int?
                let cache_creation: CacheCreation?
                let output_tokens: Int?
            }
            let results: [Result]
        }
        let data: [Bucket]
        let has_more: Bool
        let next_page: String?
    }

    private static var utc: Calendar = {
        var c = Calendar(identifier: .gregorian)
        c.timeZone = TimeZone(identifier: "UTC")!
        return c
    }()

    static func window(for period: UsagePeriod, now: Date = .now) -> (start: Date, reset: Date) {
        switch period {
        case .day:
            let start = utc.startOfDay(for: now)
            return (start, utc.date(byAdding: .day, value: 1, to: start)!)
        case .month:
            let start = utc.date(from: utc.dateComponents([.year, .month], from: now))!
            return (start, utc.date(byAdding: .month, value: 1, to: start)!)
        }
    }

    static func fetch(period: UsagePeriod, now: Date = .now) async throws -> UsageSnapshot {
        guard let key = Config.adminKey else { throw UsageError.noKey }
        let (start, reset) = window(for: period, now: now)
        let iso = ISO8601DateFormatter()

        var input = 0, output = 0
        var page: String?
        repeat {
            var comps = URLComponents(string: "https://api.anthropic.com/v1/organizations/usage_report/messages")!
            var items = [
                URLQueryItem(name: "starting_at", value: iso.string(from: start)),
                URLQueryItem(name: "ending_at", value: iso.string(from: now)),
                URLQueryItem(name: "bucket_width", value: period == .day ? "1h" : "1d"),
                URLQueryItem(name: "limit", value: period == .day ? "24" : "31"),
            ]
            if let page { items.append(URLQueryItem(name: "page", value: page)) }
            comps.queryItems = items

            var req = URLRequest(url: comps.url!)
            req.setValue(key, forHTTPHeaderField: "x-api-key")
            req.setValue("2023-06-01", forHTTPHeaderField: "anthropic-version")

            let (data, resp) = try await URLSession.shared.data(for: req)
            if let code = (resp as? HTTPURLResponse)?.statusCode, code != 200 { throw UsageError.http(code) }
            guard let decoded = try? JSONDecoder().decode(Page.self, from: data) else { throw UsageError.decode }

            for r in decoded.data.flatMap(\.results) {
                input += (r.uncached_input_tokens ?? 0)
                    + (r.cache_read_input_tokens ?? 0)
                    + (r.cache_creation?.ephemeral_1h_input_tokens ?? 0)
                    + (r.cache_creation?.ephemeral_5m_input_tokens ?? 0)
                output += r.output_tokens ?? 0
            }
            page = decoded.has_more ? decoded.next_page : nil
        } while page != nil

        return UsageSnapshot(inputTokens: input, outputTokens: output,
                             periodStart: start, resetAt: reset, fetchedAt: now)
    }
}

extension Int {
    /// 1_234_567 → "1.2M"
    var compact: String {
        switch self {
        case 1_000_000...: return String(format: "%.1fM", Double(self) / 1_000_000)
        case 1_000...: return String(format: "%.1fK", Double(self) / 1_000)
        default: return "\(self)"
        }
    }
}
