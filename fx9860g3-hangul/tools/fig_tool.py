#!/usr/bin/env python3
"""Turn any picture into a figure for the calculator notes: black on white PNG that fits the screen.

  python3 fig_tool.py in.jpg out.png                 # fit in 128 x 54 px (keeps the aspect ratio)
  python3 fig_tool.py in.png out.png --w 120 --h 40 --threshold 140
  python3 fig_tool.py in.png out.png --dither        # photos / shaded drawings (error diffusion)

Put the result in  notes/figures/<name>.png  and write a line  [그림 <name>]  (the whole line) in a note.
The tool prints an ASCII preview (# = black) so you can check it. Needs Pillow (pip install pillow).
Tips: simple line drawings work best; thin 1 px lines survive, text smaller than ~7 px does not
(write labels in the note text instead); the page keeps 64 px in total: a title line is 10 px.
"""
import argparse
import sys

from PIL import Image, ImageOps


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--w", type=int, default=128, help="max width (<= 128, 122 on pages with links)")
    ap.add_argument("--h", type=int, default=54, help="max height (<= 64; title line 10 px + figure 54 px)")
    ap.add_argument("--threshold", type=int, default=128, help="gray level below which a pixel is black")
    ap.add_argument("--dither", action="store_true", help="error-diffusion dithering instead of a threshold")
    a = ap.parse_args()
    if not (1 <= a.w <= 128 and 1 <= a.h <= 64):
        sys.exit("width must be 1..128 and height 1..64")
    im = Image.open(a.src)
    if im.mode in ("RGBA", "LA", "P"):
        im = im.convert("RGBA")
        bg = Image.new("RGBA", im.size, (255, 255, 255, 255))
        im = Image.alpha_composite(bg, im)
    im = ImageOps.autocontrast(im.convert("L"))
    im.thumbnail((a.w, a.h), Image.LANCZOS)
    if a.dither:
        bw = im.convert("1")
    else:
        bw = im.point(lambda v: 255 if v >= a.threshold else 0).convert("1")
    bw.convert("RGB").save(a.dst)
    print(f"{a.dst}: {bw.size[0]}x{bw.size[1]} px")
    px = bw.load()
    for y in range(bw.size[1]):
        print("".join("#" if px[x, y] == 0 else "." for x in range(bw.size[0])))


if __name__ == "__main__":
    main()
