# 인수인계: fx-9860G III 한글 노트 뷰어 (새 컴퓨터 / 새 Claude 계정에서 이어가기)

이 문서 하나로 지금까지의 작업을 이어갈 수 있게 정리했다. 먼저 `fx9860g3-hangul/CLAUDE.md`(구조 요약)와 이 문서를 읽고,
세부는 `README.md`(기능 설명), `docs/PROJECT_PROMPT.md`(노트 작성용 프롬프트), `docs/SELF_HOSTED_RUNNER.md`(러너 설치)를 본다.

## 0. 한눈에

- 목적: CASIO fx-9860G III(128×64 흑백 화면)에서 **한글 시험 노트**를 보는 add-in(`.g1a`). Galmuri 7px 한글 비트맵 폰트(SIL OFL)를 내장.
- 과목마다 앱 하나: `sets/<id>/` 하나가 `.g1a` 하나가 된다. 지금 세트: `mlmid`(기계공학을위한인공지능), `prodsys`(생산시스템공학; 옛 `prod`/HanProd 를 대체해서 삭제함).
- 노트는 별도의 "노트 만드는 Claude 프로젝트"(웹 Claude.ai 프로젝트)가 zip(`notes/`, `README.md`, `set.json`, `CHANGES.txt`)으로 만들어 주고, 이쪽(코딩 세션)이 그걸 `sets/<id>/`에 반영해서 빌드한다.
- 빌드는 사용자의 **Windows PC**(WSL2 Ubuntu + sh-elf-gcc + fxSDK + gint 2.11)에서 한다. GitHub 푸시 → **self-hosted runner**가 자동 빌드 → 아티팩트 `Hangul-g1a`(zip, `.g1a` 들).
- 코드 작업 세션은 클라우드 컨테이너(Claude Code on the web)이고, 사용자 PC 를 직접 제어할 수 없다. 빌드 결과는 GitHub 아티팩트를 받아 `SendUserFile` 로 사용자에게 보낸다.
- 작업 브랜치: **`claude/iphone-lockscreen-token-widget-n3jmi7`**(이름은 옛 작업의 흔적, 아직 `main`에 병합 안 함). 저장소에는 관련 없는 iPhone 위젯 프로젝트(`App/ Shared/ Widget/ project.yml`)도 같이 있다 — 건드리지 말 것.
- 저장소: `github.com/sjh030619-boop/Claude` (사용자 소유, 비공개로 유지). PR 은 사용자가 요청할 때만 만든다.

## 1. 새 환경에서 시작하기

### 1-1. 새 Claude 계정 / 새 세션
1. 새 계정의 Claude 에 같은 GitHub 계정(`sjh030619-boop`)을 연결하고 이 저장소에 접근 권한을 준다 (Claude Code on the web 에서 저장소 선택).
2. 세션 브랜치를 `claude/iphone-lockscreen-token-widget-n3jmi7` 로 맞추거나, 그 브랜치를 `main` 에 병합해서 `main` 에서 시작한다
   (`main` 병합은 사용자가 원할 때: "PR 만들어줘" → 병합).
3. 첫 메시지(그대로 붙여넣기):
   ```
   fx-9860G III 한글 노트 add-in 프로젝트를 이어서 할 거야. 저장소의 fx9860g3-hangul/docs/HANDOVER.md 와
   fx9860g3-hangul/CLAUDE.md 를 먼저 읽고 요약해줘. 한국어로 짧게 답하고, 계획을 먼저 말하라고 한 일은
   내가 확인하기 전에 구현하지 마. 빌드는 푸시하면 내 Windows PC 러너가 자동으로 해.
   ```
4. 클라우드 세션에서 GitHub 를 쓰려면 GitHub MCP(저장소 연결)가 필요하다. 아티팩트 내려받기는 `actions_get download_workflow_run_artifact` → 서명 URL → `curl` → `python3 -I` 로 unzip (§5 참고).

