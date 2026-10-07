# Token Widget

아이폰 잠금화면에 Anthropic API 토큰 사용량과 리셋까지 남은 시간을 보여주는 위젯.

## 빌드 (Xcode가 있는 맥에서)
1. `brew install xcodegen`
2. `project.yml`의 `DEVELOPMENT_TEAM`, 번들 ID/App Group(`com.example...`)을 본인 것으로 수정
3. `xcodegen && open TokenWidget.xcodeproj` → 실기기로 Run
4. 앱에서 Admin API 키 입력 → 잠금화면 길게 눌러 "맞춤화" → 위젯 추가

## 알아둘 점
- **Admin API 키(`sk-ant-admin...`)** 필요 — 조직(Organization) 계정만 발급 가능. 일반 API 키로는 사용량 조회 불가.
- Anthropic API에는 5시간 세션 같은 한도 리셋이 없어, 리셋은 **UTC 기준 일/월 경계**로 계산. 한도는 앱에서 직접 입력.
- Claude.ai / Claude Code **구독** 사용량은 공개 API가 없어 이 위젯에 포함되지 않음.
- 위젯 갱신은 iOS가 조절(대략 15분~수 시간). 남은 시간 카운트다운은 갱신 없이 실시간으로 흐름.
- 이 코드는 리눅스 환경에서 작성되어 **컴파일 검증을 못 했음**. 빌드 에러가 나면 알려주세요.
