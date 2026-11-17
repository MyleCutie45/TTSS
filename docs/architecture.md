# Architecture and Interface Contract

**Milestone:** M0 – Research and Architecture
**Status:** Draft – **awaiting review by all three team members**. After agreement, the contract
(sections 3–6) is frozen and recorded in the Technical Decision History of `WORKLOG.md`.
Later changes require agreement of the whole team.

Items marked *(proposal)* are our own design choices; they are not required by the milestone files `M0.md`–`M6.md`.

---

## 1. Goals of the Design

- **Sequential baseline is never touched by OpenMP.** It is the correctness reference for all later work.
- **Same API for sequential and OpenMP code**, so tests can compare every intermediate result
  (energy map, `M`, seam, image), not only the final picture.
- **Bit-exact results.** All intermediate data are integers and the tie rule is fixed, so the OpenMP
  version must produce exactly the same bytes as the sequential version.
- **Stable command-line and CSV format**, so the benchmark scripts (M4) and tests (M3) can be written
  in parallel with the implementation (M1, M2).
- Simple, readable C++17; no class hierarchies or design patterns.

---

## 2. Directory Structure *(proposal)*

```
CMakeLists.txt              build definition (M1)
README.md                   build/run instructions (created in M1; READ.md is kept unchanged)
include/
  seam_common.h             shared types: StageTimings, ParallelConfig, helpers
  seam_seq.h                sequential API   (namespace seq)
  seam_omp.h                OpenMP API       (namespace omp_impl)
src/
  seam_seq.cpp              sequential implementation – contains NO "#pragma omp"
  seam_omp.cpp              OpenMP implementation
  main.cpp                  command-line program "seam_carving"
tests/
  test_seq.cpp              M1 unit tests (extended in M3)
  test_equivalence.cpp      M2 seq-vs-omp tests (extended in M3)
  data/                     small lossless PNG test images (see images/SOURCES.md)
scripts/                    M4: Python benchmark runner and plotting
results/                    M4: benchmark CSV files and figures
docs/
  algorithm.md
  architecture.md
  parallelization_strategy.md
  benchmark_plan.md
  testing_report.md         M3
  benchmark_report.md       M4
  optimization_report.md    M5
images/
  SOURCES.md                image credits
  original/                 full-size photos (not committed)
```

Build products go to `build/` (ignored by git).

---

## 3. Data Representation (contract)

| Data | Type | Notes |
| ---- | ---- | ----- |
| Color image | `cv::Mat`, `CV_8UC3` (BGR, as loaded by `cv::imread`) | Allocated once at original size |
| Gray image | `cv::Mat`, `CV_8UC1` | `cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY)` **once** at the start |
| Energy map | `cv::Mat`, `CV_32SC1` (int32) | Allocated once |
| Cumulative matrix `M` | `cv::Mat`, `CV_32SC1` (int32) | Allocated once |
| Seam | `std::vector<int>` of length `H` | `seam[y]` = removed column in row `y` |
| Current width | `int width` | Columns `>= width` are garbage and must never be read |

**Shrinking without reallocating.** The matrices keep their original size during carving. A variable
`width` holds the current width; after each seam `width` decreases by one. Only columns `0..width-1`
are valid. At the end the result is `image.colRange(0, targetWidth).clone()`.

> **Common bug:** using `mat.cols` instead of `width` (e.g. for clamping at the right border).
> `mat.cols` is the *original* width.

Row access uses `mat.ptr<T>(y)` (rows of a `cv::Mat` are contiguous in memory).

**Algorithm definition** (energy formula, clamping, DP, tie rule) is fixed in `algorithm.md`,
sections 4 and 6 (decisions D1–D7). Every implementation in this project must follow it exactly.

---

## 4. Functions (contract)

Both `namespace seq` (in `seam_seq.h`) and `namespace omp_impl` (in `seam_omp.h`) provide functions
with the same names and meanings. Every function is public so that tests can call each stage separately.

