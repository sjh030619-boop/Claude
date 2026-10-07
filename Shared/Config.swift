import Foundation
import Security

enum UsagePeriod: String, CaseIterable, Identifiable {
    case day, month
    var id: String { rawValue }
    var label: String { self == .day ? "오늘 (UTC)" : "이번 달 (UTC)" }
}

/// 앱과 위젯이 공유하는 설정. 일반 설정은 App Group, Admin 키는 공유 Keychain에 저장.
enum Config {
    static let appGroup = "group.com.example.tokenwidget"
    private static let defaults = UserDefaults(suiteName: appGroup) ?? .standard

    static var period: UsagePeriod {
        get { UsagePeriod(rawValue: defaults.string(forKey: "period") ?? "") ?? .month }
        set { defaults.set(newValue.rawValue, forKey: "period") }
    }

    /// 0이면 한도 미설정 (게이지는 리셋까지 경과 시간 표시)
    static var tokenBudget: Int {
        get { defaults.integer(forKey: "tokenBudget") }
        set { defaults.set(newValue, forKey: "tokenBudget") }
    }

    // MARK: Keychain (Admin API key)

    private static var group: String? {
        Bundle.main.object(forInfoDictionaryKey: "KeychainGroup") as? String
    }

    private static func query() -> [String: Any] {
        var q: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: "anthropic-admin-key",
        ]
        if let group { q[kSecAttrAccessGroup as String] = group }
        return q
    }

    static var adminKey: String? {
        get {
            var q = query()
            q[kSecReturnData as String] = true
            q[kSecMatchLimit as String] = kSecMatchLimitOne
            var out: AnyObject?
            guard SecItemCopyMatching(q as CFDictionary, &out) == errSecSuccess,
                  let data = out as? Data else { return nil }
            return String(data: data, encoding: .utf8)
        }
        set {
            SecItemDelete(query() as CFDictionary)
            guard let newValue, !newValue.isEmpty else { return }
            var q = query()
            q[kSecValueData as String] = Data(newValue.utf8)
            q[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlock
            SecItemAdd(q as CFDictionary, nil)
        }
    }
}
