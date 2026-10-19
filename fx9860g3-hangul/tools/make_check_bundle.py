#!/usr/bin/env python3
"""Build check_bundle.zip: a standalone copy of the notes checker for the Claude project that writes
the notes (no fonts needed: the glyph-width table is stored as advances_7.json).

  python3 tools/make_check_bundle.py [out_dir]     # default: docs/
"""
import json
import sys
import zipfile
from pathlib import Path

import check_notes

ROOT = Path(__file__).resolve().parent.parent
out_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs"
out_dir.mkdir(parents=True, exist_ok=True)

adv = check_notes.load_adv(7)
adv_json = json.dumps({str(k): v for k, v in sorted(adv.items())}, separators=(",", ":"))

readme = """노트 검사기 (CASIO fx-9860G III 노트용)

실행:
  python3 check_notes.py --dir <notes 폴더 경로>
예: zip 을 만들기 전에 작업 폴더가 work/notes/... 이고 work/set.json 이 있으면
  python3 check_notes.py --dir work/notes

출력의 E 는 반드시 0개여야 납품할 수 있다. W 는 가능하면 고친다.
검사하는 것: 줄 폭 128px(링크가 있는 쪽은 122px), 쪽당 6줄, 폰트에 없는 글자,
번호 중복, 없는 링크 대상(폴더/파일/쪽), 쪽당 링크 8개, 파일·폴더 이름 문자,
BOM, 쓰지 않기로 한 문자, set.json(notes 옆, name 은 영문 8자 이하).
이 폴더의 파일(check_notes.py, calcpage.py, advances_7.json)만 있으면 동작한다.

문제 쪽의 계산 탭(F2)·추가 탭(F1):
  쪽 끝에 지시줄:  @계산  /  @식 w1=㉡-2㉢(㉡-㉠)  /  @추가 →4/3:1
  본문의 원문자 변수 ㉠~㉭ 바로 뒤에 숫자를 붙여 쓴다(예 ㉠1.2). 화면에는 숫자만 보인다.
  계산 탭은 변수마다 새 값을 묻고, 채운 문제와 @식 결과를 보여 준다. 탭 줄 때문에 본문은 56px 이하.
  검사기는 @식 을 원래 값으로 계산해 출력하므로 본문 정답과 맞는지 확인한다.

그림 (fig_tool.py, Pillow 필요):
  python3 fig_tool.py 원본그림.jpg notes/figures/이름.png        # 128x54 안으로 흑백 변환 + 미리보기
노트 안에 [그림 이름] 한 줄(줄 전체)을 쓰면 그 자리에 그림이 그려진다.
검사기는 notes/figures/이름.png 존재, 폭(128, 링크 쪽 122)·높이(64)와 쪽 높이 합(64px)을 확인한다.
"""

out = out_dir / "check_bundle.zip"
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.write(Path(check_notes.__file__), "check_notes.py")
    z.write(ROOT / "tools" / "calcpage.py", "calcpage.py")
    z.writestr("advances_7.json", adv_json)
    z.write(ROOT / "tools" / "fig_tool.py", "fig_tool.py")
    z.writestr("README.txt", readme)
print(f"wrote {out} ({out.stat().st_size // 1024} KB, {len(adv)} glyph widths)")
