# fx-9860G III 한글 뷰어 (add-in)

계산기 화면에 한글을 보여주는 fxSDK/gint add-in 입니다. Galmuri(기본 7px 한글 픽셀폰트, SIL OFL)를
비트맵으로 내장해서 128×64 화면에 **16글자 × 6줄** 정도를 표시합니다.

## 쓰는 법
1. 글은 `notes/<폴더>/<파일>.txt` (UTF-8) 로 둡니다. 계산기에서 폴더 → 파일 → 본문 순서로 열립니다.
   - 한 파일 안에서 `---` 한 줄이 페이지 구분입니다.
   - 이름 앞의 숫자(`01_`)는 정렬용이고 화면에는 안 보입니다.
   - `notes/` 바로 아래 파일은 "기타" 폴더로 들어갑니다.
2. 폰트/노트 데이터 생성:
   ```
   python3 tools/build_assets.py              # 기본 7px, KS X 1001 전체(2350자) 포함, 약 60 KB
   python3 tools/build_assets.py --only-used  # notes/ 에 쓴 글자만, 훨씬 작음
   python3 tools/build_assets.py --font 11    # 11px 폰트 (10글자 × 5줄, 더 읽기 쉬움)
   python3 tools/build_assets.py --font 9     # 9px 폰트 (12글자 × 5줄)
   ```
