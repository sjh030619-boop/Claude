import SwiftUI
import WidgetKit

struct UsageEntry: TimelineEntry {
    let date: Date
    let snapshot: UsageSnapshot?
    let budget: Int
    let error: String?

    /// 한도가 있으면 사용률, 없으면 리셋까지 경과 비율
    var gauge: Double {
        guard let s = snapshot else { return 0 }
        if budget > 0 { return min(Double(s.total) / Double(budget), 1) }
        let span = s.resetAt.timeIntervalSince(s.periodStart)
        return min(max(date.timeIntervalSince(s.periodStart) / span, 0), 1)
    }
}

struct Provider: TimelineProvider {
    func placeholder(in context: Context) -> UsageEntry {
        let (s, r) = UsageClient.window(for: .month)
        return UsageEntry(date: .now,
                          snapshot: UsageSnapshot(inputTokens: 1_200_000, outputTokens: 300_000,
                                                  periodStart: s, resetAt: r, fetchedAt: .now),
                          budget: 5_000_000, error: nil)
    }

    func getSnapshot(in context: Context, completion: @escaping (UsageEntry) -> Void) {
        completion(placeholder(in: context))
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<UsageEntry>) -> Void) {
        Task {
            let budget = Config.tokenBudget
            let entry: UsageEntry
            do {
                let snap = try await UsageClient.fetch(period: Config.period)
                entry = UsageEntry(date: .now, snapshot: snap, budget: budget, error: nil)
            } catch {
                entry = UsageEntry(date: .now, snapshot: nil, budget: budget,
                                   error: (error as? LocalizedError)?.errorDescription ?? "오류")
            }
            // iOS가 위젯 갱신 횟수를 제한하므로 15분 간격으로 요청 (보장은 아님)
            completion(Timeline(entries: [entry], policy: .after(.now.addingTimeInterval(15 * 60))))
        }
    }
}

struct TokenWidgetView: View {
    @Environment(\.widgetFamily) private var family
    let entry: UsageEntry

    var body: some View {
        Group {
            switch family {
            case .accessoryCircular: circular
            case .accessoryInline: inline
            case .accessoryRectangular: rectangular
            default: home
            }
        }
        .containerBackground(.fill.tertiary, for: .widget)
    }

    private var circular: some View {
        Gauge(value: entry.gauge) {
            Image(systemName: "bolt.fill")
        } currentValueLabel: {
            Text(entry.snapshot?.total.compact ?? "–").font(.system(size: 14, weight: .semibold))
        }
        .gaugeStyle(.accessoryCircularCapacity)
    }

    private var inline: some View {
        if let s = entry.snapshot {
            Text("\(Image(systemName: "bolt.fill")) \(s.total.compact) · \(s.resetAt, style: .relative)")
        } else {
            Text("\(Image(systemName: "bolt.fill")) \(entry.error ?? "–")")
        }
    }

    private var rectangular: some View {
        VStack(alignment: .leading, spacing: 2) {
            if let s = entry.snapshot {
                Text(entry.budget > 0 ? "\(s.total.compact) / \(entry.budget.compact) 토큰"
                                      : "\(s.total.compact) 토큰")
                    .font(.headline)
                Text("입력 \(s.inputTokens.compact) · 출력 \(s.outputTokens.compact)")
                    .font(.caption2)
                // Text(_, style: .relative)는 위젯 갱신 없이 시스템이 알아서 카운트
                Text("리셋까지 \(Text(s.resetAt, style: .relative))").font(.caption2)
                ProgressView(value: entry.gauge)
            } else {
                Text(entry.error ?? "데이터 없음").font(.caption)
            }
        }
    }

    private var home: some View {
        VStack(alignment: .leading, spacing: 4) {
            Label("토큰 사용량", systemImage: "bolt.fill").font(.caption)
            if let s = entry.snapshot {
                Text(s.total.compact).font(.largeTitle.bold())
                ProgressView(value: entry.gauge)
                Text("리셋 \(s.resetAt, style: .relative) 후").font(.caption2)
            } else {
                Text(entry.error ?? "데이터 없음").font(.caption)
            }
        }
    }
}

@main
struct TokenWidget: Widget {
    var body: some WidgetConfiguration {
        StaticConfiguration(kind: "TokenWidget", provider: Provider()) { entry in
            TokenWidgetView(entry: entry)
        }
        .configurationDisplayName("토큰 사용량")
        .description("Anthropic API 토큰 사용량과 리셋까지 남은 시간")
        .supportedFamilies([.accessoryCircular, .accessoryRectangular, .accessoryInline, .systemSmall])
    }
}
