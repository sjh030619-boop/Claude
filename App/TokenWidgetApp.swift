import SwiftUI
import WidgetKit

@main
struct TokenWidgetApp: App {
    var body: some Scene {
        WindowGroup { SettingsView() }
    }
}

struct SettingsView: View {
    @State private var key = ""
    @State private var budget = ""
    @State private var period = Config.period
    @State private var status = ""

    var body: some View {
        NavigationStack {
            Form {
                Section("Anthropic Admin API 키") {
                    SecureField("sk-ant-admin...", text: $key)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                }
                Section("집계 기간") {
                    Picker("기간", selection: $period) {
                        ForEach(UsagePeriod.allCases) { Text($0.label).tag($0) }
                    }.pickerStyle(.segmented)
                }
                Section("토큰 한도 (선택)") {
                    TextField("예: 5000000 (비우면 시간 기준 게이지)", text: $budget)
                        .keyboardType(.numberPad)
                }
                Section {
                    Button("저장하고 테스트", action: save)
                    if !status.isEmpty { Text(status).font(.footnote) }
                }
            }
            .navigationTitle("토큰 위젯")
            .onAppear {
                key = Config.adminKey ?? ""
                budget = Config.tokenBudget > 0 ? String(Config.tokenBudget) : ""
            }
        }
    }

    private func save() {
        Config.adminKey = key
        Config.period = period
        Config.tokenBudget = Int(budget) ?? 0
        status = "확인 중…"
        Task {
            do {
                let s = try await UsageClient.fetch(period: period)
                status = "성공: \(s.total.compact) 토큰 사용"
                WidgetCenter.shared.reloadAllTimelines()
            } catch {
                status = "실패: \((error as? LocalizedError)?.errorDescription ?? error.localizedDescription)"
            }
        }
    }
}
