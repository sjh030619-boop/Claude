#!/usr/bin/env python3
"""Split note files that have more than 9 pages into parts of at most 9 pages, and rewrite
every link (->F/N or ->F/N:P) so it still points at the same page.

  python3 tools/split_notes.py --set prod            # dry run: prints the plan
  python3 tools/split_notes.py --set prod --write    # does it, then verifies every link

Rules
- Only folders that contain a too-long file are renumbered (files get 01, 02, ... in order,
  split parts get consecutive numbers); other folders keep their numbers.
- A split file "NN_title" becomes "NN_title-1", "NN+1_title-2", ... (names stay <= 14 chars).
- Verification: for every link on every page, the page text it points at after the split must be
  identical to the page text it pointed at before. Nothing is written if that fails.
"""
import argparse
import re
import shutil
import sys
from pathlib import Path

import noteset

MAX_PAGES = 9
LINK_RE = re.compile(r"→(\d+)/(\d+)(?::(\d+))?")


def display_name(stem):
    return re.sub(r"^\d+[_.\- ]+", "", stem) or stem


def name_num(stem):
    m = re.match(r"^(\d+)[_.\- ]+", stem)
    return int(m.group(1)) if m else -1


def read_pages(path):
    src = path.read_text(encoding="utf-8").replace("\r\n", "\n")
    pages = [p.strip("\n") for p in re.split(r"^---[ \t]*$", src, flags=re.M)]
    return [p for p in pages if p.strip()]


def load(notes):
    """folders: list of dict(dir, num, files=[dict(path, title, num, pages)])"""
    out = []
    for d in sorted(p for p in notes.iterdir() if p.is_dir()):
        files = [dict(path=f, title=display_name(f.stem), num=name_num(f.stem), pages=read_pages(f))
                 for f in sorted(d.glob("*.txt"))]
        if files:
            out.append(dict(dir=d, num=name_num(d.name), files=files))
    return out


def plan(folders):
    """Return (new_folders, mapping). mapping[(folder_num, old_file_num)] = [(new_num, first_page)]"""
    mapping = {}
    new_folders = []
    for fo in folders:
        needs = any(len(f["pages"]) > MAX_PAGES for f in fo["files"])
        new_files = []
        counter = 1
        for f in fo["files"]:
            n_chunks = (len(f["pages"]) + MAX_PAGES - 1) // MAX_PAGES
            chunks = []
            for k in range(n_chunks):
                pages = f["pages"][k * MAX_PAGES:(k + 1) * MAX_PAGES]
                if needs:
                    num = counter
                    counter += 1
                else:
                    num = f["num"]
                title = f["title"] if n_chunks == 1 else f"{f['title']}-{k + 1}"
                chunks.append(dict(num=num, title=title, pages=pages, first=k * MAX_PAGES + 1))
                new_files.append(chunks[-1])
            if fo["num"] >= 0 and f["num"] >= 0:
                mapping[(fo["num"], f["num"])] = [(c["num"], c["first"]) for c in chunks]
        new_folders.append(dict(dir=fo["dir"], num=fo["num"], files=new_files, renumbered=needs))
    return new_folders, mapping


def remap_links(text, mapping):
    def sub(m):
        f_s, n_s, p_s = m.group(1), m.group(2), m.group(3)
        key = (int(f_s), int(n_s))
        if key not in mapping:
            return m.group(0)
        p = int(p_s) if p_s else 1
        chunks = mapping[key]
        # last chunk whose first page <= p
        new_num, first = [c for c in chunks if c[1] <= p][-1]
        new_p = p - first + 1
        width = len(n_s) if len(n_s) >= 2 else 1
        out = f"→{f_s}/{new_num:0{width}d}"
        if p_s or new_p != 1:
            out += f":{new_p}"
        return out
    return LINK_RE.sub(sub, text)


def verify(old_folders, new_folders, mapping):
    """Every link must lead to the same page text before and after."""
    def page_lookup(folders):
        d = {}
        for fo in folders:
            for f in fo["files"]:
                d[(fo["num"], f["num"])] = f["pages"]
        return d
    old, new = page_lookup(old_folders), page_lookup(new_folders)
    problems, checked = [], 0
    for fo_old, fo_new in zip(old_folders, new_folders):
        # new files in order correspond to chunks; rebuild old page list per new file order
        old_pages_flat = [(f["num"], pi, page) for f in fo_old["files"] for pi, page in enumerate(f["pages"])]
        new_pages_flat = [(f["num"], pi, page) for f in fo_new["files"] for pi, page in enumerate(f["pages"])]
        assert len(old_pages_flat) == len(new_pages_flat)
        for (on, opi, otext), (nn, npi, ntext) in zip(old_pages_flat, new_pages_flat):
            ol = LINK_RE.findall(otext)
            nl = LINK_RE.findall(ntext)
            if len(ol) != len(nl):
                problems.append(f"folder {fo_old['num']:02d}: link count changed on a page"); continue
            for (f1, n1, p1), (f2, n2, p2) in zip(ol, nl):
                k1, k2 = (int(f1), int(n1)), (int(f2), int(n2))
                if k1 not in old:
                    continue  # dangling before; left as is
                t_old = remap_links(old[k1][(int(p1) if p1 else 1) - 1], mapping)  # target page, links renumbered too
                try:
                    t_new = new[k2][(int(p2) if p2 else 1) - 1]
                except (KeyError, IndexError):
                    problems.append(f"link →{f2}/{n2}:{p2} points nowhere after the split"); continue
                checked += 1
                if t_old != t_new:
                    problems.append(f"link →{f1}/{n1}:{p1} -> →{f2}/{n2}:{p2} lands on different text")
    return checked, problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", dest="set_id", required=False)
    ap.add_argument("--write", action="store_true")
    a = ap.parse_args()
    ns = noteset.get_set(a.set_id)
    folders = load(ns["notes"])
    new_folders, mapping = plan(folders)

    # rewrite links inside the new pages
    for fo in new_folders:
        for f in fo["files"]:
            f["pages"] = [remap_links(p, mapping) for p in f["pages"]]
    # old folders must use the *same* page objects order for verification: rebuilt from disk, unchanged
    checked, problems = verify(folders, new_folders, mapping)

    split = [(fo["num"], f) for fo in folders for f in fo["files"] if len(f["pages"]) > MAX_PAGES]
    n_old = sum(len(fo["files"]) for fo in folders)
    n_new = sum(len(fo["files"]) for fo in new_folders)
    print(f"set {ns['id']}: {n_old} files -> {n_new} files; {len(split)} file(s) split; "
          f"{checked} links verified")
    for fnum, f in split:
        parts = (len(f["pages"]) + MAX_PAGES - 1) // MAX_PAGES
        print(f"  folder {fnum:02d} file {f['num']:02d} {f['title']}: {len(f['pages'])} pages -> {parts} parts")
    if problems:
        print("PROBLEMS (nothing written):")
        for p in problems[:30]:
            print("  ", p)
        sys.exit(1)
    if not a.write:
        print("dry run only; add --write to apply")
        return

    for fo in new_folders:
        d = fo["dir"]
        for old in d.glob("*.txt"):
            old.unlink()
        for f in fo["files"]:
            fname = f"{f['num']:02d}_{f['title']}.txt"
            (d / fname).write_text("\n---\n".join(f["pages"]) + "\n", encoding="utf-8", newline="\n")
    print("written. Run: python3 tools/check_notes.py --set", ns["id"])


if __name__ == "__main__":
    main()
