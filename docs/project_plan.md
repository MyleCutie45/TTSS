# Project Plan – Parallel Seam Carving with OpenMP

Status: **draft for review** (2026-11-13). Based on `M0.md`–`M6.md`, `CLAUDE.md` and an environment check on the development machine.

---

## 1. Environment check (measured on 2026-11-13)

| Item | Result |
| ---- | ------ |
| CPU | AMD Ryzen 7 4800H, 8 cores / 16 logical processors |
| Cache (reported by Windows) | L2 4096 KB total, L3 8192 KB |
| RAM | 23.4 GB |
| OS | Windows 11 Home 10.0.26200 |
| C++ compiler | g++ 16.1.0 (MinGW-w64, WinLibs UCRT, installed via winget) |
| MSVC (`cl`) | Not found |
| CMake | `cmake.exe` exists (WinLibs bundle) but **fails to start** |
| OpenCV (C++) | **Not found** (no `OpenCVConfig.cmake`, `OpenCV_DIR` unset) |
| Python | 3.12.10 with matplotlib 3.11.0, pandas 3.0.6, numpy 2.4.6 (no `cv2`) |

### Blocker B1 – Windows blocks unsigned executables

- `cmake --version` and a freshly compiled OpenMP test program (`g++ -fopenmp`, compiled successfully) both exit with code `-1058471934` = `0xC0E90002`, with no output.
- The same happens outside the tool sandbox, so this is the machine's real behaviour.
- The most likely cause is **Windows Smart App Control / application control policy** blocking unsigned binaries. This has not been confirmed in Windows Security settings.
- Impact: we cannot run any program we build on native Windows. **M1 cannot start until this is resolved.**

### Blocker B2 – OpenCV for C++ is not installed

Required by M0/M1 for image I/O.

### Options to resolve B1 + B2 (decision D1)

| Option | How | Pros | Cons |
| ------ | --- | ---- | ---- |
| **A. WSL2 Ubuntu (recommended)** | `wsl --install`, then `sudo apt install build-essential cmake libopencv-dev` | Linux binaries are not affected by Smart App Control; OpenCV from apt in one command; ThreadSanitizer and AddressSanitizer available for M3; same CPU for benchmarks | One-time setup; files should live in the Linux filesystem for speed; a small virtualization overhead (to be measured, not assumed) |
| B. Native Windows | Check Windows Security → App & browser control → Smart App Control; install signed Kitware CMake; build or install OpenCV for MinGW | No virtualization | Turning Smart App Control off cannot be undone without reinstalling Windows; OpenCV for MinGW usually has to be built from source; no ASan/TSan with MinGW |

Note: the textbook (ch. 21.2) warns about MSVC's OpenMP 2.0 limits. With g++ (on WSL or MinGW) we get a modern libgomp, so `reduction(min:)`, `declare reduction`, `schedule(runtime)` and `OMP_PROC_BIND` are available. The exact `_OPENMP` value will be recorded once a program can run.

---

## 2. Review of the milestone files

| File | Summary | Issues / gaps found |
| ---- | ------- | ------------------- |
| M0 | Research + architecture, 4 docs, **no code** | Requires **5 variants** (seq, OMP-energy, OMP-DP, OMP-removal, combined). The architecture must allow choosing seq/OMP per stage without duplicating code. Docs must be in English; the existing textbook is in Vietnamese, so it counts as background, not as an M0 deliverable. |
| M1 | Sequential implementation, CLI, README | "Target dimensions" implies height too, so horizontal seams are needed (transpose approach). `README.md` does not exist yet (`READ.md` is a different instruction file and must not be overwritten). |
| M2 | OpenMP stages + combined | M2 writes `M(x,y)` with x = column, y = row. Our docs use (i, j) = (row, col), so the notation has to be mapped explicitly. "Scheduling experiments" → use `schedule(runtime)` + `OMP_SCHEDULE`. Argmin needs deterministic tie-breaking (smallest index). |
| M3 | Testing + `docs/testing_report.md` | Requires comparing **intermediate** energy and DP matrices. Stage functions must write into caller-provided buffers so tests can inspect them. Memory and race checking needs sanitizers, which favours WSL. No test framework is specified (see D4). |
| M4 | Benchmarks, CSV, 5 plots, report | 512², 1080p, 4K, 8K × 10/20/30 % × threads {1,2,4,8,16}. 16 = SMT on this CPU (8 physical cores). Needs **test images** at these sizes (D3). The 8K × 30 % configuration may take very long (estimate in §5). "Seam finding" = argmin + backtrack. |
| M5 | Optimizations, each benchmarked separately | Must keep three versions side by side: original sequential / basic OMP / optimized OMP. Each optimization is a separately switchable variant. |
| M6 | Report, README, slides, demo, Q&A | Depends on real data from M3–M5. Write in formal English. |

