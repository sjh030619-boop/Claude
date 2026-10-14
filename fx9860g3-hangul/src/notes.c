/* Generated from notes/ by tools/build_assets.py -- do not edit. */
#include "hangul.h"

static const hg_file folder0[] = {
  { "ㄱ-ㅅ",
    "[색인 ㄱ~ㅅ] EXE=이동\n"
    "근의공식,판별식 →1/01\n"
    "허근,중근,D<0 →1/01:2\n"
    "sin,cos,tan →1/02\n"
    "첨자,e^{-x} →1/02:2"
    "\f"
    "[색인 ㅇ~ㅎ]\n"
    "조작법,도움말 →2/01\n"
    "쪽번호,F1 →2/01:2", 1 },
};
static const hg_file folder1[] = {
  { "근의공식",
    "[근의 공식]\n"
    "ax²+bx+c=0 일 때\n"
    "x = (-b±√D)/2a\n"
    "D = b²-4ac"
    "\f"
    "D>0 : 서로 다른 두 실근\n"
    "D=0 : 중근\n"
    "D<0 : 허근", 1 },
  { "삼각함수",
    "[삼각함수 기본]\n"
    "sin²θ+cos²θ=1\n"
    "tanθ = sinθ/cosθ\n"
    "1+tan²θ = sec²θ\n"
    "참고: →1/01"
    "\f"
    "[수열·첨자 예시]\n"
    "a_{n+1} = a_{n} + d\n"
    "x^{10}, e^{-x}, H_{2}O\n"
    "α β γ π ≤ ≥ ≠ ∞", 2 },
};
static const hg_file folder2[] = {
  { "조작법",
    "안녕하세요!\n"
    "fx-9860G III 에서\n"
    "한글이 보입니다."
    "\f"
    "↑↓ 선택/스크롤\n"
    "EXE 열기\n"
    "← → 페이지 이동\n"
    "EXIT 뒤로가기\n"
    "F1 쪽번호 표시", 1 },
};
static const hg_file folder3[] = {
  { "메모",
    "폴더 밖에 둔 파일은\n"
    "'기타' 폴더로 들어갑니다.", -1 },
};

const hg_folder hg_folders[] = {
  { "색인", folder0, 1, 0 },
  { "수학", folder1, 2, 1 },
  { "사용법", folder2, 1, 2 },
  { "기타", folder3, 1, -1 },
};
const int hg_folder_count = 4;
