/* Generated from notes/ by tools/build_assets.py -- do not edit. */
#include "hangul.h"

static const hg_file folder0[] = {
  { "근의공식",
    "[근의 공식]\n"
    "ax²+bx+c=0 일 때\n"
    "x = (-b±√D)/2a\n"
    "D = b²-4ac"
    "\f"
    "D>0 : 서로 다른 두 실근\n"
    "D=0 : 중근\n"
    "D<0 : 허근" },
  { "삼각함수",
    "[삼각함수 기본]\n"
    "sin²θ+cos²θ=1\n"
    "tanθ = sinθ/cosθ\n"
    "1+tan²θ = sec²θ"
    "\f"
    "[수열·첨자 예시]\n"
    "a_{n+1} = a_{n} + d\n"
    "x^{10}, e^{-x}, H_{2}O\n"
    "α β γ π ≤ ≥ ≠ ∞" },
};
static const hg_file folder1[] = {
  { "조작법",
    "안녕하세요!\n"
    "fx-9860G III 에서\n"
    "한글이 보입니다."
    "\f"
    "↑↓ 선택/스크롤\n"
    "EXE 열기\n"
    "← → 페이지 이동\n"
    "EXIT 뒤로가기\n"
    "F1 쪽번호 표시" },
};
static const hg_file folder2[] = {
  { "메모",
    "폴더 밖에 둔 파일은\n"
    "'기타' 폴더로 들어갑니다." },
};

const hg_folder hg_folders[] = {
  { "수학", folder0, 2 },
  { "사용법", folder1, 1 },
  { "기타", folder2, 1 },
};
const int hg_folder_count = 3;
