#!/usr/bin/env python3
"""Validate notes/ against the calculator's real limits.

  python3 tools/check_notes.py            # check every note set (font 7px, like the build default)
  python3 tools/check_notes.py --set prodsys # one set
  python3 tools/check_notes.py --dir path/to/notes   # standalone: check one notes/ folder (zip bundle use)
  python3 tools/check_notes.py --font 9

Checks (E = error, W = warning):
  E  line wider than 128 px (122 px on pages that contain links: the add-in reserves a
     6 px gutter for the '>' marker; real glyph advances from the bitmap font).
     On a link line the "->F/N:P" token is hidden when the line has exactly one link and a title
     in front of it (title not ending in ':'), so the width is measured without the token.
  E  table rows (lines with '│' U+2502, cells separated by it; consecutive rows = one table): width
     = widest cell per column + 3 px per column + 1 px must fit (128 / 122 on link pages), at most 6
     columns, no '$' formula or link in a row; a table row is 11 px tall (+1 px under the table)
  E  figure line "[그림 이름]" (the whole line): notes/figures/<이름>.png must exist (PNG, black on white,
     width <= 128, <= 122 on link pages, height <= 64); its height counts in the page height
  E  page too tall: 10 px per line (6 lines) but a formula line (contains '$') is 22 px tall
     (see formula_width: `$...$` with {분자;분모} fractions and _{..}/^{..} scripts); '$' must be paired,
     a formula line cannot contain a link
  E  glyph missing from the font (would print '?')
  E  duplicate folder/file numbers; link to a folder/file/page that does not exist
  E  more than 8 links on one page
  W  first page of a file: line 2 is not "이럴때: ..." (not required in the 00_ index folder)
  E  folder/file name with a character Windows forbids (< > : " / \\ | ? *)
  W  folder/file name with characters other than Hangul, letters, digits, '-' and '_'
  E  file starts with a byte-order mark
  W  characters we agreed not to use (& # ? ~ " ' middle-dot % * ! | \\ and similar)
  W  folder/file name longer than 14 characters
  info: counts of lines starting with "불확실:" and "교재밖:"
Exit status 1 if there is any E. Paste the output back to the chat that wrote the notes.
"""
import argparse
import json
import re
import sys
import unicodedata
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import calcpage

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SCREEN_W, SCREEN_H = 128, 64
MAX_PAGES, MAX_LINKS, MAX_NAME = 9, 8, 14
TABLE_ROW_H, TABLE_MAX_COLS = 11, 6
MARK_W = 6  # '>' gutter the add-in reserves on pages that contain links
TINY_ADV = 4

HEIGHT = {7: 10, 9: 12, 11: 14}  # cell height written by build_assets.py


def load_adv(size):
    """code point -> advance, primary font first, larger fonts as fallback (like build_assets).
    Without the fonts/ folder (zip bundle) the table is read from advances_<size>.json."""
    fonts = ROOT / "fonts"
    if not (fonts / f"Galmuri{size}.bdf").exists():
        data = json.loads((HERE / f"advances_{size}.json").read_text(encoding="utf-8"))
        return {int(k): v for k, v in data.items()}
    adv = {}
    for fs in [size] + [s for s in (9, 11) if s != size]:
        text = (fonts / f"Galmuri{fs}.bdf").read_text(encoding="utf-8")
        for m in re.finditer(r"ENCODING (\d+)\nSWIDTH \d+ \d+\nDWIDTH (\d+) ", text):
            adv.setdefault(int(m.group(1)), int(m.group(2)))
    return adv


FORBIDDEN = set("&#?~\"'·%*!\\∏Π∥ØΣ") | set("ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩⅰⅱⅲⅳⅴⅵⅶⅷⅸⅹ")
NAME_OK = re.compile(r"^[0-9A-Za-z가-힣_\-]+$")
WIN_BAD = set('<>:"/\\|?*')   # not allowed in Windows file names (error); other odd characters only warn


MARKS = {"\u0302", "\u0304", "\u20d7"}      # combining hat / bar: zero width, drawn over the previous glyph


def decompose_marks(text):
    out = []
    for ch in text:
        if ord(ch) > 0x7F:
            d = unicodedata.normalize("NFD", ch)
            if len(d) == 2 and d[0].isascii() and d[1] in MARKS:
                out.append(d)
                continue
        out.append(ch)
    return "".join(out)


