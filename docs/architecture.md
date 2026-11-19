# Architecture

Status: proposed in M0, to be confirmed before M1.

## 1. Toolchain (decision D1)

| Item | Choice | Reason |
| ---- | ------ | ------ |
| Environment | WSL2, Ubuntu 24.04 | Native Windows blocks our own unsigned executables (0xC0E90002, likely Smart App Control); apt provides OpenCV; sanitizers are available for M3 |
| Compiler | g++ (Ubuntu package), C++17, `-fopenmp` | Modern libgomp |
| Build | CMake ≥ 3.16, default `Release` (`-O2`) | Required by M0 |
| Libraries | OpenCV (`libopencv-dev`) for image I/O only | All computation happens on our own buffers |
| Analysis | Python 3 + pandas + matplotlib | M4 plots |

The repository should be cloned inside the Linux filesystem (e.g. `~/TTSS`), not under `/mnt/d`, to avoid slow cross-filesystem I/O. Exact versions are recorded in WORKLOG at setup time.

## 2. Directory layout

```text
TTSS/
├── CMakeLists.txt
├── README.md                    # build/run instructions (M1); READ.md is a separate file and stays unchanged
├── src/
│   ├── main.cpp                 # CLI parsing, load → carve → save, CSV output
│   ├── image_buffer.hpp/.cpp    # ImageBuffer, load/save via OpenCV, transpose
│   ├── seam_common.hpp          # shared inline rules: energyAt, dp tie-breaking, validSeam
│   ├── seam_seq.hpp/.cpp        # sequential stages (reference; frozen after M1)
│   ├── seam_omp.hpp/.cpp        # OpenMP stages (M2)
│   ├── carver.hpp/.cpp          # main loop, variant selection, per-phase timing
│   └── timer.hpp                # wall-clock helper (std::chrono::steady_clock)
├── tests/
│   ├── test_main.cpp            # minimal test runner (no external framework, decision D4)
│   └── oracle.hpp               # brute-force seam enumeration for tiny images
├── bench/
│   ├── gen_images.py            # deterministic synthetic test images
│   ├── run_bench.py             # runs the benchmark matrix, writes CSV
│   └── plot.py                  # M4 plots
├── data/                        # small test images (large generated images are git-ignored)
├── results/                     # raw benchmark CSV + environment file
└── docs/
```

## 3. Data structures

```cpp
struct ImageBuffer {
    int height = 0;              // H (rows)
    int width  = 0;              // logical width W, decreases by 1 per removed seam
    int stride = 0;              // allocated row length = original width, never changes
    std::vector<uint8_t> color;  // height * stride * 3, BGR interleaved
    std::vector<uint8_t> gray;   // height * stride
};
```

- Pixel (i, j) of `gray` is at `gray[i * stride + j]`; of `color`, channel c is at `color[(i * stride + j) * 3 + c]`.
- **Fixed stride** means no reallocation per seam. Rows never overlap in memory, so rows can be shifted in place by different threads without races.

Working arrays, allocated once by the carver and reused every iteration:

| Array | Type | Size |
| ----- | ---- | ---- |
| `energy` | `std::vector<int32_t>` | H × stride |
| `M` | `std::vector<int32_t>` | H × stride |
| `seam` | `std::vector<int>` | H |

## 4. Stage functions

Sequential and OpenMP versions have identical signatures and live in separate files. All functions write into caller-provided buffers, so tests can inspect every intermediate result (M3 requirement).

```cpp
// seam_seq.hpp
void computeEnergySeq    (const uint8_t* gray, int H, int W, int stride, int32_t* energy);
void computeCumulativeSeq(const int32_t* energy, int H, int W, int stride, int32_t* M);
int  findMinIndexSeq     (const int32_t* lastRow, int W);
void backtrackSeam       (const int32_t* M, int H, int W, int stride, int endCol, int* seam);
void removeSeamSeq       (ImageBuffer& img, const int* seam);   // does NOT change img.width

// seam_omp.hpp (M2)
void computeEnergyOmp    (const uint8_t* gray, int H, int W, int stride, int32_t* energy);
void computeCumulativeOmp(const int32_t* energy, int H, int W, int stride, int32_t* M, DpMode mode);
int  findMinIndexOmp     (const int32_t* lastRow, int W);
void removeSeamOmp       (ImageBuffer& img, const int* seam);
```

