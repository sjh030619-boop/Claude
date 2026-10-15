#!/usr/bin/env python3
"""Validate notes/ against the calculator's real limits.

  python3 tools/check_notes.py            # check every note set (font 7px, like the build default)
  python3 tools/check_notes.py --set prod # one set
  python3 tools/check_notes.py --font 9

Checks (E = error, W = warning):
  E  line wider than 128 px (122 px on pages that contain links: the add-in reserves a
     6 px gutter for the '>' marker; real glyph advances from the bitmap font)
  E  more lines on a page than fit on screen (64 px / cell height = 6 for 7px)
  E  more than 9 pages in a file
  E  glyph missing from the font (would print '?')
  E  duplicate folder/file numbers; link to a folder/file/page that does not exist
  E  more than 8 links on one page
  W  line 2 is not "이럴때: ..." (not required in the 00_ index folder)
  W  folder/file name longer than 14 characters
Exit status 1 if there is any E. Paste the output back to the chat that wrote the notes.
"""
import argparse
import re
import sys
from pathlib import Path

import noteset

ROOT = Path(__file__).resolve().parent.parent
SCREEN_W, SCREEN_H = 128, 64
MAX_PAGES, MAX_LINKS, MAX_NAME = 9, 8, 14
MARK_W = 6  # '>' gutter the add-in reserves on pages that contain links
TINY_ADV = 4

HEIGHT = {7: 10, 9: 12, 11: 14}  # cell height written by build_assets.py


def load_adv(size):
    """code point -> advance, primary font first, larger fonts as fallback (like build_assets)."""
    adv = {}
    for fs in [size] + [s for s in (9, 11) if s != size]:
        text = (ROOT / "fonts" / f"Galmuri{fs}.bdf").read_text(encoding="utf-8")
        for m in re.finditer(r"ENCODING (\d+)\nSWIDTH \d+ \d+\nDWIDTH (\d+) ", text):
            adv.setdefault(int(m.group(1)), int(m.group(2)))
    return adv


def display_name(stem):
    return re.sub(r"^\d+[_.\- ]+", "", stem) or stem


def name_num(stem):
    m = re.match(r"^(\d+)[_.\- ]+", stem)
    return int(m.group(1)) if m else -1


def line_width(line, adv, missing):
    """Pixel width of one line, honouring _{..} / ^{..} (3x5 glyphs, 4 px each)."""
    w, i = 0, 0
    while i < len(line):
        if line[i] in "_^" and line[i + 1:i + 2] == "{":
            j = line.find("}", i + 2)
            if j != -1:
                w += TINY_ADV * (j - i - 2)
                i = j + 1
                continue
        c = ord(line[i])
        if c in adv:
            w += adv[c]
        else:
            missing.add(line[i])
            w += adv.get(ord("?"), 5)
        i += 1
    return w