---

## 3. Proposed architecture (to be detailed in `docs/architecture.md` during M0)

```text
TTSS/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp              # CLI: --input --output --width --height --variant --threads --reps --csv
│   ├── image_buffer.hpp/.cpp # color (BGR) + gray buffers, fixed stride, logical width; OpenCV load/save
│   ├── seam_seq.hpp/.cpp     # sequential stages (reference, never modified after M1)
│   ├── seam_omp.hpp/.cpp     # OpenMP stages (same signatures as seq)
│   ├── carver.hpp/.cpp       # main loop; picks seq/omp per stage; per-phase timing
│   └── timer.hpp
├── tests/                    # test executable(s) + brute-force oracle
├── bench/                    # gen_images.py, run_bench.py, plot.py
├── data/                     # small test images (large generated images git-ignored)
├── results/                  # raw CSV from benchmark runs
└── docs/                     # algorithm, architecture, parallelization_strategy, benchmark_plan, reports
```

Variant selection, kept simple (no class hierarchy):

```cpp
struct Variant { bool ompEnergy, ompDP, ompMin, ompRemove; };
// seq      = {0,0,0,0}   omp-energy = {1,0,0,0}   omp-dp = {0,1,0,0}
// omp-remove = {0,0,0,1} omp (combined) = {1,1,1,1}
```

This satisfies M0 §3 (5 implementations) while keeping sequential and parallel code in separate files. The DP-cell rule (tie-breaking) lives in one shared inline function used by both versions.

Proposed defaults (to be approved in M0, decision D2):

- Energy: e1 with central differences (no division) on integer grayscale, borders clamped.
- Grayscale: integer `(299R + 587G + 114B + 500) / 1000`, computed once; the seam is then removed from the gray buffer too.
- Types: `uint8_t` gray, `int32` energy and M.
- Tie-breaking: middle, then left, then right; the last-row argmin picks the smallest index.

---

## 4. Plan per milestone

Each milestone ends with: build, tests, a WORKLOG entry, a summary, and the student's review before moving on.

### M0 – Research & architecture (no code)

| # | Task | Deliverable |
| - | ---- | ----------- |
| 0.1 | Resolve B1/B2 (decision D1) and verify with the OpenMP test program (print `_OPENMP`, team size) | Toolchain recorded in WORKLOG |
| 0.2 | Algorithm, complexity, DP dependencies, worked example | `docs/algorithm.md` |
| 0.3 | Directory layout, data structures, variant mechanism, CLI | `docs/architecture.md` |
| 0.4 | Per-stage OpenMP strategy, sync points, race analysis, what stays sequential | `docs/parallelization_strategy.md` |
| 0.5 | Test + benchmark methodology, image set, timing method, environment record | `docs/benchmark_plan.md` |
| 0.6 | Approval questions | End-of-M0 summary |

**Done when:** the 4 docs are written and the decisions in §6 are answered.

### M1 – Sequential baseline

