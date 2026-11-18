# WORKLOG – Seam Carving OpenMP Project

## Project Information

**Project:** Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP

**Primary Language:** C++17

**Parallel Framework:** OpenMP

**Purpose:** Document all development activities, implementation decisions, experiments, bugs, and progress.

---

## Current Project Status

- Current milestone: M3 finished (branch `m3-testing`), M4 in progress
- Overall status: sequential + OpenMP versions bit-exact equal (M3: no defect found, tests + ASan + TSan); OpenMP synchronization overhead on Windows makes the parallel DP slow
- Last updated: 2026-11-19

## Milestone Progress

| Milestone | Description                   | Status      |
| --------- | ----------------------------- | ----------- |
| M0        | Research and Architecture     | Completed (contract approved by Member A; Members B/C agreement not yet recorded) |
| M1        | Sequential Implementation     | Completed |
| M2        | OpenMP Implementation         | Completed (correct; parallel DP slower than sequential on this platform) |
| M3        | Correctness Testing           | Completed (2026-11-19, `docs/testing_report.md`) |
| M4        | Performance Benchmarking      | In progress (scripts written, benchmarks not yet run) |
| M5        | Performance Optimization      | Not started |
| M6        | Final Report and Presentation | Not started |

---

# Development History

> Append new entries below. Never delete previous entries.

## Entry Template

### [YYYY-MM-DD HH:MM] – [Task Name]

**Milestone:**

**Status:** Completed / In Progress / Blocked

#### 1. Objective

What was the purpose of this task?

#### 2. Work Performed

Describe exactly what was implemented or changed.

#### 3. Technical Explanation

Explain:

- How the implementation works.
- Important algorithms and formulas.
- Important functions and variables.
- Relevant OpenMP concepts.
- Important design decisions.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
|      |        |             |

#### 5. Commands Executed

