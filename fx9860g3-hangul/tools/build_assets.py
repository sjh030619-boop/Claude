#!/usr/bin/env python3
"""Generate src/font_data.c and src/notes.c for the Hangul add-in.

  python3 tools/build_assets.py --set prod      # note set sets/prod (omit --set if only one set)
  python3 tools/build_assets.py                 # Galmuri7, KS X 1001 + used chars
  python3 tools/build_assets.py --font 11       # bigger font (11px Hangul)
  python3 tools/build_assets.py --only-used     # smallest: only chars in pages.txt

Font: Galmuri (SIL OFL 1.1), BDF bitmap fonts in fonts/.
Text: notes/<folder>/<file>.txt (UTF-8); inside a file, pages are separated by a
line containing only '---'. Leading numbers in names (01_) only set the order.
"""
import argparse
import re
import sys
from pathlib import Path

import noteset

ROOT = Path(__file__).resolve().parent.parent


TINY = {  # 3x5 glyphs for _{...} / ^{...}; 5 rows, 3 bits each
    "0": "111 101 101 101 111", "1": "010 110 010 010 111", "2": "111 001 111 100 111",
    "3": "111 001 111 001 111", "4": "101 101 111 001 001", "5": "111 100 111 001 111",
    "6": "111 100 111 101 111", "7": "111 001 001 010 010", "8": "111 101 111 101 111",
    "9": "111 101 111 001 111",
    "A": "010 101 111 101 101", "B": "110 101 110 101 110", "C": "011 100 100 100 011",
    "D": "110 101 101 101 110", "E": "111 100 110 100 111", "F": "111 100 110 100 100",
    "G": "011 100 101 101 011", "H": "101 101 111 101 101", "I": "111 010 010 010 111",
    "J": "001 001 001 101 010", "K": "101 101 110 101 101", "L": "100 100 100 100 111",
    "M": "101 111 111 101 101", "N": "110 101 101 101 101", "O": "010 101 101 101 010",
    "P": "110 101 110 100 100", "Q": "010 101 101 011 001", "R": "110 101 110 101 101",
    "S": "011 100 010 001 110", "T": "111 010 010 010 010", "U": "101 101 101 101 111",
    "V": "101 101 101 101 010", "W": "101 101 111 111 101", "X": "101 101 010 101 101",
    "Y": "101 101 010 010 010", "Z": "111 001 010 100 111",
    "+": "000 010 111 010 000", "-": "000 000 111 000 000", "=": "000 111 000 111 000",
    "(": "001 010 010 010 001", ")": "100 010 010 010 100", ".": "000 000 000 000 010",
    ",": "000 000 000 010 100", "/": "001 001 010 100 100", " ": "000 000 000 000 000",
}

# Symbols the small fonts may lack; taken from a larger Galmuri when missing.
EXTRA = (set(range(0x391, 0x3AA)) | set(range(0x3B1, 0x3CA)) |
         {0x2264, 0x2265, 0x2260, 0x2248, 0x221E, 0x2211, 0x222B, 0x2202, 0x220F, 0x221A,
          0x2234, 0xB1, 0xD7, 0xF7, 0xBD, 0x2153, 0xBC, 0xB2, 0xB3, 0xB9, 0x207F, 0x3C0})


def ksx1001_chars():
    """KS X 1001 Hangul syllables (2350) + symbols + compat Jamo + Greek."""
    out = set()
    ranges = [(0xB0, 0xC8), (0xA1, 0xA2), (0xA4, 0xA5)]
    for lead, last in ranges:
        for hi in range(lead, last + 1):
            for lo in range(0xA1, 0xFF):
                try:
                    ch = bytes([hi, lo]).decode("euc-kr")
                except UnicodeDecodeError:
                    continue
                if len(ch) == 1:
                    out.add(ord(ch))
    return out


