# Windows PC 를 GitHub 빌드 서버(self-hosted runner)로 쓰기

목표: 노트나 코드를 GitHub 에 푸시하면, **켜 둔 Windows PC 가 알아서 `Hangul.g1a` 를 빌드**하고
GitHub 의 Actions 페이지에서 내려받을 수 있게 한다. (GitHub 의 기본 빌드 서버는 fxSDK 서버에 접속이
안 돼서 쓸 수 없다.)

워크플로: `.github/workflows/build-g1a-local.yml` (이미 들어 있음).

## 1. 한 번만 하는 설정

1. 브라우저에서 저장소 `sjh030619-boop/Claude` → **Settings → Actions → Runners → New self-hosted runner**
   → 운영체제 **Windows**, 아키텍처 **x64** 선택.
2. 그 페이지의 **Download** 와 **Configure** 에 나온 명령을 PowerShell 에 순서대로 복사해서 실행한다
   (버전 번호와 토큰이 페이지마다 달라서 그대로 복사해야 한다. 토큰은 1시간짜리).
   대략 이런 모양이다:
   ```
   mkdir C:\actions-runner ; cd C:\actions-runner
   Invoke-WebRequest -Uri <페이지에 나온 주소> -OutFile actions-runner-win-x64.zip
   Add-Type -AssemblyName System.IO.Compression.FileSystem
   [System.IO.Compression.ZipFile]::ExtractToDirectory("$PWD/actions-runner-win-x64.zip", "$PWD")
   .\config.cmd --url https://github.com/sjh030619-boop/Claude --token <페이지에 나온 토큰>
   ```
3. `config.cmd` 가 묻는 것에는 **전부 Enter(기본값)** 로 답한다. 단, **"run as service?" 에는 N**.
   (서비스로 돌리면 다른 계정으로 실행돼서, 내 계정에 설치된 WSL Ubuntu 를 못 찾는다.)
4. 실행: `.\run.cmd`. 창에 `Listening for Jobs` 가 보이면 준비 끝. **이 창을 닫지 말고 켜 둔다.**

## 2. 평소에 할 일

- PC 를 켜 두고, **로그인한 상태**로 `run.cmd` 창만 열어 둔다(잠금 화면은 괜찮다).
- 절전을 끈다: 설정 → 시스템 → 전원 → 화면 및 절전 → 절전 "안 함".
- 재부팅한 뒤에는 `C:\actions-runner` 에서 `.\run.cmd` 를 다시 실행한다.
  (자동으로 하고 싶으면 작업 스케줄러에 "로그온할 때 `C:\actions-runner\run.cmd` 실행" 을 등록.)

## 3. 빌드하고 받기

1. 노트/코드를 푸시한다(이 저장소의 `fx9860g3-hangul/` 아래가 바뀌면 자동으로 빌드가 시작된다).
2. GitHub → **Actions** 탭 → "Build Hangul.g1a (self-hosted Windows PC)" 실행 → 초록 체크가 뜨면
   맨 아래 **Artifacts → Hangul-g1a** 를 내려받는다(zip 안에 `Hangul.g1a`).
3. 압축을 풀고 계산기(Mass Storage 모드)에 복사한다.

첫 빌드는 도구 확인 때문에 조금 더 걸리고, 이후에는 몇 분이면 끝난다.
빌드가 실패하면 Actions 의 해당 실행 → 실패한 단계의 로그를 열어 마지막 30줄을 알려 주면 된다.

## 4. 주의

- self-hosted runner 는 **저장소의 코드를 내 PC 에서 그대로 실행**한다. 저장소를 **비공개**로 유지하고,
  남이 올린 코드(포크한 PR 등)를 이 runner 로 돌리지 마라.
- runner 를 쓰지 않을 때는 `run.cmd` 창을 닫으면 된다. 완전히 지우려면
  Settings → Actions → Runners 에서 runner 를 삭제하고 `config.cmd remove` 를 실행한다.
- 이 방식은 Windows 계정의 WSL Ubuntu(기본 사용자 root)에 이미 툴체인이 깔려 있다는 전제다.
  (`tools\windows\build_windows.ps1` 로 한 번 빌드해 본 PC 이면 충족.)