```bash
# Record actual commands executed
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
|      |                 |               |        |

#### 7. Problems Encountered

Describe bugs, errors, limitations, and unresolved issues.

#### 8. Decisions and Reasoning

Explain important technical decisions and why alternatives were rejected.

#### 9. Remaining Tasks

List unfinished work.

#### 10. Next Step

Describe the recommended next action.

### [2026-11-13] – Initialize Git repository and publish to GitHub

**Milestone:** Setup (pre-M0)

**Status:** Completed

#### 1. Objective

Put project under version control and push to a public GitHub repository.

#### 2. Work Performed

- Ran `git init`, created `.gitignore`, made initial commit, created public repo `TTSS` on GitHub with `gh`, pushed `main`.

#### 3. Technical Explanation

No code yet. `.gitignore` excludes the 21 MB original paper PDF (copyrighted; `link.md` keeps its URL), future CMake build dirs, editor and Python cache files.

#### 4. Files Created or Modified

| File       | Action   | Description                                          |
| ---------- | -------- | ---------------------------------------------------- |
| .gitignore | Created  | Ignore paper PDF, build outputs, editor/Python files |
| WORKLOG.md | Modified | Added this entry                                     |

#### 5. Commands Executed

```bash
git init -b main
git add .
git commit -m "Initial commit: project plan and docs"
gh repo create TTSS --public --source=. --remote=origin --push
```

#### 6. Testing and Results

| Test           | Expected Result               | Actual Result                                                                                 | Status |
| -------------- | ----------------------------- | --------------------------------------------------------------------------------------------- | ------ |
| Push to GitHub | Remote `main` matches local | Commit 48ffa6a pushed to https://github.com/MyleCutie45/TTSS; `main` tracks `origin/main` | Passed |

#### 7. Problems Encountered

None.

#### 8. Decisions and Reasoning

- Public visibility chosen by user.
- Paper PDF excluded: copyrighted, large binary; URL in `link.md` suffices.

#### 9. Remaining Tasks

- All milestones M0–M6.

#### 10. Next Step

Start Milestone 0.

### [2026-11-17 08:10] – Toolchain setup, smoke test and test images (work done 2026-11-16)

**Milestone:** Setup (pre-M0)

**Status:** Completed

#### 1. Objective

Prepare a modern C++17 + OpenMP + OpenCV toolchain that all three team members can install identically, and collect test images. This entry records preparation work done on 2026-11-16 (during planning no repo files were edited, so it is logged now).

#### 2. Work Performed

- Chose MSYS2 **UCRT64** as the common toolchain (installed at `C:\tools\msys64`).
- Installed GCC, CMake, Ninja and OpenCV from MSYS2 packages.
- Put `C:\tools\msys64\ucrt64\bin` before the old `C:\MinGW\bin` (GCC 6.3, 32-bit) in the system PATH.
- Built and ran a smoke-test program (in a scratch folder, not in the repo).
- Downloaded 5 Unsplash photos into `images/original/` (ignored by git) and created 800-px PNG copies in `tests/data/`.
- Wrote `images/SOURCES.md` (link, photographer, original size, purpose of each image).
- Added `images/original/` to `.gitignore`.

#### 3. Technical Explanation

- **Why not the old compiler:** GCC 6.3 (MinGW.org) is 32-bit and has incomplete C++17 support; 32-bit processes cannot use enough memory for 8K images.
- **Why MSYS2 instead of Visual Studio:** MSVC only supports OpenMP 2.0 by default; MSYS2 installs GCC + OpenCV with one `pacman` command, so all members get the same versions.
- **Why PNG test copies:** PNG is lossless, so every decoder produces exactly the same pixels. This is required for bit-exact comparison of outputs in M1–M3.

Installed versions (checked again on 2026-11-17 with `g++ --version`, `cmake --version`, `ninja --version`):

| Component | Version |
| --------- | ------- |
| GCC (MSYS2 UCRT64) | 16.2.0 (Rev7) |
| CMake | 4.5.0 |
| Ninja | 1.13.2 |
| OpenCV | 5.0.0 (`mingw-w64-ucrt-x86_64-opencv`) |
| OpenMP | 5.2 (`_OPENMP = 202111`) |
| CPU | Intel Core i9-13900H, 14 cores (6 P + 8 E), 20 logical processors |
| RAM | ~15.6 GB |
| OS | Windows 11 Home 10.0.26200 |

Test images (`tests/data/`, sizes measured with Pillow):

| File | Size (W x H) | Case |
| ---- | ------------ | ---- |
| boat_sea.png | 800 x 600 | Easy |
| tree_field.png | 800 x 533 | Easy |
| person_desert.png | 800 x 533 | Easy |
| building_windows.png | 533 x 800 | Hard (straight lines) |
| face_portrait.png | 800 x 450 | Hard (face) |

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| .gitignore | Modified | Ignore `images/original/` (large source photos) |
| images/SOURCES.md | Created | Image sources and purposes |
| tests/data/*.png (5 files) | Created | 800-px lossless test images |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```bash
# MSYS2 UCRT64 shell
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-opencv
# Smoke test (scratch folder outside repo)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Version check (2026-11-17)
g++ --version; cmake --version; ninja --version
```

The exact Pillow resize script was not saved (scratch work); method: longest side 800 px, Lanczos resampling.

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| C++17 | `__cplusplus == 201703` | 201703 | Passed |
| OpenMP | All logical processors used | 20 threads ran | Passed |
| OpenCV PNG round trip | imwrite → imread gives identical pixels | Identical | Passed |
| OpenCV `cvtColor` to gray | Type `CV_8UC1` | `CV_8UC1` | Passed |
| Smoke test with new system PATH | Builds and runs | Passed (run twice) | Passed |

#### 7. Problems Encountered

- ThreadSanitizer (race detector) does not work with MinGW on Windows. Races must be found with repeated seq-vs-omp comparison tests (M2/M3).
- New finding (2026-11-17): because the MSYS2 `ucrt64\bin` folder is first in PATH, the command `python` starts **MSYS2's Python**, which has no Pillow/numpy/matplotlib. Use the full path of another Python installation (e.g. conda) for Python scripts. Recorded in Known Issues.
- The old GCC 6.3 in `C:\MinGW` is still installed (not used).

#### 8. Decisions and Reasoning

See Technical Decision History (toolchain row).

#### 9. Remaining Tasks

- Members B and C install the same toolchain and report versions.

#### 10. Next Step

Milestone 0 documentation.

### [2026-11-17 08:45] – M0: Research and architecture documents

**Milestone:** M0

**Status:** Completed on 2026-11-17 (see update at the end of this entry). Members B and C have not yet confirmed the contract.

#### 1. Objective

Complete the M0 tasks from `M0.md` without writing implementation code: explain the algorithm and its
complexity, analyse dependencies, design the OpenMP strategy, identify races and synchronization points,
propose the directory structure and interface contract, and design the benchmark methodology.

#### 2. Work Performed

- `docs/algorithm.md`: paper summary (energy `e1`, seams, DP, removal, height via transpose), exact
  pseudocode of our variant, a hand-computed 4x4 example (energy, `M`, endpoint, backtrack with a tie,
  image after removal), a uniform-image example (all ties), implementation decisions D1–D7, complexity.
- `docs/parallelization_strategy.md`: OpenMP glossary, per-stage dependency analysis, directives, DP
  variant A (parallel for per row) and DP variant B (persistent region), the `nowait` race, deterministic
  `(value, column)` minimum reduction, why backtracking stays sequential (Amdahl), synchronization point
  table S1–S4, race checklist, bottleneck hypotheses.
- `docs/architecture.md`: directory structure, data types, function signatures (`seq` / `omp_impl`),
  `StageTimings` and `ParallelConfig`, CLI and CSV contract, data flow, build system, team workflow.
- `docs/benchmark_plan.md`: metrics, configurations from `M4.md`, image preparation, methodology
  (warm-up, ≥5 runs, median), hybrid-CPU notes, output format, plots, hypotheses.

#### 3. Technical Explanation

- **Parallel stages:** energy (independent pixels, parallel over rows), DP (rows sequential, columns
  parallel, barrier after each row), minimum endpoint (per-thread local minimum + sequential combine),
  seam removal (independent rows, `memmove`). **Sequential:** the outer seam loop (seam `i+1` needs the
  image after seam `i`) and backtracking (each row depends on the row below).
- **Key synchronization:** the implicit barrier at the end of `omp for` in DP; removing it (`nowait`)
  would let a thread read row `y` before it is complete.
- **Determinism:** integer energy/`M` (int32) and the "smallest column wins" tie rule; the parallel
  minimum compares `(value, column)` pairs, which is a total order, so the result does not depend on
  thread count or schedule.
- **4x4 example verification:** values were computed by hand and re-checked with a small independent Python
  script in the session scratch folder (not part of the repo). Both agree: seam `[1, 2, 1, 0]`, cost 140.
- **CSV detail (new, proposal):** `schedule` and `stages` values contain commas in natural form, so they
  are written with `:` and `+` (e.g. `dynamic:64`, `energy+dp+min+removal`); DP variant B is written
  `dp_persistent`. The 15 column names agreed during planning are unchanged.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/algorithm.md | Created | Algorithm, pseudocode, 4x4 example, decisions, complexity |
| docs/parallelization_strategy.md | Created | OpenMP design per stage, synchronization, races |
| docs/architecture.md | Created | Directory layout and interface contract (draft for review) |
| docs/benchmark_plan.md | Created | Preliminary benchmark methodology (M4 completes it) |
| WORKLOG.md | Modified | This entry, status tables, decisions, known issues |

#### 5. Commands Executed

```bash
g++ --version; cmake --version; ninja --version      # 16.2.0 / 4.5.0 / 1.13.2
git fetch origin; git status -sb                     # main up to date with origin/main
python ex4.py                                        # scratch check of the 4x4 example (outside repo)
<conda python> -c "...Pillow..."                     # measured test image sizes
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| 4x4 example cross-check (hand vs script) | Same energy, `M`, seam | Identical; seam `[1,2,1,0]`, cost 140 | Passed |
| Uniform 4x4 example | Seam `[0,0,0,0]` | `[0,0,0,0]` | Passed |

No C++ code exists yet, so no build or unit tests were run.

#### 7. Problems Encountered

- None blocking. The paper does not specify the discrete gradient, border handling, or tie-breaking;
  these are documented as our decisions D1–D7 in `docs/algorithm.md`.

#### 8. Decisions and Reasoning

Recorded in the Technical Decision History table below. Member A approved all proposals on 2026-11-17
(including the CSV `:`/`+` encoding and exit codes 1/2); agreement of Members B and C not yet recorded.

#### 9. Remaining Tasks

- Members B and C review `docs/architecture.md` and the other M0 documents; agree on the contract.
- After agreement: mark decisions as "frozen", set M0 to Completed.
- Commit M0 documents + test images + `images/SOURCES.md` + `.gitignore` + `WORKLOG.md` (after user approval).

#### 10. Next Step

Start M1 (Member A approved the M0 proposals; review by Members B and C still pending).

### [2026-11-17 10:30] – M1: Sequential seam carving implementation

**Milestone:** M1

**Status:** Completed

#### 1. Objective