def check(nd, font):
    adv = load_adv(font)
    max_lines = SCREEN_H // HEIGHT[font]

    if not nd.is_dir():
        sys.exit(f"{nd} not found")

    folders = []  # (name, num, [(path, title, num, pages)])
    def read_files(files):
        out = []
        for f in files:
            src = f.read_text(encoding="utf-8").replace("\r\n", "\n")
            pages = [p.strip("\n") for p in re.split(r"^---[ \t]*$", src, flags=re.M)]
            out.append((f, display_name(f.stem), name_num(f.stem), [p for p in pages if p.strip()]))
        return out
    for d in sorted(p for p in nd.iterdir() if p.is_dir()):
        files = sorted(d.glob("*.txt"))
        if files:
            folders.append((display_name(d.name), name_num(d.name), read_files(files)))
    root_files = sorted(nd.glob("*.txt"))
    if root_files:
        folders.append(("기타", -1, read_files(root_files)))

    errors, warns = [], []
    def E(msg): errors.append(msg)
    def W(msg): warns.append(msg)

    # numbering
    nums = [n for _, n, _ in folders if n >= 0]
    for n in {x for x in nums if nums.count(x) > 1}:
        E(f"폴더 번호 {n:02d} 중복")
    index = {}
    for fname, fnum, files in folders:
        fn = [n for _, _, n, _ in files if n >= 0]
        for n in {x for x in fn if fn.count(x) > 1}:
            E(f"[{fname}] 파일 번호 {n:02d} 중복")
        if fnum >= 0:
            index[fnum] = {n: len(p) for _, _, n, p in files if n >= 0}
        if len(fname) > MAX_NAME:
            W(f"폴더명이 {MAX_NAME}자 초과: {fname}")

    link_re = re.compile(r"→(\d+)/(\d+)(?::(\d+))?")
    for fname, fnum, files in folders:
        for path, title, num, pages in files:
            where = f"{path.relative_to(ROOT)}"
            if len(title) > MAX_NAME:
                W(f"{where}: 파일명이 {MAX_NAME}자 초과 ({len(title)}자)")
            if len(pages) > MAX_PAGES:
                E(f"{where}: {len(pages)}쪽 (최대 {MAX_PAGES}쪽) → 파일을 나누세요")
            for pi, page in enumerate(pages, 1):
                lines = page.split("\n")
                if len(lines) > max_lines:
                    E(f"{where} 쪽{pi}: {len(lines)}줄 (최대 {max_lines}줄)")
                missing = set()
                has_link = bool(link_re.search(page))
                limit = SCREEN_W - (MARK_W if has_link else 0)
                for li, line in enumerate(lines, 1):
                    w = line_width(line, adv, missing)
                    if w > limit:
                        note = " (링크가 있는 쪽은 선택 표시 칸 때문에 122)" if has_link else ""
                        E(f"{where} 쪽{pi} 줄{li}: 폭 {w}px > {limit}px{note} | {line}")
                if missing:
                    E(f"{where} 쪽{pi}: 폰트에 없는 글자 {''.join(sorted(missing))}")
                if fnum != 0 and not (len(lines) > 1 and lines[1].startswith("이럴때:")):
                    W(f"{where} 쪽{pi}: 둘째 줄이 '이럴때:' 로 시작하지 않음")
                links = link_re.findall(page)
                if len(links) > MAX_LINKS:
                    E(f"{where} 쪽{pi}: 링크 {len(links)}개 (최대 {MAX_LINKS}개)")
                for f_, fi_, pg in links:
                    f_, fi_ = int(f_), int(fi_)
                    if f_ not in index:
                        E(f"{where} 쪽{pi}: 링크 →{f_}/{fi_} 의 폴더 {f_:02d} 없음")
                    elif fi_ not in index[f_]:
                        E(f"{where} 쪽{pi}: 링크 →{f_}/{fi_} 의 파일 {fi_:02d} 없음")
                    elif pg and not 1 <= int(pg) <= index[f_][fi_]:
                        E(f"{where} 쪽{pi}: 링크 →{f_}/{fi_}:{pg} 쪽 번호 범위 밖 (1~{index[f_][fi_]})")

    for m in warns:
        print("W", m)
    for m in errors:
        print("E", m)
    nfiles = sum(len(f) for _, _, f in folders)
    print(f"\n폴더 {len(folders)}개, 파일 {nfiles}개 검사 — 오류 {len(errors)}, 경고 {len(warns)}")
    return len(errors)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", dest="set_id", help="note set id; default: check every set")
    ap.add_argument("--font", type=int, choices=(7, 9, 11), default=7)
    a = ap.parse_args()
    sets = noteset.list_sets() if a.set_id is None else [noteset.get_set(a.set_id)]
    if not sets:
        sys.exit("no note sets found (sets/<id>/set.json + sets/<id>/notes/)")
    bad = 0
    for ns in sets:
        print(f"===== set {ns['id']} ({ns['title']}) =====")
        bad += check(ns["notes"], a.font)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
