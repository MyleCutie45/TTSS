# Demonstration Script (≈ 6 minutes)

Goal: show live that (1) the program resizes images content-aware, (2) all three versions give the
**same** image, (3) the optimized version is faster, (4) the tests pass.

The demo runs in the Linux container (the M5 platform). Times on the presentation machine will differ
from the report; say so if asked, and point to the measured tables.

## 0. Preparation (before the presentation)

```bash
docker build -t ttss-dev docker
MSYS_NO_PATHCONV=1 docker run --rm -it -v "<repo>:/src" -v ttss-build:/build ttss-dev bash
# inside the container:
cmake -S /src -B /build/rel -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build /build/rel
alias sc=/build/rel/seam_carving
mkdir -p /tmp/demo && cd /src
```

Open `tests/data/person_desert.png` in an image viewer next to the terminal. Check that
`bench_images/person_desert_1920x1080.png` exists (otherwise `python3 scripts/make_bench_images.py`).

## 1. What seam carving does (1 min)

```bash
sc --input tests/data/person_desert.png --output /tmp/demo/carved.png --width 600
```

Show input and `/tmp/demo/carved.png` side by side.
*Say:* "The person and the horizon keep their shape; the empty desert was removed."

## 2. Three versions, one result (2 min)

```bash
IMG=bench_images/person_desert_1920x1080.png
sc --input $IMG --output /tmp/demo/seq.png --width 1536 --mode seq --csv
sc --input $IMG --output /tmp/demo/omp.png --width 1536 --mode omp --threads 8 --stages energy,min,removal --csv
sc --input $IMG --output /tmp/demo/opt.png --width 1536 --mode opt --threads 8 --csv
sha256sum /tmp/demo/seq.png /tmp/demo/omp.png /tmp/demo/opt.png
```

*Point at:* the last CSV column (`t_total`, seconds) of each line and the three identical hashes.
*Say:* "384 seams removed. Same image bit for bit, because all arithmetic is integer and ties are
broken by the smallest column. Report values on this machine: 2.21 s, 1.37 s, 0.48 s."

## 3. Where the speedup comes from (1.5 min)

```bash
sc --input $IMG --output /tmp/demo/a.png --width 1536 --mode opt --threads 1 --csv               # all optimizations, 1 thread
sc --input $IMG --output /tmp/demo/b.png --width 1536 --mode opt --threads 8 --dp-block 0 --csv  # DP sequential
sc --input $IMG --output /tmp/demo/c.png --width 1536 --mode opt --threads 8 --csv               # blocked DP, K = 64
```

*Point at:* the `t_dp` column (11th column) and `t_total`.
*Say:* "One thread is already about 2.4× faster than the original, because of less work. The blocked DP
with one barrier per 64 rows halves the DP time with 8 threads."

## 4. Correctness tests (1 min)

```bash
ctest --test-dir /build/rel -L quick
```

*Say:* "13 quick tests, including the comparison of every stage of the optimized version with the
sequential one for 1–16 threads. The four full test programs have about 237 000 checks; mutation tests show the tests
detect a removed barrier."

## 5. Backup (only if asked)

- Race demo: `docs/testing_report.md` §5.4 (TSan reports the race when the DP barrier is removed).
- Height reduction: `sc --input tests/data/boat_sea.png --output /tmp/demo/h.png --width 700 --height 400 --mode opt --threads 8`.
- If Docker is not available on the presentation machine: show the recorded tables
  (`results/m5_summary.md`) and figures instead of live timings.