Implement the complete sequential algorithm (`M1.md`) exactly as defined in `docs/algorithm.md`, with
the API of `docs/architecture.md`, as the correctness baseline for M2/M3.

#### 2. Work Performed

- `CMakeLists.txt`: C++17, Release `-O2 -DNDEBUG -Wall -Wextra`, OpenCV (core, imgproc, imgcodecs);
  static library `seam_seq` (no OpenMP flags), program `seam_carving`, test program `test_seq`, 5 CLI tests.
- `include/seam_common.h`: `StageTimings`, `ParallelConfig`, timing helper on `std::chrono::steady_clock`.
- `include/seam_seq.h`, `src/seam_seq.cpp`: `computeEnergy`, `computeCumulative`, `findMinEndpoint`,
  `backtrackSeam`, `removeSeam`, `carve` (width with vertical seams, then height via transpose).
- `src/main.cpp`: CLI with the agreed options, `--csv` line, exit codes 1/2. `--mode omp` returns
  exit code 1 with "not implemented yet (M2)".
- `tests/test_seq.cpp`: 12 test functions, 65 checks.
- `README.md`: installation, build, test, run, options, CSV format.

#### 3. Technical Explanation

- **Energy:** for each row, pointers to rows `y-1`, `y`, `y+1` (clamped) are taken once; for each
  column, left/right neighbours are clamped to `width-1` (current width, not `gray.cols`).
- **DP:** row 0 copied with `memcpy`; each next row reads only the previous row. `min` over the 2–3
  existing upper neighbours.
- **Endpoint / backtrack:** strict `<` comparisons, candidates visited in increasing column order →
  smallest column wins every tie (decision D5).
- **Removal:** per row, `memmove` the pixels right of the seam one position left, in the color image
  (3 bytes/pixel) and the gray image (1 byte/pixel). `memmove` because source and destination overlap.
- **carve:** checks arguments (throws `std::invalid_argument`), clones the input, `carveWidth` converts to
  gray once, allocates energy/`M`/seam once, loops `width > targetWidth`, crops with
  `colRange(0, targetWidth).clone()`. Height: `cv::transpose` → `carveWidth` → transpose back. The gray
  image for the height pass is converted from the already carved color image; since gray conversion is
  per-pixel, this equals carving the gray image.
- **Timing:** each stage measured with `steady_clock` and summed; `total` covers the whole `carve()`.
- **Tests use a custom `CHECK` macro, not `assert`:** Release defines `NDEBUG`, which disables `assert`.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| CMakeLists.txt | Created | Build, tests |
| include/seam_common.h | Created | Shared types and timer |
| include/seam_seq.h | Created | Sequential API |
| src/seam_seq.cpp | Created | Sequential implementation (no OpenMP) |
| src/main.cpp | Created | Command-line program |
| tests/test_seq.cpp | Created | Unit tests |
| README.md | Created | User documentation (READ.md unchanged) |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
.\build\test_seq.exe
# exit-code checks (no args, --width 801, missing input, --mode omp, unwritable output)
# each of the 5 test images, width -20 %, run twice, compare SHA-256 of outputs:
.\build\seam_carving.exe --input tests\data\<name>.png --output out\<name>_seq_a.png --width <0.8*W> --csv
.\build\seam_carving.exe --input tests\data\person_desert.png --output out\person_desert_wh.png --width 600 --height 400
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build (Release, `-Wall -Wextra`) | No errors, no warnings | No errors, no warnings | Passed |
| `ctest` | All pass | 6/6 passed (test_seq + 5 CLI tests) | Passed |
| `test_seq` checks | 0 failed | 65 checks, 0 failed (12/12 test functions PASS) | Passed |
| Exit codes | 1,1,2,1,2 | no args 1, width 801 1, missing file 2, mode omp 1, bad output path 2 | Passed |
| Determinism, 5 real images × 2 runs | Identical files | SHA-256 identical for all 5 | Passed |
| Output sizes (−20 % width) | 640/426 wide, height unchanged | 640×600, 640×533, 640×533, 426×800, 640×450 | Passed |
| Width + height (800×533 → 600×400) | 600×400 | 600×400, 200 vertical + 133 horizontal seams | Passed |
| Visual check (boat_sea, building_windows, face_portrait) | Main objects preserved | Boat and face unchanged, sea/background narrowed; building lines remain straight | Passed (subjective) |
| Mutation check: backtrack `<` → `<=` | Tests fail | 2 checks failed (4x4 tie, uniform image) | Passed (tests detect bug) |
| Mutation check: clamp with `gray.cols` | Tests fail | 2 checks failed (current width test) | Passed (tests detect bug) |

After the mutation check the original source was restored and all tests passed again.
`src/seam_seq.cpp` contains no `#pragma omp`.

Single-run timings printed by the CLI (one run each, not a benchmark; i9-13900H, Release `-O2`):
boat_sea 800×600 → 640: `t_total` 0.227 s; building_windows 533×800 → 426: 0.141 s. In every run,
energy + DP take > 90 % of the total. These are informal observations; real measurements are M4's task.

#### 7. Problems Encountered

- None.

#### 8. Decisions and Reasoning

