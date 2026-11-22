"""Make the comparison images used by docs/latex/slides.tex.

Inputs : tests/data/person_desert.png (800 x 533) and the seam-carved output of our own program
         (docs/latex/figures/carved.png, made with `seam_carving --mode opt --width 560`).
Outputs: docs/latex/figures/{original,scaled,cropped,seams}.png

The red seams in seams.png are computed here with a small NumPy re-implementation of the same rules as
src/seam_seq.cpp (e1 energy with clamped central differences, DP, smallest column wins on ties). It is
for illustration only; the C++ program is the reference.

Run inside the ttss-dev container:  python3 scripts/make_latex_figures.py
"""
import os

import numpy as np
from PIL import Image

SRC = "tests/data/person_desert.png"
OUT = "docs/latex/figures"
TARGET_W = 560
N_SEAMS = 30


def energy(g):
    gp = np.pad(g, 1, mode="edge")
    dx = np.abs(gp[1:-1, 2:] - gp[1:-1, :-2])
    dy = np.abs(gp[2:, 1:-1] - gp[:-2, 1:-1])
    return dx + dy


def find_seam(e):
    h, w = e.shape
    m = e.copy()
    big = np.iinfo(np.int64).max // 4
    for y in range(1, h):
        prev = m[y - 1]
        left = np.concatenate(([big], prev[:-1]))
        right = np.concatenate((prev[1:], [big]))
        m[y] += np.minimum(np.minimum(left, prev), right)
    seam = np.empty(h, dtype=np.int64)
    seam[-1] = int(np.argmin(m[-1]))  # argmin returns the first (smallest) column on ties
    for y in range(h - 1, 0, -1):
        x = seam[y]
        # candidates x-1, x, x+1 inside the image; smallest M wins, ties -> smallest column
        cands = [c for c in (x - 1, x, x + 1) if 0 <= c < w]
        seam[y - 1] = min(cands, key=lambda c: (m[y - 1, c], c))
    return seam


def main():
    os.makedirs(OUT, exist_ok=True)
    img = Image.open(SRC).convert("RGB")
    w, h = img.size
    img.save(f"{OUT}/original.png")
    img.resize((TARGET_W, h), Image.LANCZOS).save(f"{OUT}/scaled.png")
    left = (w - TARGET_W) // 2
    img.crop((left, 0, left + TARGET_W, h)).save(f"{OUT}/cropped.png")

    # gray as in OpenCV BGR2GRAY (rounded), integer energy
    rgb = np.asarray(img).astype(np.int64)
    gray = (rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114 + 0.5).astype(np.int64)

    # track original column of every remaining pixel to draw seams on the original image
    cols = np.tile(np.arange(w), (h, 1))
    overlay = rgb.copy()
    g = gray.copy()
    for _ in range(N_SEAMS):
        s = find_seam(energy(g))
        rows = np.arange(h)
        overlay[rows, cols[rows, s]] = (230, 30, 30)
        keep = np.ones_like(g, dtype=bool)
        keep[rows, s] = False
        g = g[keep].reshape(h, -1)
        cols = cols[keep].reshape(h, -1)
    Image.fromarray(overlay.astype(np.uint8)).save(f"{OUT}/seams.png")
    print("written:", sorted(os.listdir(OUT)))


if __name__ == "__main__":
    main()