def rel(path):
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


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
        if line[i] in MARKS:
            i += 1
            continue
        if c in adv:
            w += adv[c]
        else:
            missing.add(line[i])
            w += adv.get(ord("?"), 5)
        i += 1
    return w


TOKEN_RE = re.compile(r"→\d+/\d+(?::\d+)?")


FIG_RE = re.compile(r"^\[그림 ([^\]\n]+)\]\s*$")


def fig_size(nd, name):
    """(w, h) of notes/figures/<name>.png read from the PNG header (no Pillow needed), or None."""
    p = nd / "figures" / f"{name}.png"
    try:
        head = p.read_bytes()[:24]
    except OSError:
        return None
    if head[:8] != b"\x89PNG\r\n\x1a\n" or head[12:16] != b"IHDR":
        return None
    return int.from_bytes(head[16:20], "big"), int.from_bytes(head[20:24], "big")


def segment_tall(seg):
    """|...| around a fraction {a;b}, a sqrt, or a sum with limits is drawn as tall bars (22 px band)."""
    for k, ch in enumerate(seg):
        if ch == "{" and (k == 0 or seg[k - 1] not in "_^"):
            return True
        if ch == "∑" and seg[k + 1:k + 2] in ("_", "^"):
            return True
    return False


def formula_width(line, adv, missing, err):
    """Width of a formula line (contains '$'): `$...$` may hold {num;den} fractions; `_{..}`/`^{..}`
    are 7px glyphs outside fractions and 4 px/char inside; '$' is not drawn. err collects problems."""
    def glyph(c):
        if c in MARKS:
            return 0
        if ord(c) in adv:
            return adv[ord(c)]
        missing.add(c)
        return adv.get(ord("?"), 5)

    def run(t, big):
        w, i = 0, 0
        while i < len(t):
            if t[i] in "_^" and t[i + 1:i + 2] == "{":
                j = t.find("}", i + 2)
                if j != -1:
                    w += sum(glyph(c) for c in t[i + 2:j]) if big else TINY_ADV * (j - i - 2)
                    i = j + 1
                    continue
            w += glyph(t[i])
            i += 1
        return w

    w, i, math, tall_close = 0, 0, False, -1
    while i < len(line):
        c = line[i]
        if c == "$":
            math = not math
            i += 1
            continue
        if c == "|":
            if tall_close == i:                    # closing tall bar
                tall_close = -1
                w += 3
                i += 1
                continue
            if tall_close < 0:
                j = line.find("|", i + 1)
                if j > 0 and segment_tall(line[i + 1:j]):
                    tall_close = j
                    w += 3
                    i += 1
                    continue
        if c == "∑" and line[i + 1:i + 2] in ("_", "^") and line[i + 2:i + 3] == "{":
            # sum with limits: _{lo} and/or ^{hi} stacked above / below the sign
            lo = hi = 0
            j = i + 1
            for _ in range(2):
                if line[j:j + 1] in ("_", "^") and line[j + 1:j + 2] == "{" and "}" in line[j + 2:]:
                    k = line.index("}", j + 2)
                    if line[j] == "_":
                        lo = k - j - 2
                    else:
                        hi = k - j - 2
                    j = k + 1
            w += max(glyph(c), TINY_ADV * lo, TINY_ADV * hi) + 1
            i = j
            continue
        if c == "√" and line[i + 1:i + 2] == "{":
            depth, j, semi = 1, i + 2, -1
            while j < len(line) and depth:
                if line[j] == "{":
                    depth += 1
                elif line[j] == "}":
                    depth -= 1
                elif line[j] == ";" and depth == 1 and semi < 0:
                    semi = j
                j += 1
            if not depth and semi < 0:
                w += 5 + run(line[i + 2:j - 1], True) + 2
                i = j
                continue
        if math and c == "{":
            depth, j, semi = 1, i + 1, -1
            while j < len(line) and depth:
                if line[j] == "{":
                    depth += 1
                elif line[j] == "}":
                    depth -= 1
                elif line[j] == ";" and depth == 1 and semi < 0:
                    semi = j
                j += 1
            if depth or semi < 0:
                err.append("분수 {분자;분모} 의 괄호나 ';' 가 맞지 않음")
                return w
            w += max(run(line[i + 1:semi], False), run(line[semi + 1:j - 1], False)) + 4
            i = j
            continue
        if c in "_^" and line[i + 1:i + 2] == "{":
            j = line.find("}", i + 2)
            if j != -1:
                w += run(line[i:j + 1], True)
                i = j + 1
                continue
        w += glyph(c)
        i += 1
    if math:
        err.append("'$' 가 짝이 맞지 않음")
    return w