```cpp
// Stage 1: energy of columns 0..width-1 of every row. energy must be CV_32SC1 with gray's size.
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy);

// Stage 2: cumulative minimum energy M (CV_32SC1), columns 0..width-1.
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M);

// Stage 3: column of the minimum in the last row of M; ties -> smallest column.
int findMinEndpoint(const cv::Mat& M, int width);

// Stage 4: reconstruct the seam from endCol upwards; ties -> smallest column. seam.size() == rows.
void backtrackSeam(const cv::Mat& M, int width, int endCol, std::vector<int>& seam);

// Stage 5: remove seam from both the color image and the gray image (columns shift left).
// After the call, the valid width is width - 1 (the caller decrements width).
void removeSeam(cv::Mat& image, cv::Mat& gray, int width, const std::vector<int>& seam);

// Whole algorithm. targetWidth <= input.cols, targetHeight <= input.rows, both >= 1.
// timings may be nullptr. File loading/saving is NOT part of carve().
cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight,
              StageTimings* timings);                                   // seq
cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight,
              const ParallelConfig& config, StageTimings* timings);     // omp_impl
```

In `omp_impl`, the stage functions with a parallel option take the configuration as an extra last
parameter, `const ParallelConfig& config = ParallelConfig()`, e.g. `computeCumulative(energy, width, M, config)`,
so a test can run one parallel stage alone (final signatures in `include/seam_omp.h`, M2). They use the
current OpenMP thread count; only `carve()` sets it (`omp_set_num_threads(config.threads)`).

`backtrackSeam` is identical in both namespaces (it stays sequential), but it is still provided in
`omp_impl` so that the API is symmetric.

---

## 5. Shared Types – `seam_common.h` (contract)

```cpp
struct StageTimings {          // seconds, summed over all seams
    double energy      = 0.0;
    double dp          = 0.0;
    double minEndpoint = 0.0;
    double backtrack   = 0.0;
    double removal     = 0.0;
    double total       = 0.0;  // whole carve(): includes gray conversion, transposes, final crop
};

struct ParallelConfig {
    int  threads      = 1;
    bool parEnergy    = true;
    bool parDP        = true;
    bool persistentDP = false; // false: variant A (parallel for per row); true: variant B (one region)
    bool parMin       = true;
    bool parRemoval   = true;
};
```

- Timing uses `std::chrono::steady_clock`. Image loading and saving are **excluded**.
- `StageTimings` exists from M1 on, so M4 does not have to modify the sequential code.
- Measuring each stage separately adds a little overhead (two clock reads per stage per seam);
  it is the same for seq and omp, so comparisons stay fair.
- Schedule is **not** in `ParallelConfig`: all parallel loops use `schedule(runtime)` and the schedule
  is set with the environment variable `OMP_SCHEDULE` (e.g. `static`, `dynamic,64`, `guided`).
  If `OMP_SCHEDULE` is not set, the CLI selects `static` (added in M2: libgomp's own default is
  `dynamic,1`, which is very slow here; see `parallelization_strategy.md` §11.2).

---

## 6. Command-Line Interface (contract)

Program name: `seam_carving`.

```
seam_carving --input <in.png> --output <out.png> --width <W> [--height <H>]
             [--mode seq|omp] [--threads <N>] [--stages <list>] [--persistent-dp] [--csv]
```

| Option | Meaning | Default |
| ------ | ------- | ------- |
| `--input` | Input image path | required |
| `--output` | Output image path | required |
| `--width` | Target width, `1 <= W <= input width` | required |
| `--height` | Target height, `1 <= H <= input height` | input height (no change) |
| `--mode` | `seq` or `omp` | `seq` |
| `--threads` | Thread count (omp only) | `omp_get_max_threads()` |
| `--stages` | Comma-separated subset of `energy,dp,min,removal` to parallelize, or `none` (omp only) | all four |
| `--persistent-dp` | Use DP variant B (persistent region) | off (variant A) |
| `--csv` | Print exactly one machine-readable line to stdout | off (human-readable output) |

**Errors** *(proposal)*: a clear message on stderr and a non-zero exit code:
`1` = invalid arguments (missing option, target larger than input or `<= 0`, unknown stage),
`2` = cannot read input or write output.

### CSV line

With `--csv`, the program prints exactly one line, no header, with these columns in this order:

```
mode,threads,schedule,stages,in_w,in_h,out_w,out_h,seams,t_energy,t_dp,t_min,t_backtrack,t_removal,t_total
```

| Column | Content |
| ------ | ------- |
| `mode` | `seq` or `omp` |
| `threads` | `1` for seq; thread count for omp |
| `schedule` | Value from `omp_get_schedule`, e.g. `static`, `dynamic:64`; `none` for seq |
| `stages` | Parallel stages joined by `+`, e.g. `energy+dp+min+removal`; `dp` is written `dp_persistent` for variant B; `none` for seq |
| `in_w,in_h,out_w,out_h` | Image sizes in pixels |
| `seams` | Total number of seams removed (vertical + horizontal) |
| `t_*` | `StageTimings` fields in seconds |

*(proposal)* Commas inside a column are replaced by `:` (schedule) or `+` (stages) so that the line
stays valid CSV. The header string above is provided by the scripts in M4.

Example (format only, the numbers are not measurements):

```
omp,8,static,energy+dp+min+removal,800,600,640,600,160,0.0,0.0,0.0,0.0,0.0,0.0
```

---

## 7. Data Flow

```
            main.cpp                       carve()  (seq or omp_impl)
 imread ──> color (8UC3) ──────────────> gray = cvtColor(color)      (once)
                                          width = cols
                                          loop k times:
                                            computeEnergy(gray)      -> energy
                                            computeCumulative(energy)-> M
                                            findMinEndpoint(M)       -> endCol
                                            backtrackSeam(M, endCol) -> seam
                                            removeSeam(color, gray, seam)
                                            width--
                                          [height: transpose, repeat, transpose back]
 imwrite <── result <──────────────────── color.colRange(0, targetWidth).clone()
 print timings / CSV line
```

Memory is allocated once per `carve()` call (color copy, gray, energy, `M`, seam). Nothing is
allocated inside the seam loop.

---

## 8. Build System (contract)

- CMake ≥ 3.16, generator Ninja, C++17 (`CMAKE_CXX_STANDARD 17`, no compiler extensions).
- `find_package(OpenCV REQUIRED)` and `find_package(OpenMP REQUIRED)`; link `OpenMP::OpenMP_CXX`
  only to targets that need it *(proposal: the sequential library is built without OpenMP flags, which
  proves it contains no OpenMP code)*.
- Flags: `Release` build, `-O2 -Wall -Wextra`. The exact flags are recorded in `README.md` (needed for M4).
- Commands:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

- Tests are small executables using `assert`-style checks, registered with `add_test` (no external
  test framework needed).
- Toolchain used by the team: MSYS2 UCRT64, GCC 16.2.0, CMake 4.5.0, Ninja 1.13.2, OpenCV 5.0.0,
  OpenMP 5.2 (see `WORKLOG.md`).

---

## 9. Team Workflow *(proposal)*

| Member | Milestones | Depends on |
| ------ | ---------- | ---------- |
| A | M0, M1, M2 | – |
| B | M3 (testing), leads M6 | contract (now), tag `v0.1-m1`, tag `v0.2-m2` |
| C | M4 (benchmark), M5 (optimization) | contract (now), CSV format, tag `v0.2-m2` |
| All | M6 (report, presentation) | everything |

- One branch per milestone (`m1-sequential`, `m2-openmp`, ...), merged into `main` by pull request.
- Tags `v0.1-m1` and `v0.2-m2` mark stable versions that B and C can build on.
- Never `git add .`; add files explicitly. Full-size images and build outputs are not committed.
- Every member records their work in `WORKLOG.md` (append only).

### What B and C can start before M1/M2 are finished

- **B:** an independent Python/NumPy reference implementation of the contract (energy, `M`, seam, tie
  rule) for cross-checking; deterministic test image generators (uniform, gradient, stripes,
  checkerboard, seeded noise, odd sizes such as 1 x 1, 2 x N, N x 1); a seam validity checker.
- **C:** the benchmark runner (calls `seam_carving --csv` via `subprocess`, repeats, computes median),
  plotting scripts, preparing benchmark images, collecting hardware information. Any dummy data used
  for testing scripts must be marked as fake and never appear in a report.