- `CMAKE_CXX_FLAGS_RELEASE` overridden to `-O2 -DNDEBUG` (CMake's default for GCC is `-O3`) to follow the contract.
- `carve()` requires `CV_8UC3`; the CLI loads with `IMREAD_COLOR` so gray PNGs are expanded to 3 channels.
- `--mode omp` is parsed but rejected until M2, so the CLI format is already final.

#### 9. Remaining Tasks

- M2 (OpenMP). Members B/C review of the contract still not recorded.

#### 10. Next Step

M2: `include/seam_omp.h`, `src/seam_omp.cpp`, equivalence tests.

### [2026-11-17 15:30] – M2: OpenMP implementation and equivalence tests

**Milestone:** M2

**Status:** Completed for correctness (all configurations bit-exact equal to the sequential version).
Performance: the parallel DP is much SLOWER than the sequential DP on this machine (measured, see
sections 6 and 7).

#### 1. Objective

Parallelize energy, DP (two variants), minimum endpoint and seam removal with OpenMP as designed in
`docs/parallelization_strategy.md`, keep the sequential code unchanged, allow thread count and schedule
experiments, and prove equivalence with the sequential version.

#### 2. Work Performed

- `include/seam_omp.h`, `src/seam_omp.cpp` (namespace `omp_impl`): same functions as `seq` plus a
  `ParallelConfig` parameter per stage; `carve()` with `config.threads`; helpers `defaultThreadCount`,
  `scheduleDescription`, `useStaticScheduleUnlessSet`.
- `CMakeLists.txt`: `find_package(OpenMP)`, library `seam_omp` (only this library is compiled with
  `-fopenmp`), test `test_equivalence`, CLI tests `cli_bad_stage`, `cli_zero_threads` (must fail),
  `cli_omp_run`.
- `src/main.cpp`: `--mode omp`, `--threads`, `--stages` (list or `none`), `--persistent-dp`; CSV columns
  `threads`, `schedule` (`:` for chunk), `stages` (`+`, `dp_persistent`).
- `tests/test_equivalence.cpp`: 4 tests (lockstep per stage and per seam; full carve on the 5 real images;
  minimum endpoint with many ties; DP stress).
- Docs: `parallelization_strategy.md` §11 "Implementation Notes (M2)" (directives, default schedule, what
  stays sequential, measured OpenMP overhead); `architecture.md` (final `omp_impl` signatures, `--stages none`,
  static default schedule). `README.md` updated.

#### 3. Technical Explanation

- **Energy / removal:** `#pragma omp parallel for schedule(runtime)` over rows; each thread writes only
  its rows (energy rows; color+gray rows with `memmove`). Implicit barrier at loop end.
- **DP variant A:** sequential loop over rows; for each row a `parallel for` over the columns. The implicit
  barrier at the end of each `parallel for` guarantees row `y` is complete before row `y+1`.
- **DP variant B:** one `#pragma omp parallel` region; all threads run the row loop (row pointers declared
  inside → private); `#pragma omp for` over columns WITHOUT `nowait` → one barrier per row.
- **Minimum endpoint:** each thread keeps a private `(value, column)` best; `omp for ... nowait` (safe: the
  region-end barrier follows); result stored in its own `alignas(64)` slot (no false sharing); sequential
  combine with lexicographic `(value, column)` comparison → same column as the sequential version for any
  thread count and schedule.
- **Backtrack, outer seam loop, DP row 0, combine:** sequential (dependencies / tiny work).
- **Switched-off stages** use the same row/cell helper functions without any directive.
- **Default schedule:** with `OMP_SCHEDULE` unset, libgomp's run-time schedule is `dynamic,1` (observed in
  the CSV output). The CLI therefore sets `static` unless `OMP_SCHEDULE` is set.
- No `critical`, `atomic` or locks anywhere.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| include/seam_omp.h | Created | OpenMP API |
| src/seam_omp.cpp | Created | OpenMP implementation |
| tests/test_equivalence.cpp | Created | seq-vs-omp equivalence tests |
| CMakeLists.txt | Modified | OpenMP library, new tests |
| src/main.cpp | Modified | omp mode, stages, threads, CSV fields |
| docs/parallelization_strategy.md | Modified | §11 implementation notes + measured overhead; old §11 renumbered to §12 |
| docs/architecture.md | Modified | final omp signatures, `--stages none`, default schedule |
| README.md | Modified | M2 status, OpenMP usage |
| WORKLOG.md | Modified | This entry |

`src/seam_seq.cpp` and `include/seam_seq.h` were not changed in M2.

#### 5. Commands Executed

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\test_equivalence.exe
ctest --test-dir build --output-on-failure
# timing: OMP_SCHEDULE=static / unset, threads 1..20, stages energy|removal|dp|dp+persistent|none|all
.\build\seam_carving.exe --input tests\data\boat_sea.png --output out\t_omp.png --width 640 --mode omp --stages <s> --threads <t> --csv
# scratch micro-benchmark (outside repo): g++ -O2 -fopenmp ompcost.cpp; OMP_WAIT_POLICY passive/active
# mutation check: "omp for schedule(runtime)" -> "... nowait" in DP variant B, run test_equivalence, restore
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build (`-Wall -Wextra`) | No warnings | No warnings | Passed |
| `test_equivalence` | 0 failed | 147 368 checks, 0 failed (run twice; about 3–5 min per run) | Passed |
| `ctest` (10 tests) | All pass | 10/10 passed | Passed |
| Mutation: `nowait` on DP variant B | Tests fail | Immediate failures "M differs: random 47x61, dp(B), 2 threads, static, seam 0" and many more | Passed (race detected) |
| CLI omp runs (18 configurations, boat_sea → 640) | Output identical to seq | SHA-256 identical in all 18 runs | Passed |

Lockstep covers: 3 inputs (random 47×61, uniform 30×40, 200×160 crop of boat_sea) × 8 stage variants
(none, energy, DP variant A, DP variant B, min, removal, all stages with DP variant A, all stages with DP variant B) × threads {1,2,4,8,20} × schedules {static, static:1,
dynamic, guided} × 3 repetitions × 20 seams; at every seam energy, `M`, endpoint, seam and image are compared exactly.

**Informal timing** (ONE run per configuration, not a benchmark; i9-13900H, Windows 11, AC power,
"Balanced" power plan, GCC 16.2.0 `-O2`, `OMP_SCHEDULE` unset → static; boat_sea 800×600 → 640×600,
160 seams; seconds):

| Configuration | t_energy | t_dp | t_min | t_removal | t_total |
| ------------- | -------- | ---- | ----- | --------- | ------- |
| seq | 0.086 | 0.056 | 0.00026 | 0.0025 | 0.149 |
| omp, none, 8 thr | 0.088 | 0.058 | 0.00026 | 0.0032 | 0.152 |
| omp, energy, 2 / 4 / 8 / 20 thr | 0.055 / 0.043 / 0.042 / 0.041 | – | – | – | 0.124 / 0.124 / 0.117 / 0.114 |
| omp, removal, 2 / 8 / 20 thr | – | – | – | 0.014 / 0.019 / 0.026 | 0.191 / 0.206 / 0.183 |
| omp, DP variant A, 2 / 8 thr | – | 2.89 / 5.20 | – | – | 2.99 / 5.30 |
| omp, DP variant B, 2 / 8 thr | – | 1.79 / 5.05 | – | – | 1.88 / 5.15 |
| omp, energy+min+removal, 8 thr | 0.042 | – | 0.017 | 0.012 | 0.150 |
| omp, all stages (DP variant A), 8 thr | 0.073 | 10.24 | 0.018 | 0.020 | 10.36 |

Earlier run with `OMP_SCHEDULE` unset (libgomp default `dynamic,1`, before the static default was added),
boat_sea → 760 (40 seams): `t_dp` 0.51 s (1 thread), 1.48 s (4), 3.16 s (20) vs 0.016 s sequential.

OpenMP overhead micro-benchmark: see `docs/parallelization_strategy.md` §11.4 (region 8–261 µs,
barrier 15–213 µs for 1–20 threads; `OMP_WAIT_POLICY=active` no effect).