- Backtracking is inherently sequential (`s[i]` depends on `s[i+1]`), so there is one implementation used by all variants.
- `img.width` is decremented only by the carver, at exactly one place.
- `seam_common.hpp` contains `energyAt()`, the parent-selection rule and `validSeam()`. Sharing these is what guarantees identical results between seq and omp.

## 5. Variants (M0 §3 requirement)

```cpp
enum class DpMode { PerRowParallelFor, PersistentRegion };
struct Variant {
    bool ompEnergy = false, ompDp = false, ompMin = false, ompRemove = false;
    DpMode dpMode = DpMode::PerRowParallelFor;
};
```

| CLI name | ompEnergy | ompDp | ompMin | ompRemove |
| -------- | --------- | ----- | ------ | --------- |
| `seq` | – | – | – | – |
| `omp-energy` | ✓ | – | – | – |
| `omp-dp` | – | ✓ | – | – |
| `omp-remove` | – | – | – | ✓ |
| `omp-min` | – | – | ✓ | – |
| `omp` (combined) | ✓ | ✓ | ✓ | ✓ |

The carver calls the seq or omp function for each stage according to the flags (a simple `if`, no class hierarchy). The single-stage variants make it possible to measure each stage's speedup separately (M4 breakdown). M5 optimizations are added later as new flags or variants, keeping "original sequential / basic OMP / optimized OMP" side by side.

## 6. Carver

```cpp
struct PhaseTimes { double energy = 0, dp = 0, find = 0, remove = 0, total = 0; };  // seconds

void carveWidth(ImageBuffer& img, int targetWidth, const Variant& v,
                PhaseTimes* times,                       // may be nullptr
                std::vector<std::vector<int>>* seamLog); // may be nullptr; tests record every seam
void carve(ImageBuffer& img, int targetWidth, int targetHeight, const Variant& v, PhaseTimes* times);
```

`carve` reduces the width first, then transposes, reduces the (original) height, and transposes back. Transpose time is reported separately. "find" = argmin + backtrack.

## 7. Command-line interface

```text
seam_carving --input in.png --output out.png --width 1536 [--height 864]
             [--variant seq|omp-energy|omp-dp|omp-remove|omp-min|omp]
             [--dp-mode per-row|persistent]
             [--threads N]          # omp_set_num_threads(N); default: OpenMP default
             [--reps R]             # repeat the carve R times on fresh copies (benchmark)
             [--csv results.csv]    # append one row per repetition
             [--dump-seams file]    # write every seam (tests / debugging)
```

At startup the program prints the `_OPENMP` value and `omp_get_max_threads()`, so a build without OpenMP is detected immediately.

## 8. Data flow

```text
imread (BGR) ──► ImageBuffer.color ──► gray (once)
                                         │
          ┌──────────────────────────────┘
          ▼
   ┌─► computeEnergy ─► energy ─► computeCumulative ─► M ─► findMin ─► backtrack ─► seam
   │                                                                                 │
   └────────────── removeSeam(color, gray) ◄─────────────────────────────────────────┘
                   width -= 1   (repeat until targetWidth)
                                         │
                                         ▼
                              ImageBuffer.color ──► imwrite
```

## 9. Invariants (checked in debug/test builds)

1. After every iteration, `color` and `gray` have had the same seam removed, and `width` decreased by exactly 1.
2. Every seam satisfies `validSeam()`: in range and 8-connected.
3. `Σ e(i, s[i]) == M(H-1, s[H-1]) == min of last row`.
4. The sequential output for each variant is byte-identical to `seq`.

## 10. Testing layout (decision D4)

- A single executable `seam_tests` with a minimal runner (macros `CHECK(cond)` and `CHECK_EQ(a, b)`, counters, non-zero exit on failure), registered with CTest via `add_test`.
- Test inputs are generated in code (deterministic) or loaded from `data/`.
- See `benchmark_plan.md` §2 for the test list.