### 1-2. 새 Windows PC (빌드 서버로 쓸 때)
1. **WSL2 + Ubuntu**: PowerShell(관리자) `wsl --install -d Ubuntu`, 재부팅, Ubuntu 를 한 번 열어 사용자 설정(여기서는 기본 사용자 root 로 썼다).
2. 저장소를 PC 에 clone (경로에 공백/한글이 없는 편이 안전).
3. **툴체인 한 번 빌드**(30~60분 이상): 저장소 `fx9860g3-hangul` 폴더에서
   `powershell -ExecutionPolicy Bypass -File tools\windows\build_windows.ps1`
   WSL 안에 sh-elf-gcc, fxSDK, gint(GiteaPC 로)를 설치하고 `~/.hangul_toolchain_ok` 표시를 남긴다. 끝나면 `dist\*.g1a` 가 생긴다.
   - fxSDK 서버는 **`git.planet-casio.com`** (옛 `gitea.planet-casio.fr` 는 안 열림). GitHub 호스트 러너에서는 이 서버에 접속이 안 되어서 self-hosted 만 가능하다.
4. **self-hosted runner 등록**: `docs/SELF_HOSTED_RUNNER.md` 의 순서대로(저장소 Settings → Actions → Runners → New self-hosted runner, Windows x64, 토큰은 1시간짜리). `config.cmd` 에서 "run as service?" 는 **N**(서비스로 돌리면 다른 계정이 되어 WSL Ubuntu 를 못 찾는다). 실행은 `.\run.cmd`, `Listening for Jobs` 가 보이면 OK, 창을 닫지 않는다. 절전 끄기. 재부팅 후에는 `run.cmd` 를 다시 실행(원하면 작업 스케줄러 "로그온할 때 실행").
   - 이전 PC 의 러너 폴더는 `C:\Users\JH\actions-runner`. 새 PC 에서 새로 등록하면 이전 러너는 GitHub Settings 에서 삭제한다.
