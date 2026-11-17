# WORKLOG – Seam Carving OpenMP Project

## Project Information

**Project:** Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP

**Primary Language:** C++17

**Parallel Framework:** OpenMP

**Purpose:** Document all development activities, implementation decisions, experiments, bugs, and progress.

---

## Current Project Status

- Current milestone: M2 finished (Member A's part M0–M2 done and pushed to GitHub `main`)
- Overall status: sequential + OpenMP versions bit-exact equal; OpenMP synchronization overhead on Windows makes the parallel DP slow
- Last updated: 2026-11-17

## Milestone Progress

| Milestone | Description                   | Status      |
| --------- | ----------------------------- | ----------- |
| M0        | Research and Architecture     | Completed (contract approved by Member A; Members B/C agreement not yet recorded) |
| M1        | Sequential Implementation     | Completed |
| M2        | OpenMP Implementation         | Completed (correct; parallel DP slower than sequential on this platform) |
| M3        | Correctness Testing           | Not started |
| M4        | Performance Benchmarking      | Not started |
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
| No ThreadSanitizer | TSan is not supported by MinGW on Windows; races must be detected by repeated seq-vs-omp comparison tests | Medium | Open (accepted limitation) |
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

**In Progress:**

- None (Member A's work M0–M2 is done and pushed).

**Blocked:**

- None.

**Next Recommended Task:**

- Members B and C confirm the interface contract (`docs/architecture.md`), then start M3 (Member B) and M4/M5 (Member C).