def parse_bdf(path, wanted):
    text = path.read_text(encoding="utf-8")
    asc = int(re.search(r"^FONT_ASCENT (\d+)", text, re.M).group(1))
    glyphs = {}
    for m in re.finditer(r"STARTCHAR.*?ENDCHAR", text, re.S):
        blk = m.group(0)
        enc = int(re.search(r"^ENCODING (-?\d+)", blk, re.M).group(1))
        if enc not in wanted:
            continue
        adv = int(re.search(r"^DWIDTH (\d+)", blk, re.M).group(1))
        w, h, xo, yo = map(int, re.search(r"^BBX (-?\d+) (-?\d+) (-?\d+) (-?\d+)", blk, re.M).groups())
        rows = re.search(r"^BITMAP\s*(.*?)ENDCHAR", blk, re.M | re.S).group(1).split()
        glyphs[enc] = (adv, w, h, xo, yo, rows)
    return asc, glyphs


def to_rows(g, top, height, fit=False):
    """Return `height` uint16 rows (MSB = leftmost pixel) in a cell whose top is
    `top` pixels above the baseline. With fit=True (glyph borrowed from a larger
    font) a glyph taller than the cell is shifted down instead of being cut."""
    adv, w, h, xo, yo, rows = g
    if fit:
        shift = max(0, (yo + h - 1) - top)  # rows sticking out above the cell
        yo -= shift
    cell = [0] * height
    for i, hexrow in enumerate(rows):
        nbits = len(hexrow) * 4
        v = int(hexrow, 16) >> (nbits - w)          # w-bit value
        v = (v << (16 - w)) & 0xFFFF
        v = (v >> xo) if xo >= 0 else (v << -xo) & 0xFFFF
        y = yo + h - 1 - i                           # pixel y above baseline
        r = top - y
        if 0 <= r < height:
            cell[r] = v
    return cell


def build_font(size, used, only_used):
    bdf = ROOT / "fonts" / f"Galmuri{size}.bdf"
    wanted = set(range(0x20, 0x7F)) | set(used)
    if not only_used:
        wanted |= ksx1001_chars()
    wanted |= EXTRA
    asc, glyphs = parse_bdf(bdf, wanted)
    own = dict(glyphs)  # cell height is decided by the selected font only
    for fb in (9, 11):
        gap = {c for c in wanted if c not in glyphs}
        if not gap or fb == size:
            continue
        _, more = parse_bdf(ROOT / "fonts" / f"Galmuri{fb}.bdf", gap)
        glyphs.update(more)
    missing = sorted(c for c in used if c not in glyphs and c not in (0x0A,))
    if missing:
        print("warning: no glyph for:", "".join(map(chr, missing)), file=sys.stderr)

    # vertical extent actually used
    top = max(g[4] + g[2] - 1 for g in own.values() if g[2])
    bot = min(g[4] for g in own.values() if g[2])
    height = top - bot + 1
    cps = sorted(glyphs)

    lines = [
        "/* Generated by tools/build_assets.py -- do not edit. */",
        "/* Font: Galmuri%d (c) Lee Minseo, SIL OFL 1.1 (see fonts/Galmuri-OFL.txt) */" % size,
        '#include "hangul.h"',
        "",
        "const int hg_height = %d;" % height,
        "const int hg_count = %d;" % len(cps),
        "const uint16_t hg_cps[] = {",
    ]
    lines += ["  " + ", ".join("0x%04X" % c for c in cps[i:i + 12]) + "," for i in range(0, len(cps), 12)]
    lines += ["};", "const uint8_t hg_adv[] = {"]
    lines += ["  " + ", ".join(str(glyphs[c][0]) for c in cps[i:i + 24]) + "," for i in range(0, len(cps), 24)]
    lines += ["};", "const uint16_t hg_bits[] = {"]
    for c in cps:
        r = to_rows(glyphs[c], top, height, fit=c not in own)
        lines.append("  " + ",".join("0x%04X" % v for v in r) + ",")
    lines.append("};")
    lines += ["", "/* 3x5 glyphs for _{} and ^{}: ASCII 0x20..0x7E, 5 rows of 3 bits (bit2 = left). */",
              "const uint8_t hg_tiny[95][5] = {"]
    for code in range(0x20, 0x7F):
        g = TINY.get(chr(code).upper(), "000 000 000 000 000")
        rows = [int(r, 2) for r in g.split()]
        lines.append("  {" + ",".join(map(str, rows)) + "},")
    lines.append("};")
    (ROOT / "src" / "font_data.c").write_text("\n".join(lines) + "\n", encoding="utf-8")
    nbytes = len(cps) * (2 + 1 + 2 * height)
    print(f"font: Galmuri{size}, {len(cps)} glyphs, cell height {height}px, ~{nbytes/1024:.1f} KB")


