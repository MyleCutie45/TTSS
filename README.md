# Parallel Seam Carving with OpenMP

University Parallel Computing project: **"Parallel Implementation of Seam Carving for Content-Aware
Image Resizing using OpenMP"**, based on Avidan & Shamir, *Seam Carving for Content-Aware Image
Resizing*, SIGGRAPH 2007.

Seam carving makes an image narrower (or shorter) by repeatedly removing the connected path of pixels
with the lowest "energy" (edge strength), so important objects keep their shape.

| Milestone | Content | Status |
| --------- | ------- | ------ |
| M0 | Research and architecture (`docs/`) | Done |
| M1 | Sequential implementation (`src/seam_seq.cpp`) | Done |
| M2 | OpenMP implementation (`src/seam_omp.cpp`) | Done (results identical to sequential; see limitation below) |
| M3 | Correctness testing (`tests/test_m3.cpp`, `docs/testing_report.md`) | Done |
| M4 | Benchmarking (`scripts/`, `results/`, `docs/benchmark_report.md`) | Done |
| M5–M6 | Optimization, report | Not started |

See `WORKLOG.md` for the detailed development history.

---

## 1. Requirements

The team uses **MSYS2 UCRT64** on Windows (same versions on every machine):

| Tool | Version used |
| ---- | ------------ |
| GCC | 16.2.0 |
| CMake | 4.5.0 (≥ 3.16 required) |
| Ninja | 1.13.2 |
| OpenCV | 5.0.0 |
| OpenMP | 5.2 (comes with GCC) |

Installation:

