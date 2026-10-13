#!/usr/bin/env python3
"""Render host_test PBM pages as a calculator-LCD-style preview.png.
   usage: python3 tools/preview.py <dir with page*.pbm> <out.png>"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw

d, out = Path(sys.argv[1]), sys.argv[2]
files = sorted(d.glob("*.pbm"))
S = 5  # pixel size
BG, FG, GRID = (172, 190, 160), (28, 36, 30), (160, 178, 148)
panels = []
for f in files:
    rows = f.read_text().split("\n")[2:66]
    im = Image.new("RGB", (128 * S, 64 * S), BG)
    dr = ImageDraw.Draw(im)
    for y in range(0, 64 * S, S): dr.line([(0, y), (128 * S, y)], fill=GRID)
    for x in range(0, 128 * S, S): dr.line([(x, 0), (x, 64 * S)], fill=GRID)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "1": dr.rectangle([x * S, y * S, x * S + S - 2, y * S + S - 2], fill=FG)
    panels.append(im)
pad = 24
W = 128 * S + pad * 2
canvas = Image.new("RGB", (W, len(panels) * (64 * S + pad) + pad), (50, 52, 58))
for i, p in enumerate(panels):
    canvas.paste(p, (pad, pad + i * (64 * S + pad)))
canvas.save(out)