#### 7. Problems Encountered

- **Parallel DP is 30–90× slower than sequential DP here.** Cause (measured): every OpenMP barrier /
  region costs tens to hundreds of µs with this libgomp build on Windows, while one DP row has only a few µs
  of work. One barrier per row × 599 rows × 160 seams ≈ 96 000 synchronizations. Exact cause inside libgomp
  not verified (hypothesis: OS-level synchronization via winpthreads instead of Linux futexes).
- Parallel removal and minimum are also slower than sequential (work per seam is only a few µs).
- Only the energy stage gains (about 2.1× with 4–20 threads in one run).
- libgomp default schedule `dynamic,1` → fixed by the CLI's static default.
- Because of the synchronization cost, the full-carve test uses only 12 + 6 seams per run (lockstep still
  covers 20 seams per configuration).

#### 8. Decisions and Reasoning

- Static schedule as CLI default when `OMP_SCHEDULE` is unset (libgomp `dynamic,1` default is pathological for DP).
- Kept both DP variants and all stage switches although the DP is slow: correctness first (M2.md), the
  performance study belongs to M4/M5; the slowdown is a real, documented result.
- Per-stage parallel functions take `ParallelConfig` with a default argument, so tests can call them alone.

#### 9. Remaining Tasks

- M3 (correctness validation), M4 (benchmarking: analyse the synchronization overhead),
  M5 (optimization: parallel region management, synchronization overhead) – as defined in M3.md–M5.md.
- Commit/push of M0–M2; Members B/C agreement on the contract not yet recorded.

#### 10. Next Step

Commit and push M0–M2, then hand over to Member B (M3) and Member C (M4/M5).

### [2026-11-18] – M3/M4 planning and reproduction of M0–M2 tests on the M3/M4 machine

**Milestone:** M3, M4 (planning)

**Status:** Planning completed; existing tests reproduced. No M3/M4 code written yet.

#### 1. Objective

The owner of M3 (correctness) and M4 (benchmarking) reviewed M0–M2, planned M3/M4 and checked that the
existing build and tests work on the machine that will be used for M3/M4.

#### 2. Work Performed

- Read `M3.md`, `M4.md`, `docs/benchmark_plan.md`, `CMakeLists.txt`, `include/seam_omp.h`, test function list.
- Built the project (Release) and ran `ctest` on this machine.

#### 3. Technical Explanation

M3/M4 machine (a **different machine** from the one used in M0–M2):

| Item | Value |
| ---- | ----- |
| CPU | Intel Core i7-1065G7 @ 1.30 GHz, 4 cores, 8 logical processors (not hybrid) |
| RAM | 31.8 GB |
| OS | Windows 11 Home Single Language 10.0.22631 |
| Compiler / libraries | GCC 16.2.0 (MSYS2 UCRT64), CMake 4.5.0, Ninja 1.13.2, OpenCV 5.0.0, OpenMP 5.2 (`_OPENMP = 202111`) – identical to the team contract |

Consequences for M4: thread counts 1, 2, 4, 8 (16 > 8 logical processors is not supported; may only be
shown as an oversubscription experiment). All M4 results must come from this one machine; M2's informal
timings (i9-13900H) are not comparable.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| WORKLOG.md | Modified | This entry, Current Development Summary |

No source files changed.

#### 5. Commands Executed

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build (Release, `-Wall -Wextra`) | No errors, no warnings | No errors, no warnings | Passed |
| `ctest` | 10/10 pass | 10/10 passed, total 254.17 s | Passed |
| `test_seq` | 0 failed | 65 checks, 0 failed (0.34 s) | Passed |
| `test_equivalence` | 0 failed | 147 368 checks, 0 failed (252.02 s) | Passed |

Same check counts as on the M2 machine, so the M2 results are reproduced. `test_equivalence` uses up to
20 threads; on this 8-logical-processor CPU that is oversubscription (more threads than hardware threads)
and the results are still bit-exact.

#### 7. Problems Encountered

- None.

#### 8. Decisions and Reasoning

Plan (agreed with the user, details may change):

- **M3:** coverage review of existing tests vs the 10 M3 requirements; new tests for gaps (very narrow
  images, threads > columns, 0 seams, target width 1, synthetic patterns, large images, odd thread counts,
  oversubscription, height carving in omp, many repetitions); sanitizer builds on Linux/WSL (ASan/UBSan,
  TSan/Valgrind where usable, because MinGW has no sanitizers); mutation checks; split quick/full tests;
  `docs/testing_report.md`.
- **M4:** scripts for benchmark images, a runner (warm-up + ≥5 runs, randomized order, output hash check,
  environment metadata) and analysis/plots; pilot run first to estimate total time (parallel DP is very
  slow, see M2); then main grid, stage study, schedule study, DP A vs B; `docs/benchmark_report.md`.

#### 9. Remaining Tasks

- Execute M3 and M4 plans.

#### 10. Next Step

M3 step 1–2: code review of `seam_seq.cpp` / `seam_omp.cpp` and test-coverage gap analysis.

### [2026-11-18] – M3 step 1–2: code review and test-coverage analysis

**Milestone:** M3

**Status:** Completed (steps 1–2 of the M3 plan). M3 as a whole: in progress.

#### 1. Objective

Before writing new tests: read both implementations to look for race conditions and out-of-range
accesses, and compare the existing tests with the 10 requirements of `M3.md` to find gaps.

#### 2. Work Performed

- Reviewed every parallel loop in `src/seam_omp.cpp` and the matching code in `src/seam_seq.cpp`.
- Reviewed `tests/test_seq.cpp` and `tests/test_equivalence.cpp` (all test functions).
- Created `docs/testing_report.md` with environment, review table, coverage table and planned tests.
- Created branch `m3-testing` (team rule: one branch per milestone).

#### 3. Technical Explanation

- **Review method:** for each `#pragma omp` loop: shared vs private data, who writes which memory,
  where the barrier is. Result: every parallel loop writes disjoint memory (one row or one cell per
  iteration, one result slot per thread) and every read of another thread's data happens after a
  barrier. **No race or out-of-range access found by reading.**
- **Overflow bound:** `M <= 510 · rows`; with height reduction the image is transposed, so rows can be the
  original width: 7680 → `M <= 3.9 M`, far below the int32 limit.
