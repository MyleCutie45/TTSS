"""Create the M4 benchmark images (docs/benchmark_plan.md, section 4).

Source photos (images/SOURCES.md):
  * preferred: full-resolution photos in images/original/<name>.jpg (ignored by git; download them
    by hand from the Unsplash links in images/SOURCES.md - Unsplash blocks automated downloads);
  * fallback:  the 800-px PNG copies in tests/data/ (always available). Large sizes are then strongly
    upscaled; the manifest records this.

Each source is center-cropped to the target aspect ratio and resized with Lanczos. Results are lossless
PNG files in bench_images/ (ignored by git) plus bench_images/manifest.json (source, crop, scale, hash).

Series:
  main     all four sizes from ONE photo (person_desert), so only the size changes between sizes;
  content  1920x1080 from the other four photos + seeded random noise (does content change run time?).

Usage:  python scripts/make_bench_images.py
Requires: Pillow, numpy.
"""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ORIG = ROOT / "images" / "original"
TESTDATA = ROOT / "tests" / "data"
OUT = ROOT / "bench_images"

NAMES = ["person_desert", "boat_sea", "tree_field", "building_windows", "face_portrait"]
MAIN_SOURCE = "person_desert"
MAIN_SIZES = [(512, 512), (1920, 1080), (3840, 2160), (7680, 4320)]
CONTENT_SIZE = (1920, 1080)
NOISE_SEED = 12345


def find_source(name):
    """Full-resolution original if present, otherwise the 800-px test copy."""
    for ext in (".jpg", ".jpeg", ".png"):
        p = ORIG / f"{name}{ext}"
        if p.exists():
            return p, "original"
    return TESTDATA / f"{name}.png", "tests/data (800 px)"


def crop_resize(img, w, h):
    """Center crop to the aspect ratio w:h, then resize to exactly w x h with Lanczos."""
    sw, sh = img.size
    if sw * h > sh * w:            # source too wide -> cut left/right
        cw, ch = sh * w // h, sh
    else:                          # source too tall -> cut top/bottom
        cw, ch = sw, sw * h // w
    left, top = (sw - cw) // 2, (sh - ch) // 2
    box = img.crop((left, top, left + cw, top + ch))
    return box.resize((w, h), Image.LANCZOS), (cw, ch)


def save(img, name, series, source, kind, crop, manifest):
    OUT.mkdir(exist_ok=True)
    dst = OUT / name
    img.save(dst)
    w, h = img.size
    entry = {
        "file": name, "series": series, "source": source, "source_kind": kind,
        "crop": crop, "size": f"{w}x{h}",
        "scale": round(w / int(crop.split("x")[0]), 3) if crop else None,
        "sha256": hashlib.sha256(dst.read_bytes()).hexdigest(),
    }
    manifest.append(entry)
    print(f"{name}: {source} [{kind}] crop {crop} scale {entry['scale']}", flush=True)


def main():
    manifest = []
    for name, sizes, series in [(MAIN_SOURCE, MAIN_SIZES, "main")] + \
                               [(n, [CONTENT_SIZE], "content") for n in NAMES if n != MAIN_SOURCE]:
        path, kind = find_source(name)
        src = Image.open(path).convert("RGB")
        for (w, h) in sizes:
            out, (cw, ch) = crop_resize(src, w, h)
            save(out, f"{name}_{w}x{h}.png", series, path.name, kind, f"{cw}x{ch}", manifest)

    # Seeded random noise: maximal "content" (no smooth regions at all).
    rng = np.random.default_rng(NOISE_SEED)
    w, h = CONTENT_SIZE
    noise = Image.fromarray(rng.integers(0, 256, size=(h, w, 3), dtype=np.uint8), "RGB")
    save(noise, f"noise_{w}x{h}.png", "content", f"numpy default_rng({NOISE_SEED})", "synthetic", None,
         manifest)

    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
