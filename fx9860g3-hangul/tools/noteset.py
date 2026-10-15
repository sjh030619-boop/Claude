#!/usr/bin/env python3
"""Note sets: one add-in per subject.

  sets/<id>/set.json      {"name": "mlmid", "title": "기계공학을위한인공지능"}   name = add-in name (<= 8 ASCII chars)
  sets/<id>/notes/<NN_folder>/<NN_file>.txt

  python3 tools/noteset.py list     # prints "<id><TAB><output .g1a name>" per set
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SETS = ROOT / "sets"


def list_sets():
    out = []
    dirs = sorted(p for p in SETS.iterdir() if p.is_dir()) if SETS.is_dir() else []
    for d in dirs:
        cfg = d / "set.json"
        if not ((d / "notes").is_dir() and cfg.is_file()):
            continue
        c = json.loads(cfg.read_text(encoding="utf-8"))
        name = c["name"]
        if not (1 <= len(name) <= 8 and name.isascii()):
            sys.exit(f"{cfg}: name must be 1-8 ASCII characters (the add-in name), got {name!r}")
        out.append({"id": d.name, "name": name, "title": c.get("title", d.name),
                    "notes": d / "notes", "output": name + ".g1a"})
    return out


def get_set(set_id=None):
    sets = list_sets()
    if not sets:
        sys.exit("no note sets found (expected sets/<id>/set.json + sets/<id>/notes/)")
    if set_id is None:
        if len(sets) == 1:
            return sets[0]
        sys.exit("several sets exist, pass --set one of: " + ", ".join(s["id"] for s in sets))
    for s in sets:
        if s["id"] == set_id:
            return s
    sys.exit(f"unknown set {set_id!r}; available: " + ", ".join(s["id"] for s in sets))


if __name__ == "__main__":
    if sys.argv[1:] == ["list"]:
        for s in list_sets():
            print(f"{s['id']}\t{s['output']}")
    else:
        sys.exit(__doc__)