def displayed_line(line):
    """What the viewer draws: a line with exactly one link token and a title in front of it
    (not a label ending in ':') shows the title only; the token is hidden."""
    toks = list(TOKEN_RE.finditer(line))
    if len(toks) != 1:
        return line
    t = toks[0]
    title = line[:t.start()].rstrip(" \r")
    if not title or title.endswith(":"):
        return line
    return title + line[t.end():].rstrip(" \r")


INFOS = {}
CTX = {}
TAB_BAR_H = 8   # bottom function-key bar of a page with the 계산 / 추가 tabs


def check(nd, font):
    adv = load_adv(font)
    max_lines = SCREEN_H // HEIGHT[font]

    if not nd.is_dir():
        sys.exit(f"{nd} not found")

    folders = []  # (name, num, [(path, title, num, pages)])
    global CTX
    CTX = calcpage.build_context(nd)
    def read_files(files, fnum_):
        out = []
        for f in files:
            num_ = name_num(f.stem)
            src = decompose_marks(f.read_text(encoding="utf-8").replace("\r\n", "\n"))
            pages = [p.strip("\n") for p in re.split(r"^---[ \t]*$", src, flags=re.M)]
            pages = [p for p in pages if p.strip()]
            shown = []
            for k, p in enumerate(pages):
                d, info = calcpage.split_page(p, CTX.get((fnum_, num_, k + 1)))
                shown.append(d)
                if info:
                    INFOS[(f, k + 1)] = info
            out.append((f, display_name(f.stem), name_num(f.stem), shown))
        return out
    for d in sorted(p for p in nd.iterdir() if p.is_dir()):
        files = sorted(d.glob("*.txt"))
        if files:
            folders.append((display_name(d.name), name_num(d.name), read_files(files, name_num(d.name))))
    root_files = sorted(nd.glob("*.txt"))
    if root_files:
        folders.append(("기타", -1, read_files(root_files, -1)))

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
    path_of = {(fnum_, n_): p_ for _, fnum_, fl_ in folders for p_, _, n_, _ in fl_ if fnum_ >= 0 and n_ >= 0}
    n_uncertain = n_outside = 0
    n_calc = []
    for d in sorted(p for p in nd.iterdir() if p.is_dir()):
        stem = re.sub(r"^\d+[_.\- ]+", "", d.name)
        if stem and set(stem) & WIN_BAD:
            E(f"폴더 이름에 Windows 에서 쓸 수 없는 문자: {d.name}")
        elif stem and not NAME_OK.match(stem):
            W(f"폴더 이름은 한글·영문·숫자·-·_ 만 쓰기로 함: {d.name}")
    for fname, fnum, files in folders:
        for path, title, num, pages in files:
            where = rel(path)
            if set(title) & WIN_BAD:
                E(f"{where}: 파일 이름에 Windows 에서 쓸 수 없는 문자")
            elif not NAME_OK.match(title):
                W(f"{where}: 파일 이름은 한글·영문·숫자·-·_ 만 쓰기로 함")
            raw_bytes = path.read_bytes()
            if raw_bytes.startswith(b"\xef\xbb\xbf"):
                E(f"{where}: BOM 이 있음 (BOM 없는 UTF-8 로 저장)")
            if len(title) > MAX_NAME:
                W(f"{where}: 파일명이 {MAX_NAME}자 초과 ({len(title)}자)")
            for pi, page in enumerate(pages, 1):
                lines = page.split("\n")
                figs = []
                for l in lines:
                    fm = FIG_RE.match(l)
                    figs.append(fig_size(nd, fm.group(1).strip()) if fm else False)
                nfig_h = sum(f[1] for f in figs if f)
                nfig = sum(1 for f in figs if f is not False)
                is_tab = ["│" in l for l in lines]
                nform = sum(1 for l, t in zip(lines, is_tab) if "$" in l and not t)
                ntab = sum(is_tab)
                ngrp = sum(1 for i, t in enumerate(is_tab) if t and (i == 0 or not is_tab[i - 1]))
                # normal 10 px; formula line 22 px; table row 11 px, +1 px under each table
                height = (((len(lines) - ntab - nfig) * HEIGHT[font]) + nform * 12 + ntab * TABLE_ROW_H + ngrp
                          + nfig_h)
                info = INFOS.get((path, pi))
                budget = SCREEN_H - (TAB_BAR_H if info and (info.calc or info.extra) else 0)
                if height > budget:
                    extra = (f", 수식 줄 {nform}개(각 {HEIGHT[font] + 12}px)" if nform else "") + \
                            (f", 표 줄 {ntab}개(각 {TABLE_ROW_H}px)" if ntab else "") + \
                            (f", 그림 {nfig}개(높이 합 {nfig_h}px)" if nfig else "")
                    E(f"{where} 쪽{pi}: 높이 {height}px > {budget}px ({len(lines)}줄{extra})" + (" (계산/추가 탭 줄 8px 제외)" if budget < SCREEN_H else ""))
                missing = set()
                has_link = bool(link_re.search(page))
                limit = SCREEN_W - (MARK_W if has_link else 0)
                li = 0
                while li < len(lines):
                    line = lines[li]
                    li += 1
                    fgm = FIG_RE.match(line)
                    if fgm:                              # figure line: file, size
                        fname_ = fgm.group(1).strip()
                        sz = fig_size(nd, fname_)
                        if sz is None:
                            E(f"{where} 쪽{pi} 줄{li}: 그림 파일 없음 또는 PNG 아님: notes/figures/{fname_}.png")
                        else:
                            if sz[0] > limit:
                                E(f"{where} 쪽{pi} 줄{li}: 그림 폭 {sz[0]}px > {limit}px ({fname_})")
                            if sz[1] > SCREEN_H:
                                E(f"{where} 쪽{pi} 줄{li}: 그림 높이 {sz[1]}px > {SCREEN_H}px ({fname_})")
                        continue
                    if "│" in line:                      # table: handled per group of consecutive rows
                        grp = [line]
                        while li < len(lines) and "│" in lines[li]:
                            grp.append(lines[li])
                            li += 1
                        cells = [[c for c in r.split("│")] for r in grp]
                        ncol = max(len(r) for r in cells)
                        colw = [0] * ncol
                        for r in cells:
                            for ci, cell in enumerate(r):
                                colw[ci] = max(colw[ci], line_width(cell, adv, missing))
                        tw = sum(colw) + 3 * ncol + 1
                        first = li - len(grp) + 1
                        if ncol > TABLE_MAX_COLS:
                            E(f"{where} 쪽{pi} 줄{first}: 표 열 {ncol}개 (최대 {TABLE_MAX_COLS}개) | {grp[0]}")
                        if tw > limit:
                            E(f"{where} 쪽{pi} 줄{first}: 표 폭 {tw}px > {limit}px | {grp[0]}")
                        for r in grp:
                            if "$" in r or link_re.search(r):
                                E(f"{where} 쪽{pi} 줄{first}: 표 줄에는 수식($)이나 링크를 둘 수 없음 | {r}")
                        continue
                    if "$" in line:
                        ferr = []
                        w = formula_width(line, adv, missing, ferr)
                        for m_ in ferr:
                            E(f"{where} 쪽{pi} 줄{li}: {m_} | {line}")
                        if has_link and link_re.search(line):
                            E(f"{where} 쪽{pi} 줄{li}: 수식 줄에는 링크를 둘 수 없음 | {line}")
                    else:
                        w = line_width(displayed_line(line) if has_link else line, adv, missing)
                    if w > limit:
                        note = " (링크가 있는 쪽은 선택 표시 칸 때문에 122)" if has_link else ""
                        E(f"{where} 쪽{pi} 줄{li}: 폭 {w}px > {limit}px{note} | {line}")
                if missing:
                    E(f"{where} 쪽{pi}: 폰트에 없는 글자 {''.join(sorted(missing))}")
                bad = sorted({c for c in page if c in FORBIDDEN})
                if bad:
                    W(f"{where} 쪽{pi}: 쓰기로 한 범위 밖의 문자 {''.join(bad)}")
                n_uncertain += sum(1 for l in lines if l.startswith("불확실:"))
                n_outside += sum(1 for l in lines if l.startswith("교재밖:"))
                if fnum != 0 and "색인" not in fname and "예제" not in fname and not info and pi == 1 and not (len(lines) > 1 and lines[1].startswith("이럴때:")):
                    W(f"{where} 쪽1: 둘째 줄이 '이럴때:' 로 시작하지 않음")
                if info:
                    for m_ in info.errors:
                        E(f"{where} 쪽{pi}: {m_}")
                    if (info.calc or info.extra) and link_re.search(page):
                        E(f"{where} 쪽{pi}: 계산/추가 탭이 있는 쪽 본문에는 링크를 둘 수 없음 (추가 정보는 @추가 줄로)")
                    if info.extra:
                        f_, fi_, pg_ = info.extra
                        if f_ not in index or fi_ not in index[f_]:
                            E(f"{where} 쪽{pi}: @추가 →{f_}/{fi_} 대상 없음")
                        elif not 1 <= pg_ <= index[f_][fi_]:
                            E(f"{where} 쪽{pi}: @추가 쪽 번호 범위 밖")
                        elif info.calc:
                            # solution pages from the @추가 page to the end of that file are filled with the numbers
                            vals0 = dict(info.vars)
                            tpath = path_of[(f_, fi_)]
                            for k2 in range(pg_, index[f_][fi_] + 1):
                                i2 = INFOS.get((tpath, k2))
                                if not i2:
                                    continue
                                for c_, v_ in i2.vars:
                                    if c_ not in vals0:
                                        E(f"{rel(tpath)} 쪽{k2}: 변수 {c_} 가 문제 쪽({where} 쪽{pi})에 없음")
                                    elif vals0[c_] != v_:
                                        E(f"{rel(tpath)} 쪽{k2}: 변수 {c_} 값 {v_} 가 문제 쪽({where} 쪽{pi})의 {vals0[c_]} 와 다름")
                                for e_ in i2.exprs:
                                    for ch in e_:
                                        if ch in calcpage.MARKS and ch not in vals0:
                                            E(f"{rel(tpath)} 쪽{k2}: {{={e_}}} 의 변수 {ch} 가 문제 쪽에 없음")
                    vals = dict(info.vars)
                    for lab, expr in info.formulas:
                        try:
                            r = calcpage.py_eval(expr, vals)
                            n_calc.append(f"{where} 쪽{pi}: {lab} = {r:.10g}")
                        except Exception as ex:
                            E(f"{where} 쪽{pi}: @식 {lab} 계산 실패 ({ex}) | {expr}")
                    if len(info.vars) > 8:
                        E(f"{where} 쪽{pi}: 변수 {len(info.vars)}개 (최대 8개)")
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
    if n_calc:
        print(f"info: 계산 탭 식 {len(n_calc)}개 (원래 값으로 계산한 결과 — 본문 정답과 맞는지 확인)")
        for m in n_calc[:60]:
            print("  ", m)
    for m in errors:
        print("E", m)
    nfiles = sum(len(f) for _, _, f in folders)
    print(f"\n폴더 {len(folders)}개, 파일 {nfiles}개 검사 — 오류 {len(errors)}, 경고 {len(warns)}"
          f"  (불확실: {n_uncertain}줄, 교재밖: {n_outside}줄)")
    return len(errors)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", dest="set_id", help="note set id; default: check every set")
    ap.add_argument("--dir", help="check this notes/ folder directly (standalone mode)")
    ap.add_argument("--font", type=int, choices=(7, 9, 11), default=7)
    a = ap.parse_args()
    if a.dir:
        nd = Path(a.dir).resolve()
        cfg = nd.parent / "set.json"
        bad = 0
        if cfg.is_file():
            try:
                c = json.loads(cfg.read_text(encoding="utf-8"))
                name = c["name"]
                if not (1 <= len(name) <= 8 and name.isascii()):
                    print(f"E set.json: name 은 영문 8자 이하여야 함 ({name!r})"); bad += 1
            except Exception as e:                      # noqa: BLE001
                print(f"E set.json 을 읽을 수 없음: {e}"); bad += 1
        else:
            print("W notes/ 옆에 set.json 이 없음 ({\"name\": \"영문8자이하\", \"title\": \"과목명\"})")
        print(f"===== {nd} =====")
        bad += check(nd, a.font)
        sys.exit(1 if bad else 0)
    import noteset
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