| # | Task |
| - | ---- |
| 1.1 | CMake project (C++17, OpenCV, OpenMP linked but unused in seq), Release by default |
| 1.2 | `image_buffer`: load → gray, fixed-stride buffers, save |
| 1.3 | Stages: `computeEnergySeq`, `computeCumulativeSeq`, `findSeamSeq` (argmin + backtrack), `removeSeamSeq` |
| 1.4 | Carver loop for width; height via transpose |
| 1.5 | CLI + README (new `README.md`, leave `READ.md` alone) |
| 1.6 | Tests: dimensions, `validSeam`, exactly one pixel per row removed, brute-force oracle on tiny images, edge cases W = 1/2/3, H = 1 |
| 1.7 | First timing of seq on 1080p (to size the M4 budget) |

**Done when:** all tests pass, the output image looks right, and the seq time is recorded.

### M2 – OpenMP

| # | Task |
| - | ---- |
| 2.1 | `computeEnergyOmp` – parallel for over rows |
| 2.2 | `computeCumulativeOmp` – version A (parallel for per row) and version B (one parallel region, `omp for` per row) |
| 2.3 | `findMinOmp` – deterministic argmin (`declare reduction` or local + critical) |
| 2.4 | `removeSeamOmp` – parallel over rows, fixed stride |
| 2.5 | Combined variant + `--threads`, `schedule(runtime)` |
| 2.6 | Equivalence seq ↔ omp (seams each iteration + final bytes) with 1, 2, 4, 8, 16 threads |

**Done when:** all 5 variants are byte-identical to seq on the test set, over repeated runs.

### M3 – Correctness validation

Test matrix from textbook ch. 27: energy and DP matrices compared exactly; odd thread counts; repeated runs (≥ 50) to catch races; ASan/TSan if on WSL. Deliverable: `docs/testing_report.md`.

### M4 – Benchmarking

`bench/gen_images.py` (deterministic synthetic + optional real photos) → `run_bench.py` (warm-up, ≥ 5 reps, median, CSV with per-phase times + environment) → `plot.py` (5 required plots) → `docs/benchmark_report.md` with real numbers only.

### M5 – Optimization

Candidates, each as a separate variant with test + benchmark:

1. One persistent parallel region.
2. Local energy update.
3. Smaller types for bandwidth.
4. Branch-free DP inner loop (SIMD).
5. Scheduling comparison.
6. (Stretch) trapezoid tiling to cut barriers.

Keep an optimization only if the benchmarks support it. Deliverable: `docs/optimization_report.md`.

### M6 – Report & presentation

Report (15 sections, English), README, 10–15-slide outline, demo script, Q&A list. Everything must come from real code and real data.

---

## 5. Benchmark budget (ESTIMATE, to be replaced with M1 measurements)

- Work per run ≈ seams × (several passes over H × W pixels).
- For 7680×4320 at 30 % (2304 seams), a rough guess of 0.1–0.3 s per seam sequentially gives **~4–12 min per sequential run**. With ≥ 5 repetitions × several variants and thread counts, 8K alone could take hours.
- Proposal: run the full matrix for 512² / 1080p / 4K; for 8K use fewer repetitions (e.g. 3) and only seq + combined OMP. Decide after the M1 timing.
- Benchmark conditions: laptop plugged in, high-performance power mode, other apps closed. Runs are interleaved to reduce thermal bias.

---

## 6. Decisions

**Status (2026-11-13):** the student approved all recommendations. D5 (deadline) is still unknown.

| ID | Question | Recommendation |
| -- | -------- | -------------- |
| D1 | Toolchain: WSL2 Ubuntu or native Windows (Smart App Control off + build OpenCV)? | **WSL2** |
| D2 | Energy definition: central difference vs Sobel; gray vs per-channel | Central difference on integer gray (matches the paper's e1; simplest to verify) |
| D3 | Test images: synthetic generated, real photos (license?), or both | Both: synthetic for reproducibility, 2–3 CC0 photos for realism |
| D4 | Test framework: plain custom runner (no dependency) or single-header library (doctest/Catch2) | Plain runner + CTest (no extra dependency) |
| D5 | Deadline / time budget per milestone | Unknown – needed to scale M5 scope |
| D6 | Is forward energy / seam insertion in scope? | Out of scope unless time remains (not required by M0–M6) |