1. Install MSYS2 (https://www.msys2.org).
2. In the **"MSYS2 UCRT64"** terminal:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
             mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-opencv
   ```
3. Add `<msys64>\ucrt64\bin` to the Windows `PATH`, **before** any other MinGW installation.
   Check with `where g++`: the first line must be the MSYS2 one.

On Linux, install `g++`, `cmake`, `ninja-build` and `libopencv-dev` from the package manager.

---

## 2. Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Compiler flags: C++17 (`-std=c++17`), `-O2 -DNDEBUG -Wall -Wextra` (Release).
The sequential library `seam_seq` is compiled **without** OpenMP flags.

## 3. Test

```bash
ctest --test-dir build --output-on-failure          # everything
ctest --test-dir build -L quick                     # fast set (~30 s): after every change
ctest --test-dir build -L full                      # slow set (~8 min): before a merge
```

- `test_seq`: unit tests of every stage (hand-computed 4x4 example from `docs/algorithm.md`, tie rule,
  brute-force optimality, seam connectivity, removal, edge cases, height via transpose, determinism).
- `test_equivalence`: OpenMP vs sequential, every stage of every seam, many thread counts and schedules.
- `test_m3` (`--quick` / full): M3 tests – independent reference implementation, edge-case images,
  border seams, maximum contrast, large images, odd thread counts, oversubscription, repeated runs.
- `cli_*`: command-line checks (invalid arguments must fail, a normal run must succeed).

Extra checks (M3, see `docs/testing_report.md`): `python scripts/mutation_check.py` (do the tests catch
known bugs?) and, on Linux/WSL, `bash scripts/run_sanitizers.sh asan|tsan` (AddressSanitizer,
ThreadSanitizer).

## 3a. Benchmark (M4, see `docs/benchmark_report.md`)

```bash
python scripts/make_bench_images.py     # benchmark images (bench_images/, not in git)
python scripts/run_all_bench.py         # all experiments, about 7.5 h on the M4 laptop
python scripts/analyze_bench.py         # results/summary.md + docs/figures/*.png
```

`scripts/run_bench.py <experiment>` runs one experiment (`--list` shows them). Raw results are in
`results/<experiment>.csv`, one row per run.

Main results (Intel i7-1065G7, 4 cores / 8 threads, Windows 11, GCC 16.2 libgomp):
- All four stages parallel: slower than sequential in every configuration (speedup 0.01–0.68).
- DP sequential, energy + min + removal parallel: 1.4–1.8× faster for 1920×1080 and larger images.
- Same code under Linux (WSL2): the parallel DP is no longer slow, and all four stages parallel reach
  1.36–1.71× with 4 threads.

---

## 4. Run

```bash
build/seam_carving --input tests/data/boat_sea.png --output out/boat_640.png --width 640
build/seam_carving --input tests/data/person_desert.png --output out/desert.png --width 600 --height 400
build/seam_carving --input tests/data/boat_sea.png --output out/boat_640.png --width 640 --csv
```

| Option | Meaning | Default |
| ------ | ------- | ------- |
| `--input <path>` | Input image | required |
| `--output <path>` | Output image | required |
| `--width <W>` | Target width (1 … input width) | required |
| `--height <H>` | Target height (1 … input height) | input height |
| `--mode seq\|omp` | Sequential or OpenMP version | `seq` |
| `--threads <N>` | OpenMP threads (omp only) | all available |
| `--stages <list>` | Parallel stages, subset of `energy,dp,min,removal` (omp only) | all |
| `--persistent-dp` | DP with one persistent parallel region (omp only) | off |
| `--csv` | Print one machine-readable line | off |

Exit codes: `0` success, `1` invalid arguments, `2` cannot read/write an image.

OpenMP examples:

```bash
build/seam_carving --input tests/data/boat_sea.png --output out/b.png --width 640 --mode omp --threads 8
build/seam_carving --input tests/data/boat_sea.png --output out/b.png --width 640 --mode omp --threads 8 --stages energy,removal
OMP_SCHEDULE="dynamic,64" build/seam_carving ... --mode omp      # PowerShell: $env:OMP_SCHEDULE="dynamic,64"
```

All parallel loops use `schedule(runtime)`. If `OMP_SCHEDULE` is not set, the program uses `static`.

**Known limitation (measured on Windows + GCC libgomp):** every OpenMP synchronization costs tens to
hundreds of microseconds, so the parallel DP stage (one barrier per image row) is much slower than the
sequential one. Details: `docs/parallelization_strategy.md` §11.4 and `docs/benchmark_report.md` §7.1.
The same code built for Linux does not have this problem (measured in WSL2, benchmark report §6.9). On
Windows the fastest measured configuration therefore keeps the DP sequential: `--stages energy,min,removal`.

`--csv` prints one line with the columns

```
mode,threads,schedule,stages,in_w,in_h,out_w,out_h,seams,t_energy,t_dp,t_min,t_backtrack,t_removal,t_total
```

Times are in seconds and exclude image loading and saving (details: `docs/architecture.md`).

---

## 5. Repository Layout

```
include/   seam_common.h (shared types), seam_seq.h, seam_omp.h
src/       seam_seq.cpp (sequential, no OpenMP), seam_omp.cpp, main.cpp
tests/     test programs and tests/data/ (lossless PNG test images, see images/SOURCES.md)
scripts/   benchmark, analysis, mutation-check and sanitizer scripts; membw.cpp (bandwidth reference)
results/   raw benchmark data (*.csv, *.meta.json), summary.md, membw.txt
docs/      algorithm.md, parallelization_strategy.md, architecture.md, benchmark_plan.md,
           testing_report.md, benchmark_report.md, figures/ (benchmark plots)
```

## 6. Documentation

- `docs/algorithm.md` – the algorithm, pseudocode, worked 4x4 example, complexity.
- `docs/parallelization_strategy.md` – how each stage is parallelized with OpenMP, synchronization, races.
- `docs/architecture.md` – interface contract (functions, types, CLI, CSV).
- `docs/benchmark_plan.md` – benchmark methodology.
- `docs/testing_report.md` – M3 correctness testing (tests, mutation check, sanitizers).
- `docs/benchmark_report.md` – M4 benchmark results and analysis.

Test images: Unsplash photos, credits in `images/SOURCES.md`.
