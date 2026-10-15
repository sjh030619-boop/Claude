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
추가 파일 없이 이 폴더의 파일 두 개(check_notes.py, advances_7.json)만 있으면 동작한다.
"""

out = out_dir / "check_bundle.zip"
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.write(Path(check_notes.__file__), "check_notes.py")
    z.writestr("advances_7.json", adv_json)
    z.writestr("README.txt", readme)
print(f"wrote {out} ({out.stat().st_size // 1024} KB, {len(adv)} glyph widths)")
