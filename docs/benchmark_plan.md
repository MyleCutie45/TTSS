# Testing and Benchmark Plan

This document defines how correctness is verified (M1–M3) and how performance is measured (M4–M5). **No results are recorded here.** Results go to `docs/testing_report.md` and `docs/benchmark_report.md`, and only measured values are allowed.

---

## 1. Principles

1. The sequential implementation is the reference. OpenMP variants must match it **exactly** (integer data, see `parallelization_strategy.md` §5).
2. Every test input is deterministic: either generated with a fixed seed or a fixed file in `data/`.
3. Benchmarks measure only the core algorithm. Image loading and saving are excluded.
4. Every number in a report comes from a run that is recorded in `results/` together with the environment.

## 2. Correctness tests

### 2.1 Unit tests (M1)

| # | Test | Check |
| - | ---- | ----- |
| U1 | Energy on hand-made 3×3 / 4×5 images, including borders | exact values |
| U2 | DP on the worked example (`algorithm.md` §8) | M last row = [11, 10, 14, 19, 15] |
| U3 | Backtrack on the worked example | seam = [1, 1, 0, 1], cost 10 |
| U4 | DP vs brute-force oracle on random images H ≤ 7, W ≤ 6 (≥ 200 seeds) | min cost equal |
| U5 | Uniform image (all energies 0) | seam follows the tie rule: column 0 in every row |
| U6 | Bright vertical stripe | seam never touches the stripe |
| U7 | Seam removal | width − 1, the pixel at `s[i]` is gone, the rest is shifted correctly |
| U8 | Edge cases W = 1, 2, 3 and H = 1 | no crash; behaviour defined (W = 1 cannot shrink further) |
| U9 | Full carve | output dimensions, `validSeam` for every seam, exactly one pixel per row per iteration |
| U10 | Height reduction via transpose | dimensions; transpose ∘ transpose = identity |

### 2.2 Equivalence tests (M2/M3)

For every variant (`omp-energy`, `omp-dp` in both DP modes, `omp-min`, `omp-remove`, `omp`), every thread count {1, 2, 3, 4, 7, 8, 16} and every test image:

1. Compare the `energy` matrix (exact) on the first iteration and after a few seams.
2. Compare the full `M` matrix (exact).
3. Compare the seam of **every** iteration (record the first iteration and row that differ, if any).
4. Compare the final images byte by byte.
5. Repeat (3)–(4) at least 50 times per configuration to catch intermittent races.

### 2.3 Race and memory checks (M3, WSL)

- AddressSanitizer + UndefinedBehaviorSanitizer build (`-fsanitize=address,undefined`) on the full test suite.
- ThreadSanitizer build (`-fsanitize=thread`). libgomp may report false positives; any report is investigated and documented, not silently ignored.
- A code review checklist for every parallel region: shared vs private variables, who writes, who reads, which barrier separates them.

## 3. Test images

| Name | Size | Source |
| ---- | ---- | ------ |
| `syn_512` | 512×512 | `bench/gen_images.py`, fixed seed |
| `syn_1080p` | 1920×1080 | generated |
| `syn_4k` | 3840×2160 | generated |
| `syn_8k` | 7680×4320 | generated (git-ignored, regenerated on demand) |
| `photo_*` | various, rescaled to the sizes above | 2–3 public-domain / CC0 photos (to be selected; license recorded in `data/LICENSES.md`) |

Synthetic images contain smooth gradients, a few high-contrast shapes and mild seeded noise, so seams have a realistic structure (not only ties).

## 4. Benchmark matrix (M4)

| Factor | Levels |
| ------ | ------ |
| Image size | 512×512, 1920×1080, 3840×2160, 7680×4320 (if time allows) |
| Seams removed | 10 %, 20 %, 30 % of the original width |
| Variant | `seq`; `omp` (combined); `omp-energy`, `omp-dp`, `omp-remove`, `omp-min` for the per-stage breakdown |
| Threads | 1, 2, 4, 8, 16 (OMP variants) |
| DP mode | per-row, persistent |
| Schedule | `static` (default); `dynamic`, `guided` as a separate experiment on 1080p/4K |
| Repetitions | 5 per configuration (3 for 8K), after 1 warm-up run |

The full matrix may be too long (see `project_plan.md` §5). After M1 measures the sequential time:

- The full matrix runs on 512², 1080p and 4K.
- 8K runs `seq` and `omp` only, with fewer repetitions.

This reduction will be stated in the report.

## 5. Measurement method

- **Clock:** `std::chrono::steady_clock` (monotonic wall clock), in seconds as double.
- **Per-phase timing:** each stage call inside the seam loop is timed and accumulated into `PhaseTimes` (energy, dp, find = argmin + backtrack, remove). `total` is measured around the whole carve.
- The timer overhead (two clock reads per stage per seam) will be measured once and reported. If it is not negligible, `total` is taken from a run without per-phase timing.
- **Warm-up:** an empty `#pragma omp parallel` region plus one untimed carve before the timed repetitions. Thread creation and first-touch page faults are thus excluded from steady-state numbers. The warm-up time is recorded separately, to report initialization overhead (M4 §3).
- **Fresh input per repetition:** each repetition carves a fresh copy of the loaded image. Copying happens outside the timed region.
- **Statistics:** median over the repetitions (primary), plus min, max and standard deviation.
- **Speedup:** `S(p) = T_seq / T_omp(p)`, relative to the sequential reference.
- **Efficiency:** `E(p) = S(p) / p`.
- `T_omp(1) / T_seq` is also reported, because it shows the pure OpenMP overhead.

## 6. Environment record (one file per benchmark session)

`results/env_<date>.txt` contains:

- CPU model, physical cores, logical processors, cache sizes (`lscpu`)
- RAM
- OS and kernel; WSL version
- compiler and version (`g++ --version`)
- full compiler flags (from the CMake build)
- `_OPENMP` value
- OpenCV version
- `OMP_SCHEDULE`, `OMP_PROC_BIND`, `OMP_PLACES`
- git commit hash
- power state (plugged in, Windows power mode)
- date and time

## 7. Run conditions

- Laptop plugged in, Windows power mode set to "Best performance", other heavy applications closed.
- Configurations are run interleaved (round-robin over thread counts and variants), not all repetitions of one configuration in a row. This reduces bias from thermal throttling.
- Runs with visibly abnormal variability (e.g. max/min > 1.2) are re-run and the fact is noted.

## 8. Output format

`results/bench_<date>.csv`, one row per repetition:

```text
image,width,height,target_width,seams,variant,dp_mode,schedule,threads,rep,
energy_s,dp_s,find_s,remove_s,total_s,commit
```

## 9. Plots (`bench/plot.py`, M4 §4)

1. Execution time vs threads (one line per image size).
2. Speedup vs threads, with the ideal line.
3. Efficiency vs threads.
4. Runtime breakdown by phase (stacked bars per thread count).
5. Comparison across image sizes and seam percentages.

Every plot is generated from the CSV only. The script prints the number of repetitions behind every point.

## 10. M5 protocol

Each optimization:

1. is a new variant or flag, keeping the previous version;
2. passes the full equivalence suite (§2.2);
3. is benchmarked against the previous best on the same matrix subset, in the same session;
4. is kept only if the median improvement exceeds the observed run-to-run variability.
