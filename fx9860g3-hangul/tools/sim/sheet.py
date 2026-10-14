#!/usr/bin/env python3
"""Contact sheet of sim frames: python3 tools/sim/sheet.py <framedir> <out.png> [cols]"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw
d, out = Path(sys.argv[1]), sys.argv[2]
cols = int(sys.argv[3]) if len(sys.argv) > 3 else 3
fr = sorted(d.glob("frame_*.pbm"))
S, pad = 3, 14
BG, FG = (172, 190, 160), (28, 36, 30)
tiles = []
for n, f in enumerate(fr):
    rows = f.read_text().split("\n")[2:66]
    im = Image.new("RGB", (128 * S, 64 * S), BG)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "1":
                ImageDraw.Draw(im).rectangle([x * S, y * S, x * S + S - 1, y * S + S - 1], fill=FG)
    tiles.append(im)
rowsn = (len(tiles) + cols - 1) // cols
W, H = 128 * S, 64 * S + 16
sheet = Image.new("RGB", (cols * (W + pad) + pad, rowsn * (H + pad) + pad), (50, 52, 58))
dr = ImageDraw.Draw(sheet)
for i, im in enumerate(tiles):
    x, y = pad + (i % cols) * (W + pad), pad + (i // cols) * (H + pad)
    sheet.paste(im, (x, y + 14))
    dr.text((x, y), f"#{i}", fill=(230, 230, 230))
sheet.save(out)