def display_name(stem):
    """'01_근의공식' -> '근의공식' (leading number only controls ordering)."""
    return re.sub(r"^\d+[_.\- ]+", "", stem) or stem


def name_num(stem):
    """'01_근의공식' -> 1; no numeric prefix -> -1."""
    m = re.match(r"^(\d+)[_.\- ]+", stem)
    return int(m.group(1)) if m else -1


def c_str(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def c_text(pages):
    """C literal for a file: pages joined by form feed."""
    parts = []
    for i, p in enumerate(pages):
        if i:
            parts.append('    "\\f"')
        lines = p.split("\n")
        for j, line in enumerate(lines):
            nl = "\\n" if j < len(lines) - 1 else ""
            parts.append(f'    "{c_str(line)}{nl}"')
    return "\n".join(parts) if parts else '    ""'


# --- index (folder 00) -------------------------------------------------------------------
JAMO_INITIALS = "ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ"
JAMO_GROUP = {"ㄲ": "ㄱ", "ㄸ": "ㄷ", "ㅃ": "ㅂ", "ㅆ": "ㅅ", "ㅉ": "ㅈ"}
ENTRY_RE = re.compile(r"^(.*?)\s*→\d+/\d+")


def initial_group(ch):
    """First character -> index group: 초성 for Hangul syllables, upper-case letter, '0-9', or '#'."""
    if "가" <= ch <= "힣":
        j = JAMO_INITIALS[(ord(ch) - 0xAC00) // 588]
        return JAMO_GROUP.get(j, j)
    if "ㄱ" <= ch <= "ㅎ":
        return JAMO_GROUP.get(ch, ch)
    if ch.isascii() and ch.isalpha():
        return ch.upper()
    if ch.isdigit():
        return "0-9"
    return "#"


def entry_chars(page):
    """First characters of the entry lines (lines with a link) of an index page."""
    out = []
    for line in page.split("\n")[1:]:
        m = ENTRY_RE.match(line.strip())
        if m and m.group(1).strip():
            out.append(m.group(1).strip()[0])
    return out


def merge_index(pages):
    """All pages of folder 00 -> one continuous document: a 초성 table of contents first, then every
    index page with a title that shows which part of ㄱ~ㅎ / A~Z it covers."""
    body = []
    first_page_of = {}            # group -> index into body
    for page in pages:
        chars = entry_chars(page)
        lines = page.split("\n")
        if chars:
            a, b = chars[0], chars[-1]
            ga, gb = initial_group(a), initial_group(b)
            title = f"색인 {ga} {a}~{b}" if ga == gb else f"색인 {ga}~{gb}"
            lines[0] = title
            for ch in chars:
                first_page_of.setdefault(initial_group(ch), len(body))
        body.append("\n".join(lines))

    # table of contents: "ㄱ 3  ㄴ 9 ..." packed into lines of <= ~124 px (estimated advances)
    def width(s):
        return sum(8 if ord(c) > 0x2E80 else 3 if c == " " else 5 for c in s)

    def toc_pages(offset, sep):
        tokens = [f"{g} {offset + i + 1}" for g, i in first_page_of.items()]
        lines, cur = [], ""
        for tok in tokens:
            cand = tok if not cur else cur + sep + tok
            if width(cand) <= 124:
                cur = cand
            else:
                lines.append(cur)
                cur = tok
        if cur:
            lines.append(cur)
        out = []
        for i in range(0, len(lines), 5):
            head = "색인 목차: 쪽번호 눌러 이동" if not out else "색인 목차 (계속)"
            out.append("\n".join([head] + lines[i:i + 5]))
        return out

    def best_toc(offset):
        """two spaces between entries (easier to read); one space only if that saves a page"""
        wide, tight = toc_pages(offset, "  "), toc_pages(offset, " ")
        return tight if len(tight) < len(wide) else wide

    offset = 0
    for _ in range(4):            # page numbers depend on how many TOC pages there are
        toc = best_toc(offset)
        if len(toc) == offset:
            break
        offset = len(toc)
    return toc + body


def check_dups(nums, what):
    seen = set()
    for n in nums:
        if n >= 0 and n in seen:
            print(f"warning: duplicate number {n:02d} among {what}s (number jump / links will hit the first)", file=sys.stderr)
        seen.add(n)


def build_notes(nd):
    if not nd.is_dir():
        sys.exit(f"{nd} not found (sets/<id>/notes/<folder>/<file>.txt)")
    folders = []
    for d in sorted(p for p in nd.iterdir() if p.is_dir()):
        files = sorted(d.glob("*.txt"))
        if files:
            folders.append((display_name(d.name), name_num(d.name), files))
    root_files = sorted(nd.glob("*.txt"))
    if root_files:
        folders.append(("기타", -1, root_files))
    if not folders:
        sys.exit(f"no .txt files under {nd}")

    used = set()
    lines = ["/* Generated from a note set by tools/build_assets.py -- do not edit. */",
             '#include "hangul.h"', ""]
    nfiles = npages = 0
    index_info = ""
    check_dups([n for _, n, _ in folders], 'folder')
    for fi, (fname, fnum, files) in enumerate(folders):
        used |= {ord(c) for c in fname}
        check_dups([name_num(f.stem) for f in files], f'file in {fname}')
        lines.append(f"static const hg_file folder{fi}[] = {{")
        if fnum == 0:
            # folder 00 = index: one continuous document (see merge_index)
            raw = []
            for f in files:
                src = f.read_text(encoding="utf-8").replace("\r\n", "\n")
                pg = [p.strip("\n") for p in re.split(r"^---[ \t]*$", src, flags=re.M)]
                raw += [p for p in pg if p.strip()]
            pages = merge_index(raw)
            used |= {ord(c) for p in pages for c in p} | {ord(c) for c in "색인"}
            lines.append('  { "색인",')
            lines.append(c_text(pages) + ", 1 },")
            nfiles += 1
            npages += len(pages)
            lines.append("};")
            index_info = f"index: {len(files)} file(s), {len(raw)} page(s) -> 1 document, {len(pages)} page(s) incl. table of contents"
            continue
        for f in files:
            title = display_name(f.stem)
            src = f.read_text(encoding="utf-8").replace("\r\n", "\n")
            pages = [p.strip("\n") for p in re.split(r"^---[ \t]*$", src, flags=re.M)]
            pages = [p for p in pages if p.strip()]
            used |= {ord(c) for c in title} | {ord(c) for p in pages for c in p}
            lines.append(f'  {{ "{c_str(title)}",')
            lines.append(c_text(pages) + f", {name_num(f.stem)} }},")
            nfiles += 1
            npages += len(pages)
        lines.append("};")
    lines.append("")
    lines.append("const hg_folder hg_folders[] = {")
    for fi, (fname, fnum, files) in enumerate(folders):
        count = 1 if fnum == 0 else len(files)
        lines.append(f'  {{ "{c_str(fname)}", folder{fi}, {count}, {fnum} }},')
    lines += ["};", f"const int hg_folder_count = {len(folders)};"]
    (ROOT / "src" / "notes.c").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"notes: {len(folders)} folder(s), {nfiles} file(s), {npages} page(s)")
    if index_info:
        print(index_info)
    return used


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", dest="set_id", help="note set id (sets/<id>); optional if only one set exists")
    ap.add_argument("--font", type=int, choices=(7, 9, 11), default=7)
    ap.add_argument("--only-used", action="store_true",
                    help="only include glyphs that appear in the notes (smallest)")
    a = ap.parse_args()
    ns = noteset.get_set(a.set_id)
    used = build_notes(ns["notes"])
    build_font(a.font, used, a.only_used)
    # add-in name / output file for CMake (see CMakeLists.txt)
    (ROOT / "src" / "addin.cmake").write_text(
        f'set(ADDIN_NAME "{ns["name"]}")\nset(ADDIN_OUTPUT "{ns["output"]}")\n', encoding="utf-8")
    print(f"set: {ns['id']} ({ns['title']}) -> add-in {ns['name']} / {ns['output']}")


if __name__ == "__main__":
    main()