3. 빌드 ([fxSDK](https://git.planet-casio.com/Lephenixnoir/fxsdk) 필요):
   ```
   fxsdk build-fx
   ```
   → `dist/Hangul.g1a` 를 USB 로 계산기 저장 메모리에 복사.

fxSDK CMake 템플릿이 버전에 따라 다르면 `fxsdk new hangul` 로 새 프로젝트를 만들고 `src/*`,
`assets-fx/icon.png` 와 `pages.txt`, `tools/` 만 옮겨서 `CMakeLists.txt` 의 SOURCES 를 맞춰주세요.

## Windows 에서 한 번에 빌드 (WSL2)
```
powershell -ExecutionPolicy Bypass -File tools\windows\build_windows.ps1
```
WSL2(Ubuntu) 가 없으면 안내가 나옵니다(`wsl --install -d Ubuntu` 후 재부팅). 처음에는 툴체인 빌드에
30~60분 이상 걸리고, 끝나면 `dist\Hangul.g1a` 가 생기며 탐색기가 열립니다.
(fxSDK 서버는 git.planet-casio.com 입니다. 실제 PC 에서는 아직 검증하지 못했습니다.)

## 번호 점프 / 색인 링크
- **번호 점프**: 폴더·파일 이름 앞의 숫자(`01_`)가 목록에 `01` 처럼 보입니다. 목록에서 숫자키를
  **두 자리로** 누르면(예: `0` `2`) 그 번호 항목으로 바로 갑니다. EXE 로 엽니다.
- **링크**: 본문 안에 `→폴더번호/파일번호` 또는 `→폴더번호/파일번호:쪽` 을 쓰면(예: `→1/02`, `→1/02:2`)
  그 페이지는 **메뉴 페이지**가 됩니다. 링크 줄 앞에 `>` 가 뜨고(폴더 목록과 같은 방식), ↑ ↓ 로 링크 줄을
  고르고 **EXE** 로 바로 이동합니다. **EXIT / DEL** 로 이전 위치(고르던 줄 그대로)로 돌아오고, **AC** 는
  어디서든 최상위 폴더 목록으로 갑니다. 링크가 있는 페이지는 `>` 칸 때문에 **줄 폭이 122픽셀 이하**여야 합니다.
  링크는 한 페이지에 여러 개(최대 8)여도 되고, 중첩은 6단계까지입니다.
  링크 줄은 `\n` 으로 직접 나눈 줄 기준이므로 한 줄이 화면 폭을 넘지 않게 쓰세요.
- **F2**: 어디서든(목록·본문) 번호 `00` 폴더(색인)로 순간이동합니다. EXIT 로 돌아옵니다.
  → `notes/00_색인/` 폴더를 만들어 키워드 → 위치 링크를 모아 두는 용도입니다.

## 본문에서: 쪽 번호 점프 / 가짜 계산기 화면
- **쪽 번호 점프**: 본문에서 숫자키 `1`~`9` 를 누르면 그 쪽으로 바로 갑니다. 없는 쪽 번호는 무시합니다.
  **한 파일은 9쪽 이하로** 만드세요(10쪽 이상이면 빌드 때 경고가 나오고, 10쪽 이후는 두 자리 입력으로만 갑니다).
- **0 = 가짜 계산기 화면**: 숫자 `0` 을 누르면 Run-Mat 사용 중인 화면처럼 보이는 그림
  (`2+2×2 → 6`, `(2+2)×2 → 8`, `1÷400 → 2.5E-03`, 입력 커서, 하단 `JUMP DEL ▶MAT MATH` 메뉴)이 뜹니다.
  그림일 뿐이고 **`0` 외의 키는 모두 무시**, `0` 을 다시 누르면 원래 쪽으로 돌아옵니다.
  화면은 실제 계산기 스크린샷을 픽셀 그대로 옮긴 비트맵(`src/fakecalc_data.c`)입니다. 다른 화면으로 바꾸려면
  `python3 tools/make_fakecalc.py 스크린샷.png` (LCD 한 픽셀 = 3×3 블록, 가장자리에 1픽셀 프레임+여백이 있는 구성).

## 첨자 / 특수문자
- 아래첨자 `_{...}`, 윗첨자 `^{...}` : 예) `a_{n+1}`, `x^{10}`, `e^{-x}`, `H_{2}O`.
  3×5 크기의 작은 글꼴로 그려지고 영문/숫자/`+-=().,/` 만 지원합니다 (중괄호 중첩 불가).
  중괄호 없이 쓴 `_` `^` 는 그냥 글자로 나옵니다. `²` `³` 은 그대로 써도 됩니다.
- 그리스 문자(α β γ θ π …)와 ≤ ≥ ≠ ≈ ∞ ∑ ∫ √ 등은 7px 폰트에 없는 것을 9px 폰트에서 가져와 채웁니다
  (조금 커 보일 수 있음).

## 조작
목록: ↑ ↓ 선택 · 숫자 2자리 점프 · EXE 열기 · F2 색인 · EXIT/DEL 뒤로 · AC 최상위 폴더로 (최상위에서는 EXIT 만 종료)
본문: ← → 페이지 · 숫자 쪽 점프 · 0 가짜 계산기 · ↑ ↓ 한 줄 스크롤 (링크 페이지는 링크 줄 선택) · EXE 링크 열기 · F1 쪽번호 · F2 색인 · EXIT/DEL 이전 위치 · AC 최상위 폴더로

## 노트 검사 / 지침
- `python3 tools/check_notes.py` : 줄 폭(128px), 페이지 6줄, 파일 9쪽, 번호 중복, 링크 대상, 폰트에 없는 글자를 검사합니다.
- `docs/PROMPT.md` : 시험 노트를 만들 Claude 프로젝트용 지침(붙여 넣기)과 단계별 요청 프롬프트.

## PC 에서 동작 시뮬레이션 (gint 스텁)
```
gcc -Isrc -Itools/sim -Dmain=app_main tools/sim/sim.c src/main.c src/hangul.c \
    src/font_data.c src/notes.c -o sim
mkdir out && ./sim out "0 EXE EXE EXE DOWN DOWN EXE"    # 키: EXIT EXE UP DOWN LEFT RIGHT F1 F2 0-9
python3 tools/sim/sheet.py out sheet.png 3             # 프레임을 한 장으로
```

## PC 에서 미리보기
```
gcc -Isrc tools/host_test.c src/hangul.c src/font_data.c src/notes.c -o host_test
mkdir out && ./host_test out && python3 tools/preview.py out preview.png
```

## 주의
- 한글은 **이 add-in 안에서만** 보입니다. eActivity(.g2e)나 기본 Run-Mat 화면은 그대로입니다.
- 이 코드는 작성한 환경에 SH 크로스 툴체인이 없어 **add-in 빌드/실기 테스트는 못 했고**,
  폰트 변환·렌더링 로직만 PC 에서 검증했습니다.
- 폰트 라이선스: `fonts/Galmuri-OFL.txt`.