- **Main gaps:** no large images (1920×1080, 3840×2160); omp never tested on edge-case images
  (1 column / 1 row / 1×1 / narrower than the thread count); no odd thread counts; seam connectivity not
  checked for every seam; no tool-based race/memory check (no sanitizers in MinGW).

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/testing_report.md | Created | M3 report (sections 1–4; results pending) |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
git switch -c m3-testing
git commit -m "Record M3/M4 plan and test reproduction on M3/M4 machine"   # 55e06af
```

#### 6. Testing and Results

No new tests run in this step (review only). Baseline from the previous entry: `ctest` 10/10.

#### 7. Problems Encountered

- None. Note: `omp_impl::carve()` changes the global thread count (`omp_set_num_threads`); not a bug, but
  tests must set the thread count explicitly.

#### 8. Decisions and Reasoning

- New tests go into a new file `tests/test_m3.cpp`; the existing test files (written by Member A) stay
  unchanged, so their results remain comparable with M2.
- Sanitizers will be run on Linux (WSL), since MinGW GCC supports neither ASan nor TSan.

#### 9. Remaining Tasks

- M3 steps 3–6: implement `tests/test_m3.cpp`, sanitizer runs, mutation checks, fast/full test split,
  finish `docs/testing_report.md`.

#### 10. Next Step

Implement `tests/test_m3.cpp` (edge cases for omp, forced seam positions, maximum contrast, brute force on
non-square shapes, connectivity of every seam, large images, odd thread counts).

### [2026-11-19] – M3 steps 3–6: new tests, mutation check, sanitizers, testing report

**Milestone:** M3

**Status:** Completed. No defect found in the implementation; no source file in `src/` or `include/` changed.

#### 1. Objective

Close the coverage gaps found in steps 1–2, prove that the tests can find real bugs, check for memory errors
and data races with tools, and write `docs/testing_report.md`.

#### 2. Work Performed

- `tests/test_m3.cpp`: independent reference implementation + 9 test groups (edge-case images, forced border
  seams, patterns / maximum energy 510, brute force on non-square shapes, 100-seam connectivity run, large
  images 1920×1080 / 3840×2160, odd thread counts / oversubscription / threads > columns or rows, 50× repeated
  runs, real images −20 % width −10 % height). Modes `--quick` and full.
- `CMakeLists.txt`: target `test_m3`, CTest entries `test_m3_quick`, `test_m3_full`, labels `quick` / `full`.
- `scripts/mutation_check.py`: 8 mutants built in `build/mutants/` (real sources untouched).
- `scripts/run_sanitizers.sh` (WSL): ASan+UBSan (g++), TSan+Archer (clang/libomp), memcheck mode (not run).
- `tests/tsan.supp`: TSan suppressions for uninstrumented libomp / libtbb internals.
- `docs/testing_report.md` completed; `README.md` test section and status table updated.
- Environment for sanitizers: WSL Ubuntu 26.04 with g++ 15.2.0, clang 21.1.8, OpenCV 4.10.0, valgrind 3.26.0.

#### 3. Technical Explanation

- **Why an independent reference:** all earlier equivalence tests compare omp with seq; a mistake present in
  both cannot be seen that way. The reference uses `at<>()`, 64-bit sums, `(value, column)` pair comparison
  and copy-based removal – a different way to write the same contract.
- **Lockstep:** seq, omp and reference run seam by seam; after every stage everything is compared exactly and
  each seam is checked for connectivity. Integer data → no tolerance.
- **Mutation testing idea:** if a test suite passes on correct code and fails on every inserted bug, it is
  able to detect that kind of bug.
- **ThreadSanitizer + Archer:** TSan records which thread touched which memory and reports two accesses to the
  same address (one a write) that are not ordered by synchronization. Archer tells TSan where OpenMP
  barriers are. Libraries not compiled with TSan (libomp internals, libtbb used by OpenCV) produce false
  reports, which are suppressed with `called_from_lib:` rules only.
- **Positive control lesson:** a first suppression file with `race:libomp.so` silenced everything, including
  the real race of mutant M1 – because a `race:` rule matches any stack frame and every OpenMP worker is
  started by libomp. Found only because the mutant was tested with the same setup; rule removed.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| tests/test_m3.cpp | Created | M3 tests + reference implementation |
| tests/tsan.supp | Created | TSan suppressions (libomp/libtbb internals only) |
| CMakeLists.txt | Modified | `test_m3`, CTest labels quick/full |
| scripts/mutation_check.py | Created | Mutation check |
| scripts/run_sanitizers.sh | Created | ASan / TSan / memcheck runs in WSL |
| docs/testing_report.md | Modified | Sections 4–6 (tests, results, conclusions), environment |
| README.md | Modified | Test section, milestone table |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
cmake --build build
.\build\test_m3.exe --quick ; .\build\test_m3.exe
ctest --test-dir build -L quick ; ctest --test-dir build -N -L full
python scripts\mutation_check.py
wsl --manage Ubuntu --move E:\wsl\Ubuntu
wsl -u root -- apt-get install g++ cmake ninja-build libopencv-dev clang libomp-dev valgrind llvm-21
wsl -u root -- bash scripts/run_sanitizers.sh asan . /root/ttss-sanitizers
wsl -u root -- bash scripts/run_sanitizers.sh tsan . /root/ttss-sanitizers
wsl -u root -- bash scripts/run_sanitizers.sh tsan-quick build/mutants/M1 /root/ttss-sanitizers
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build with `test_m3` (`-Wall -Wextra`) | No warnings | No warnings | Passed |
| `test_m3 --quick` (Windows) | 0 failed | 6 801 checks, 0 failed, 15 s | Passed |
| `test_m3` full (Windows) | 0 failed | 71 580 checks, 0 failed, 245 s (machine busy) | Passed |
| `ctest -L quick` | 10/10 | 10/10, 31 s | Passed |
| Mutation check (8 mutants) | all detected | 8/8 detected by `test_m3 --quick`; M8 (same bug in seq and omp) NOT detected by `test_equivalence` (0/147 368) | Passed |
| ASan+UBSan: test_seq / test_m3 / test_equivalence | no report, 0 failed | 65 / 71 580 / 147 368 checks, 0 failed, no sanitizer report | Passed |
| TSan, no suppressions, test_m3 --quick | – | 4 reports, all inside libomp / libtbb (false positives), 0 failed | Analysed |
| TSan positive control, mutant M1 | race reported in seam_omp.cpp | 83 reports, all in `seam_omp.cpp` (`dpCell`, line 39) | Passed |
| TSan (final suppressions), test_m3 --quick / full | 0 reports | 0 / 0 reports; 6 801 / 71 580 checks, 0 failed | Passed |

#### 7. Problems Encountered

- TSan false positives from uninstrumented libraries; first suppression attempt too broad (see section 3).
- Unsuppressed TSan `test_equivalence` was still running after > 40 min → stopped; not repeated (test_m3
  covers the same configurations). ASan `test_equivalence` took 2 650 s because it ran concurrently with TSan.
- Editing `run_sanitizers.sh` while the ASan run used it gave a harmless bash "unexpected EOF" after the last
  test had finished (all results were already written).
- Valgrind memcheck mode exists in the script but was not run (ASan already covers memory errors).

#### 8. Decisions and Reasoning

- Do not modify `test_seq.cpp` / `test_equivalence.cpp` (Member A's), so their M2 results stay comparable.
- TSan suppressions limited to `called_from_lib:` and validated by a positive control.
- No change to `src/` because no defect was found (M3.md: do not change the algorithm to make tests pass).

#### 9. Remaining Tasks

- M4 (benchmarking).

#### 10. Next Step

M4 pilot run on an idle machine.

### [2026-11-19] – M4 preparation: benchmark scripts, images, pilot runs

**Milestone:** M4

**Status:** In progress. Scripts done and tested; pilot done; main benchmark not yet run.

#### 1. Objective

Build a reproducible benchmark system (M4.md) and estimate how long the real experiments take.

#### 2. Work Performed

- `scripts/make_bench_images.py`: benchmark images in `bench_images/` (ignored by git) + `manifest.json`.
- `scripts/run_bench.py`: experiments `pilot`, `pilot_threads`, `main`, `main_nodp`, `stages`, `schedule`,
  `content`; raw CSV rows in `results/`, metadata in `results/<exp>.meta.json`.
- `scripts/analyze_bench.py`: medians, min/max, IQR, speedup, efficiency, Markdown tables, plots
  (`docs/figures/`); tested on a small real run written to a scratch folder (not kept).
- `scripts/run_all_bench.py`: the overnight plan chosen by the user.
- Pilot runs: `results/pilot.csv` (12 runs), `results/pilot_threads.csv` (6 runs).

#### 3. Technical Explanation

- **Images:** the full-resolution Unsplash photos are not on this machine and Unsplash blocks automated
  downloads (HTTP 401 / bot check), so the 800-px PNGs in `tests/data/` are the sources: main series from
  `person_desert` (512² scale 0.96; 1920×1080 ×2.4; 3840×2160 ×4.8; 7680×4320 ×9.6 upscaled), content
  series 1920×1080 from the other photos + seeded noise. Run time per seam is O(W·H) with no data-dependent
  loops, so content should matter little; the `content` experiment checks this.
- **Runner:** round 0 = warm-up (excluded), then measured rounds; configurations shuffled inside each round
  (seed 2026); every output image hashed and compared with the seq output; resumable.
- **Pilot (1 run each, machine idle, power plan "HP Recommended", AC; planning data only):**

| Size, −10 % | seq t_total | omp 4 thr, all stages | omp 4 thr, DP sequential |
| ----------- | ----------- | --------------------- | ------------------------ |
| 512×512 | 0.036 s | 1.44 s | 0.041 s |
| 1920×1080 | 1.02 s | 10.95 s | 0.86 s |
| 3840×2160 | 9.15 s | 45.1 s | 5.91 s |
| 7680×4320 | 71.2 s | 193.0 s | 41.8 s |

  All 12 outputs had the same hash as seq. Parallel DP synchronization cost per image row (from `t_dp`,
  1920×1080): 1 thread ≈ 12 µs, 2 ≈ 40 µs, 4 ≈ 51 µs, 8 ≈ 82 µs, 16 ≈ 118 µs, variant B with 16 ≈ 190 µs
  (sequential DP ≈ 2 µs of work per row).
- **Time budget (estimate from the pilot):** all experiments with 1 warm-up + 5 rounds ≈ 22 h. User chose
  the overnight variant (≈ 8–9 h): `main` (all stages) with 3 rounds, no warm-up, 7680×4320 only at −10 %;
  `schedule` with 3 rounds; the rest 1 + 5 rounds.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| scripts/make_bench_images.py | Created | Benchmark images |
| scripts/run_bench.py | Created | Benchmark runner |
| scripts/analyze_bench.py | Created | Analysis and plots |
| scripts/run_all_bench.py | Created | Overnight plan |
| results/pilot*.csv, *.meta.json | Created | Pilot data (planning only) |
| .gitignore | Modified | `bench_images/` |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
python scripts\make_bench_images.py
python scripts\run_bench.py pilot --rounds 1 --warmup 0
python scripts\run_bench.py pilot_threads --rounds 1 --warmup 0
python scripts\analyze_bench.py <scratch csv files> --out <scratch folder>   # script test
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Runner smoke test (512², 3 configs) | CSV rows, hash_ok 1 | 3 rows, hash_ok 1 | Passed |
| Pilot outputs vs seq | identical hash | 12/12 identical | Passed |
| Analysis script on small real data | tables + plots | tables + 8 plots; fixed a crash in the schedule plot (empty panel) | Passed after fix |

#### 7. Problems Encountered

- Unsplash blocks automated downloads → upscaled 800-px sources (documented in the manifest and report).
- Parallel DP is 3–40× slower than sequential DP (pilot) → full plan would take ~22 h.

#### 8. Decisions and Reasoning

- Primary speedup series = `main_nodp` (energy, min, removal parallel; DP sequential) in addition to the
  required "combined" version with all stages (`main`), because the parallel DP alone makes the combined
  version slower than sequential; both are reported.
- Pilot data are never reported as results; `analyze_bench.py` ignores `results/pilot*.csv` by default.

#### 9. Remaining Tasks

- Run `scripts/run_all_bench.py` (user prepares the machine first), analyse, write `docs/benchmark_report.md`.

#### 10. Next Step

Main benchmark run.

---

# Technical Decision History

Record important architecture and algorithmic decisions here.

| Date | Decision | Reason | Alternatives |
| ---- | -------- | ------ | ------------ |
| 2026-11-16 | Toolchain: MSYS2 UCRT64 (GCC 16.2.0, CMake 4.5.0, Ninja 1.13.2, OpenCV 5.0.0, OpenMP 5.2) for all members | Modern C++17/OpenMP, one-command install, identical versions | Visual Studio + vcpkg (OpenMP 2.0 only, long OpenCV build); old MinGW GCC 6.3 (32-bit, incomplete C++17) |
| 2026-11-17 | Integer central-difference energy `|I(x+1)-I(x-1)| + |I(y+1)-I(y-1)|` on gray image, borders clamped to current width | Exact, deterministic, follows paper's `e1` | Sobel, float gradients (rounding differences) |
| 2026-11-17 | Gray image computed once, seams removed from color and gray | Less work, no repeated rounding | Re-convert gray after each seam |
| 2026-11-17 | Energy and `M` as int32 (`CV_32SC1`) | Bit-exact seq/omp comparison; no overflow (max ≈ 2.2 M) | float / double |
| 2026-11-17 | Tie rule "smallest column wins" (endpoint and backtrack) | Unique result independent of threads | First found by any thread (non-deterministic) |
| 2026-11-17 | Matrices allocated once; `width` variable tracks current width | No allocation inside the seam loop | Reallocate/crop every seam |
| 2026-11-17 | Same API in `seq` and `omp_impl`; all stages public | Stage-by-stage equivalence tests | Only `carve()` public |
| 2026-11-17 | `ParallelConfig` switches stages; `schedule(runtime)` + `OMP_SCHEDULE` | 5 required versions without duplicated code; schedule experiments without rebuild | Separate binaries per version |
| 2026-11-17 | DP: two variants (DP variant A: parallel for per row; DP variant B: persistent region) | M2.md asks to investigate region overhead | Only DP variant A |
| 2026-11-17 | Min endpoint: per-thread `(value, column)` + sequential combine | Deterministic tie-breaking | `reduction(min)` (no column); `declare reduction` |
| 2026-11-17 | Backtracking and outer seam loop stay sequential | True data dependencies | – |
| 2026-11-17 | Height reduction by transpose, after width | Reuse vertical-seam code | Separate horizontal implementation; paper's optimal order |
| 2026-11-17 | CLI + fixed 15-column CSV line (`:` / `+` inside fields) | Stable interface for M3/M4 scripts | Free-form output |
| 2026-11-17 | CLI uses `static` schedule when `OMP_SCHEDULE` is unset | libgomp default `dynamic,1` made DP extremely slow (measured) | Keep libgomp default; hard-code schedule |
| 2026-11-17 | `-O2` set explicitly for Release | Contract; CMake's GCC default is `-O3` | `-O3` |

# Known Issues

| Issue | Description | Priority | Status |
| ----- | ----------- | -------- | ------ |
| No ThreadSanitizer | TSan is not supported by MinGW on Windows; races must be detected by repeated seq-vs-omp comparison tests | Medium | Mitigated in M3: TSan + Archer and ASan run on Linux/WSL (`scripts/run_sanitizers.sh`); Windows build itself still has no sanitizer |
| TSan false positives | libomp / libtbb (OpenCV) are not built with TSan → false reports; `tests/tsan.supp` uses `called_from_lib:` only (a `race:libomp.so` rule hides real races) | Info | Handled (validated with mutant M1) |
| `python` resolves to MSYS2 Python | MSYS2 is first in PATH, so `python` has no Pillow/numpy/matplotlib. Call another Python (e.g. conda) by its full path | Low | Open (workaround known) |
| Hybrid CPU | i9-13900H has P- and E-cores; speedup beyond 6 threads will not be linear; affects M4 analysis | Info | Open |
| 8K benchmark image | Largest source photo is 6000x4000; 7680x4320 needs upscaling | Low | Open (to note in M4 report) |
| Expensive OpenMP synchronization | libgomp on Windows: region 8–261 µs, barrier 15–213 µs (1–20 threads, measured). Parallel DP (one barrier per row) is 30–90× slower than sequential DP | High | Open (to analyse in M4, address in M5) |
| libgomp default schedule | `schedule(runtime)` without `OMP_SCHEDULE` = `dynamic,1` | Medium | Fixed in CLI (static default) |

# Benchmark History

Record actual benchmark experiments.

| Date | Image Size | Version | Threads | Execution Time | Notes |
| ---- | ---------- | ------- | ------- | -------------- | ----- |
|      |            |         |         |                |       |

**Important:** Only record experimentally measured values. Never insert estimated or fabricated results.

# Learning Notes

Record important concepts learned during development.

Examples:

- Dynamic Programming dependencies.
- OpenMP parallel for.
- OpenMP reduction.
- Synchronization and barriers.
- Race conditions.
- Cache locality.
- Speedup and parallel efficiency.
- Amdahl's Law.

Notes so far (2026-11-17):

- **DP dependency:** `M(x,y)` needs `M(x-1..x+1, y-1)`. Rows must run in order; cells of one row are
  independent. So the DP is parallel only *inside* a row, with a barrier between rows.
- **Barrier:** a point where every thread waits for all others. `omp for` has an implicit barrier at its
  end; `nowait` removes it. Removing the DP barrier gave wrong `M` immediately in the tests (race condition).
- **Deterministic reduction:** comparing `(value, column)` pairs defines a total order, so the parallel
  minimum gives the same column as the sequential loop for any thread count or schedule.
- **False sharing:** per-thread result slots are `alignas(64)` so two threads never write the same cache line.
- **Overhead vs work:** a parallel region or barrier costs 8–260 µs here (measured); a DP row has only a few
  µs of work. Parallelism pays off only when the work between synchronizations is much larger than the
  synchronization cost (energy stage: one region per seam → faster; DP: one barrier per row → slower).
- **`schedule(runtime)`:** the schedule is taken from `OMP_SCHEDULE`; libgomp's default when it is unset is
  `dynamic,1` (one iteration at a time, high overhead for small loops).
- **Testing without ThreadSanitizer:** races are detected by comparing every intermediate result with the
  sequential version, many times, with many thread counts and schedules.

---

# Current Development Summary

This section must always reflect the latest project state.

**Completed:**

- Toolchain setup and smoke test (MSYS2 UCRT64, GCC 16.2.0, OpenCV 5.0.0, OpenMP 5.2).
- Test images in `tests/data/` and `images/SOURCES.md`.
- M0 documents: `docs/algorithm.md`, `docs/parallelization_strategy.md`, `docs/architecture.md`, `docs/benchmark_plan.md`.
- M1: sequential implementation, CLI, README, unit tests (65 checks pass).
- M2: OpenMP implementation (all stages, DP variants A/B), equivalence tests (147 368 checks pass).

- M0–M2 tests reproduced on the M3/M4 machine (i7-1065G7): build OK, `ctest` 10/10 (2026-11-18).
- M3: `tests/test_m3.cpp` (71 580 checks), mutation check 8/8, ASan + TSan clean, `docs/testing_report.md` (2026-11-19).

**In Progress:**

- M4 (owner: user of the i7-1065G7 machine): scripts `make_bench_images.py`, `run_bench.py`, `analyze_bench.py` written; benchmarks not yet run.

**Blocked:**

- None.

**Next Recommended Task:**

- M4 pilot run, then the main benchmark on an idle machine (AC power, Best performance). (Agreement of the other members on `docs/architecture.md` still not recorded.)
