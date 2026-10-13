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
3. 빌드 ([fxSDK](https://gitea.planet-casio.fr/Lephenixnoir/fxsdk) 필요):
   ```
   fxsdk build-fx
   ```
   → `dist/Hangul.g1a` 를 USB 로 계산기 저장 메모리에 복사.

fxSDK CMake 템플릿이 버전에 따라 다르면 `fxsdk new hangul` 로 새 프로젝트를 만들고 `src/*`,
`assets-fx/icon.png` 와 `pages.txt`, `tools/` 만 옮겨서 `CMakeLists.txt` 의 SOURCES 를 맞춰주세요.

## 조작
목록: ↑ ↓ 선택 · EXE 열기 · EXIT 뒤로/종료
본문: ← → 페이지 · ↑ ↓ 한 줄 스크롤 · F1 쪽번호 · EXIT 목록으로

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
- 7px 폰트에는 그리스 문자(θ 등)가 없습니다. 필요하면 `--font 9` 이상을 쓰세요.