5. 워크플로 `.github/workflows/build-g1a-local.yml`: `push`(경로 `fx9860g3-hangul/**`) 또는 수동 실행. `runs-on: [self-hosted, Windows]`, `shell: cmd`(PowerShell 실행 정책 문제 때문), `.gitattributes` 로 sh/py/c/h/txt 를 LF 로 강제(CRLF 체크아웃이 bash 를 깨뜨렸다). `build-g1a.yml`(GitHub 호스트용)은 수동 전용이고 실제로는 성공 못 한다.
6. 러너가 꺼져 있으면 실행이 **queued** 로 계속 대기한다. 그때는 PC 에서 `run.cmd` 를 켜면 된다. 급하면 PC 에서 `build_windows.ps1` 을 직접 돌려도 `dist\` 에 같은 `.g1a` 가 생긴다.

### 1-3. 리눅스/클라우드에서의 개발 환경 (빌드 없이 테스트)
- SH 크로스 컴파일러가 없어서 **`.g1a` 는 못 만든다**. 대신 PC 시뮬레이터(`tools/sim/`, gint 스텁)로 화면 동작을 시험한다:
  ```
  python3 tools/build_assets.py --set mlmid
  gcc -Wall -Wextra -Isrc -Itools/sim -Dmain=app_main tools/sim/sim.c src/main.c src/hangul.c \
      src/font_data.c src/notes.c src/fakecalc.c src/fakecalc_data.c src/calc_font.c src/calc_eval.c src/calc_ui.c -o sim
  mkdir out && ./sim out "F2 0 2 PLUS 2 EXE"    # 키 문자열 → out/frame_*.pbm
  python3 tools/sim/sheet.py out sheet.png 3
  gcc -Isrc tools/calc_test.c src/calc_eval.c -o calc_test && ./calc_test     # 계산기 시험 47개
  python3 tools/check_notes.py                                                  # 노트 검사(오류 0 이어야 함)
  ```
- Python 3 + Pillow, gcc, pdftotext 가 있으면 충분하다.

## 2. 저장소 구조

```
fx9860g3-hangul/
  CLAUDE.md, README.md          구조 요약 / 기능 설명
  CMakeLists.txt                fxSDK 빌드(소스: main.c hangul.c fakecalc.c fakecalc_data.c calc_font.c calc_eval.c calc_ui.c font_data.c notes.c)
  src/main.c                    목록/본문/링크/색인/키 처리 (UI 상태 머신)
  src/hangul.c/.h               UTF-8, 한글 비트맵 글꼴, 줄바꿈, 첨자, 수식 줄, 표, 그림, 링크 파싱, 폭 계산
  src/calc_eval.c, calc_ui.c, calc_font.c, calc.h   진짜 미니 계산기 (파서 + 표시, 라이브러리 없이 sin/ln/exp 직접 구현)
  src/fakecalc*.c/.h            원래 가짜 Run-Mat 화면 비트맵(스크린샷) — 지금은 계산기 하단 기능 줄(JUMP DEL >MAT MATH)만 사용
  src/font_data.c, notes.c, addin.cmake, icon_gen.png   빌드가 만드는 생성물(gitignore, 손으로 고치지 말 것)
  sets/<id>/{set.json, icon.json, README.md, notes/<NN_폴더>/<NN_파일>.txt, notes/figures/*.png}
  tools/build_assets.py         세트 → 폰트/노트/그림/아이콘/CMake 설정 생성 (--set, --font, --only-used)
  tools/check_notes.py          노트 검사기(E 오류 / W 경고). --dir 단독 모드
  tools/make_check_bundle.py    docs/check_bundle.zip 생성 (노트 프로젝트에 올릴 검사기 + fig_tool.py)
  tools/fig_tool.py, make_icon.py, noteset.py, split_notes.py(선택), make_fakecalc.py, preview.py, host_test.c, calc_test.c
  tools/sim/                    PC 시뮬레이터
  tools/windows/                build_windows.ps1, build_in_wsl.sh (Windows 에서 모든 세트 빌드)
  docs/                         PROJECT_PROMPT.md(노트 작성용), PROMPT.md(옛 지침), SELF_HOSTED_RUNNER.md, check_bundle.zip, 이 문서
.github/workflows/              build-g1a-local.yml(실사용), build-g1a.yml(수동·불가)
```
`CLAUDE.md` 의 "Rules of thumb": 생성 파일은 손대지 않는다. 화면 128×64, 7px 한글(기본), 쪽 높이 합 ≤ 64px.

## 3. 기능 요약 (사용자가 요청해서 만든 순서대로 정리)

**탐색**: 최상위 폴더 목록 → 파일 목록 → 본문(쪽). 폴더·파일 이름 앞 두 자리 번호(`01_`)로 숫자키 점프(두 자리). 한 문서뿐인 폴더는 바로 열림.
`EXIT/DEL` 이전 위치, `AC` 최상위로, `F1` 쪽번호, `F2/F3` 색인 1/2, `0` 계산기, 숫자키로 쪽 점프, ← → 쪽 이동, ↑ ↓ 링크 선택/스크롤.
오른쪽 위 `현재쪽/전체쪽`(첫 줄에 자리가 있을 때). 쪽 수 제한 없음.

**링크**: `→폴더/파일:쪽`(쪽 생략=1쪽, 폴더 번호 앞 0 없음, 파일 번호 두 자리). 링크가 있는 쪽은 메뉴 쪽 — 줄 앞 `>` 칸 때문에 줄 폭 ≤ 122px, 쪽당 링크 ≤ 8개.
한 줄에 링크 1개면 줄 전체가 선택·밑줄, 제목 글자가 앞에 있고 `:` 로 끝나는 레이블이 아니면 `→..` 토큰은 화면에서 **숨김**. 링크가 여러 개인 줄(`연관: →3/01 →3/04`)은 토큰이 보이고 토큰별 선택.

**색인**: 폴더 `00` 또는 이름에 "색인"이 들어가는 폴더(최대 2개: 첫째 F2, 둘째 F3). 빌드할 때 파일 여러 개를 **하나의 이어진 문서**로 합치고 초성(ㄱ~ㅎ, A~Z) 목차 쪽과 쪽 제목(`색인 ㄱ 각~계`)을 자동 생성. F2/F3 은 마지막으로 보던 쪽에서 열림, 색인 안에서 같은 키는 1쪽(목차).

**글자/수식** (`$` 가 있는 줄 = 수식 줄, 22px, 줄바꿈 없음):
- `_{..}` `^{..}` 첨자(일반 줄은 3×5 작은 글꼴; 수식 줄 분수 밖은 7px 로 위/아래 3px 이동), 그리스·수학 기호는 9/11px 폰트에서 보충.
- `$...$` 안: `{분자;분모}` 분수(중첩 불가), `√{..}`, `∑_{i=1}^{n}`(한계 쌓기), 글자 뒤 U+20D7 벡터 화살표, `|...|`(안에 분수/√/∑ 한계가 있으면 긴 막대, 폭 3px).
- 모자/막대: `ŷ ȳ Ŷ`(빌더가 기본 글자 + 결합 문자 U+0302/U+0304 로 분해, 뷰어가 글자 위에 그림, 폭 0).
- 표 줄: 한 줄에 `│`(U+2502)로 칸 구분, 연속된 줄이 한 표(테두리·칸선 자동, 행 11px + 표 아래 1px, 열 ≤ 6, 폭 = 열 폭 합 + 3×열 수 + 1).
- 그림: 노트에 `[그림 이름]` 한 줄 → `notes/figures/이름.png`(흑백, 폭 ≤ 128/122, 높이 ≤ 64)를 1비트로 변환해 그린다. 쪽 높이 합 = 일반 줄 10 + 수식 줄 22 + 표 행 11 + 그림 높이 ≤ 64.
- 금지 문자는 W 경고(& # ? ~ " ' · % * ! \ ∏ Π ∥ Ø Σ ⑪… 등; `|` 는 허용으로 바뀜).

**계산기(EXP 키)**: 진짜 계산기. Casio fx-9860GIII 설명서 v3.21 의 계산 우선순위(`6÷2(1+2)=1`, `−2^2=−4`, 곱셈 기호 생략)와 10자리·Norm 1 지수 표시(`2.5E-03`). 키: 숫자 . + − × ÷ ( ) (−) x² ^ EXP sin cos tan log ln, SHIFT+EXP=π, SHIFT+(−)=Ans, SHIFT+x²=√, SHIFT+log=10^x, SHIFT+ln=e^x, DEL/F2 지움, AC 식 지움, ↑ 직전 식, F6 Rad/Deg(표시 없음), EXIT 나가기. 글꼴은 카시오 Run-Mat 5×7 (tools/make_calc_font.py → src/calc_font.c). 화면은 원래 가짜 화면과 같은 구성(8px 줄 간격 3쌍, 네모 커서, 하단 JUMP DEL ▶MAT MATH 줄은 스크린샷 비트맵).

**메뉴 아이콘**: 30×19, 위에 이름(3×5 글자) 아래 그림(`net` 신경망, `gear` 톱니바퀴, `book`). `sets/<id>/icon.json`. 종료할 때 `gint_setrestart(1)`(안 하면 앱을 나갔다 다시 들어갈 때 가끔 안 열린다 — gint 문서의 알려진 현상).

## 4. 노트 작업 흐름 (사용자 ↔ 노트 프로젝트 ↔ 이 저장소)

1. **노트 프로젝트**(웹 Claude.ai 프로젝트): 지침은 `docs/PROJECT_PROMPT.md` 의 코드블록을 그대로 붙여넣은 것. 프로젝트 파일에 **`docs/check_bundle.zip`**(검사기 + 폰트 폭 표 + fig_tool.py)과 시험 자료를 올린다. 규칙이 바뀌면 `python3 tools/make_check_bundle.py` 로 번들을 다시 만들어 사용자에게 보내고, 노트 프로젝트에 "새 번들로 교체하고, 아래 규칙을 반영해줘"라고 전달하는 텍스트를 같이 준다.
2. 노트 프로젝트가 `notes-vN.zip`(루트: `notes/ README.md set.json CHANGES.txt`, 그림이 있으면 `notes/figures/`)을 낸다.
3. 이쪽: zip 을 **새 빈 폴더**에 풀고(§5 코드) → `sets/<id>/notes` 를 통째로 교체(폴더 번호가 바뀌는 큰 개편이 있었다 — 변경분만 덮어쓰지 말고 전체 zip 이면 교체) → `README.md`, `set.json` 복사(`icon.json` 은 세트 폴더에 있어서 안 지워진다) → `python3 tools/check_notes.py --set <id>`(오류 0) → 시뮬레이터로 화면 확인 → 커밋·푸시 → 러너 빌드 → 아티팩트 → `SendUserFile`.
4. 새 과목: `sets/<새id>/set.json`(`{"name": "영문8자이하", "title": "과목명"}`), `icon.json`, `notes/` 를 만들면 자동으로 앱이 늘어난다(`python3 tools/noteset.py list`).
5. 세트 삭제는 사용자가 확인한 뒤에만(`ml` → `mlmid`, `prod` → `prodsys` 로 대체하며 삭제한 전례. git 기록에서 복구 가능).

## 5. 자주 쓰는 작업 레시피

**사용자 zip 풀기** (한글 파일명이 cp437 로 깨져서 들어온다):
```python
import zipfile, os
z = zipfile.ZipFile(zip_path)
for i in z.infolist():
    n = i.filename
    if not i.flag_bits & 0x800: n = n.encode("cp437").decode("utf-8")
    assert ".." not in n.split("/") and not n.startswith("/")
    p = os.path.join(dest, n)
    if n.endswith("/"): os.makedirs(p, exist_ok=True); continue
    os.makedirs(os.path.dirname(p), exist_ok=True); open(p, "wb").write(z.read(i))
```
빈 새 폴더에 풀고, 파이썬은 `python3 -I`(풀린 파일을 신뢰하지 않는다).

**빌드 결과 받기** (GitHub MCP): `actions_list list_workflow_runs`(resource_id `build-g1a-local.yml`) → 최신 run 이 `completed/success` 인지 → `list_workflow_run_artifacts` → `actions_get download_workflow_run_artifact`(서명 URL, 몇 분 유효) → `curl -o` → `python3 -I` 로 안전하게 unzip → `.g1a` 매직(`aa ac bd af 90 88 9a 8d`) 확인 → `SendUserFile`. 푸시가 연속이면 러너가 실행을 순서대로 처리하므로 **가장 마지막 run** 의 아티팩트를 쓴다.

**푸시 후 확인 예약**: `send_later`(2~3분) 로 자기 자신에게 "빌드 확인" 메시지를 보내 두면 편하다. 러너가 꺼져 있으면(`queued` 가 길다) 사용자에게 `run.cmd` 를 켜라고 알린다.

**화면 확인**: PC 시뮬레이터 프레임(`.pbm`)을 PIL 로 확대해서 이미지로 본다(`tools/sim/sheet.py`), 또는 `hg_draw_text` 하네스(`hangul.c` + `font_data.c` + `notes.c` 링크)로 한 쪽씩 그린다. 사용자에게는 확대 이미지를 `SendUserFile` 로 보여 주면 반응이 좋다.

## 6. 사용자 선호 / 작업 방식 (중요)

- 한국어, 짧고 구체적으로. 결과는 먼저, 근거는 짧게. 확인되지 않은 것(실기 미검증)은 숨기지 말고 말한다.
- "계획만 / 아직 만들지 마"라고 하면 구현하지 않는다. "ㄱㄱ / ㅇㅇ"가 오면 진행.
- 큰 기능은 먼저 **PC 목업 이미지**로 보여 주고 확정 받는다(분수·표·그림이 그렇게 진행됐다).
- 빌드는 사용자의 PC 러너가 한다. 푸시하면 자동으로 빌드가 시작되므로, 사용자가 "빌드는 ㄴㄴ" 라고 하면 푸시도 하지 않는다(로컬 커밋만).
- PR 은 요청할 때만. 커밋 메시지 끝에 세션이 지정한 `Co-Authored-By` / `Claude-Session` 줄.
- 노트 내용(교재 해석, 정답)은 노트 프로젝트의 몫이다. 이쪽은 형식·검사·뷰어·빌드를 책임진다. 노트 규칙을 바꿀 때는 `docs/PROJECT_PROMPT.md` + `check_bundle.zip` + 노트 프로젝트에 붙여 넣을 짧은 텍스트 세 가지를 같이 낸다.
- 시험에서 앱을 써도 되는지는 사용자가 규정을 확인할 몫이다(한 번 안내함).

## 7. 실기에서 확인된 것 / 아직 모르는 것

확인됨(사용자 피드백): 7px 한글 가독, 폴더/파일/쪽 탐색, 색인, 링크 선택, 가짜 화면 모양(스크린샷 비트맵 그대로), "다 잘 된다"는 반응(색인·링크·ŷ 등).
**실기 미확인** — 계산기에서 보고 알려 달라고 한 것:
- 새 계산기 화면(하단 `JUMP DEL ▶MAT MATH` 줄이 원래와 같은지, 식·결과 줄 글자 간격), 계산 결과가 본체와 같은지(`6÷2(1+2)`, `1÷400`, `sin`/`ln`), `F6` Rad/Deg.
- 22px 수식 줄, 표, 그림(prodsys 의 BOM 트리·조립도·PLC 곡선·신뢰도 도식)이 실제 LCD 에서 읽히는지.
- 메뉴 아이콘(`ML MID`+신경망, `PRODSYS`+톱니바퀴) 모양, 앱을 나갔다 다시 들어갈 때 열리는지(`gint_setrestart`), `AC`(`KEY_ACON`) 동작.
- 앱 크기(`mlmid` 약 200KB, `prodsys` 약 270KB)에 따른 열기 속도.

## 8. 현재 상태 (이 문서 작성 시점)

- 마지막 커밋: `272f8a1` (계산기 화면 = 원래 Run-Mat 구성, 하단 기능 줄 비트맵). 직전 빌드 #25 성공, `mlmid.g1a`(200,832 B)와 `prodsys.g1a`(271,200 B)를 사용자에게 보냄.
- `mlmid`: notes-v7(폴더 9개, 파일 60개, 표 포함, 검사 오류 0/경고 0). `prodsys`: notes-v6(폴더 4개, 파일 82개, 그림 16개, 910쪽, 검사 오류 0/경고 0).
- 노트 프로젝트에 마지막으로 전달한 규칙 번들: `check_bundle` v7(그림·fig_tool.py 포함). 이후 코드 변경(계산기)은 노트 규칙과 무관하다.

## 9. 알려진 한계 / 아이디어 (사용자가 보류·거절한 것 포함)

- 거절/불필요: 키워드 바로 찾기(알파키 검색), 마지막 위치 기억(저장), 개념만 겹치는 링크 추가.
- 보류 가능한 개선: 대문자 모자(Ŷ) 위쪽 공간(줄 높이 문제), 색인 링크를 키워드가 있는 쪽으로 더 정밀하게(노트 쪽 작업), 빌드 전에 `check_notes.py` 를 워크플로에서 먼저 실행(오류면 실패), 시뮬레이터 화면 회귀 시험.
- 한계: 7px 보다 작은 글자는 안 읽힌다(그림 속 라벨은 노트 글로 쓰기). 파일 하나에 쪽 제한은 없지만 쪽 높이 합 ≤ 64px. 계산기는 분수 입력·행렬·방정식·통계가 없고 본체와 마지막 자리 반올림이 다를 수 있다.
- 도구 제약: 이 작업 환경에는 SH 크로스 컴파일러가 없어 `.g1a` 컴파일 오류는 러너 빌드에서야 드러난다(지금까지 컴파일/링크 실패는 수학 라이브러리를 직접 구현해서 피했다).
- 보안: self-hosted runner 는 저장소 코드를 PC 에서 그대로 실행한다. 저장소를 비공개로 두고, 남의 PR 코드를 러너로 돌리지 말 것. 토큰/비밀은 저장소에 없다(러너 등록 토큰은 설정할 때 GitHub 에서 새로 받는다).

## 10. 파일 외 인수 항목 (저장소에 없는 것)

- **노트 프로젝트(웹)**: 지침 텍스트(`docs/PROJECT_PROMPT.md`)와 `check_bundle.zip` 을 새 계정의 프로젝트에 다시 올려야 한다. 시험 자료(교안 PDF, 족보 등)와 지금까지의 노트 작성 대화는 저장소에 없다(그 결과물은 `sets/*/notes` 에 이미 있음). 새 프로젝트에서 이어 쓸 때는 현재 `sets/<id>/notes` 를 zip 으로 묶어 올리고 "이 번호·링크 체계를 유지해서 추가해줘"라고 시킨다. 번호를 바꾸면 모든 링크가 깨지므로 바꾸기 전에 물어보게 한다.
- **사용자가 올린 원본 zip/스크린샷**: 채팅 업로드에만 있다. 필요한 내용은 이미 저장소에 반영됨(가짜 화면 원본은 `src/fakecalc_data.c`, 아이콘은 `tools/make_icon.py`).
- **Casio 설명서**: https://www.casio.com/content/dam/casio/global/support/manuals/calculators/pdf/004-en/f/fx-9860GIII_Soft_v321_EN.pdf (계산 규칙 참고용; `pdftotext -layout` 으로 텍스트 추출 가능).
- gint 문서/소스: https://git.planet-casio.com/Lephenixnoir/gint , fxSDK: https://git.planet-casio.com/Lephenixnoir/fxsdk
