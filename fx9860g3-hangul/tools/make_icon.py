#!/usr/bin/env python3
"""30x19 monochrome add-in icon for the OS menu: a name (3x5 font) on top, a small picture below
(like the built-in icons).  Configured per note set in sets/<id>/icon.json:

  {"text": "ML MID", "art": "net"}        art: net | gear | book (default book); text: A-Z 0-9 space + - . / ( )

  python3 tools/make_icon.py <out.png> "<text>" <art>      # standalone
"""
import math
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_assets import TINY  # noqa: E402

W, H = 30, 19


def draw_text(px, text):
    text = text.upper()
    w = 4 * len(text) - 1
    x = (W - w) // 2
    for ch in text:
        g = TINY.get(ch)
        if g:
            for r, bits in enumerate(g.split()):
                for c, b in enumerate(bits):
                    if b == "1":
                        px[x + c, 1 + r] = 1
        x += 4


def line(px, x0, y0, x1, y1):
    dx, dy = abs(x1 - x0), abs(y1 - y0)
    sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
    err = dx - dy
    while True:
        if 0 <= x0 < W and 0 <= y0 < H:
            px[x0, y0] = 1
        if (x0, y0) == (x1, y1):
            break
        e2 = 2 * err
        if e2 > -dy:
            err -= dy
            x0 += sx
        if e2 < dx:
            err += dx
            y0 += sy


def box(px, x, y, w, h):
    for i in range(w):
        for j in range(h):
            px[x + i, y + j] = 1


def art_net(px):
    layers = [(4, [10, 16]), (14, [10, 16]), (24, [13])]
    pts = [[(x, y) for y in ys] for x, ys in layers]
    for a, b in zip(pts, pts[1:]):
        for (x0, y0) in a:
            for (x1, y1) in b:
                line(px, x0, y0, x1, y1)
    for layer in pts:
        for (x, y) in layer:                       # 3x3 filled node
            box(px, x - 1, y - 1, 3, 3)


def art_gear(px):
    cx, cy = 15, 13
    for y in range(H):
        for x in range(W):
            d = math.hypot(x - cx, y - cy)
            ang = math.atan2(y - cy, x - cx)
            near = min(abs(((ang / (math.pi / 4)) + 0.5) % 1.0 - 0.5), 1) * (math.pi / 4)   # distance to a multiple of 45 deg
            if 1.7 < d <= 3.9 or (3.9 < d <= 5.3 and near < 0.30):
                px[x, y] = 1


def art_book(px):
    for y in range(8, 18):                       # two pages
        px[5, y] = px[14, y] = px[15, y] = px[24, y] = 1
    for x in range(5, 15):
        px[x, 8] = px[x, 17] = 1
    for x in range(15, 25):
        px[x, 8] = px[x, 17] = 1
    for y in (11, 13, 15):                       # text lines
        for x in range(7, 13):
            px[x, y] = 1
        for x in range(17, 23):
            px[x, y] = 1


ARTS = {"net": art_net, "gear": art_gear, "book": art_book}


def make_icon(out, text, art="book"):
    im = Image.new("1", (W, H), 0)
    px = im.load()
    draw_text(px, text)
    ARTS.get(art, art_book)(px)
    # 1 = black pixel on white
    rgb = Image.new("RGB", (W, H), (255, 255, 255))
    rp = rgb.load()
    for y in range(H):
        for x in range(W):
            if px[x, y]:
                rp[x, y] = (0, 0, 0)
    rgb.save(out)
    return rgb


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    make_icon(sys.argv[1], sys.argv[2], sys.argv[3])
