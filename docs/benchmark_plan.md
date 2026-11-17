# Benchmark Plan (preliminary)

**Milestone:** M0 – Research and Architecture (plan). M4 owner completes and executes it.
**Status:** Draft for team review (2026-11-17)

**No benchmark has been run yet. This document contains no measured results.** Every number below is
a configuration value or a calculation, never a measurement.

---

## 1. Questions the Benchmarks Must Answer

1. How much faster is the combined OpenMP version than the sequential baseline, for each image size and thread count?
2. Which stage benefits most from parallelization, and which stage limits the speedup?
3. Is the persistent-region DP (variant B) faster than one `parallel for` per row (variant A)?
4. Which OpenMP schedule (`static`, `dynamic`, `guided`) works best for each stage?
5. How do image size and the number of removed seams change the speedup?

---

## 2. Metrics

| Metric | Definition |
| ------ | ---------- |
| Total time `T` | `t_total` from the CSV line: the whole `carve()` call, **excluding** image loading and saving |
| Stage times | `t_energy`, `t_dp`, `t_min`, `t_backtrack`, `t_removal` (summed over all seams) |
| Speedup | `S(p) = T_seq / T_omp(p)` |
| Efficiency | `E(p) = S(p) / p` |
| Stage speedup | Same formula per stage, e.g. `t_dp_seq / t_dp_omp(p)` |
| Sequential fraction | Estimated from stage times, used with Amdahl's law |

`T_seq` is the run time of `--mode seq` (the real sequential code), **not** `--mode omp --threads 1`.
The OpenMP version with 1 thread is measured separately: the difference shows the overhead of OpenMP itself.

---

## 3. Configurations (from `M4.md`)

| Parameter | Values |
| --------- | ------ |
| Image sizes | 512 x 512, 1920 x 1080, 3840 x 2160, 7680 x 4320 (if memory/time allows) |
| Width reduction | 10 %, 20 %, 30 % of the original width |
| Threads | 1, 2, 4, 8, 16 (machine has 20 logical processors; 20 may be added) |
| Versions | seq; omp with stages `energy`, `dp`, `dp` persistent, `removal`, all |
| Schedules | `static`, `dynamic`, `guided` (via `OMP_SCHEDULE`) |

The full cross product is large. Proposed order *(proposal)*:

1. Main grid: all sizes × all reductions × all thread counts, combined version, `static`.
2. Stage study: one size (1920 x 1080, 20 %), all stage configurations × thread counts.
3. Schedule study: one size, combined version, the three schedules × thread counts.
4. DP variant A vs B: all sizes, 8 and 16 threads.

**Number of seams** (calculation): 10/20/30 % of width 512 → 51/102/153 seams;
of 1920 → 192/384/576; of 3840 → 384/768/1152; of 7680 → 768/1536/2304.

**Expected long runs:** the 7680 x 4320, 30 % sequential run removes 2304 seams from a 33-megapixel
image and may take a long time. Measure one run first before planning repetitions for this size.

---

## 4. Benchmark Images

- Generated from the photos listed in `images/SOURCES.md` by resizing (and cropping to the exact
  aspect ratio) to the four target sizes, saved as PNG.
- The largest original photo is 6000 x 4000, so **7680 x 4320 requires upscaling**. Seam carving run time
  does not depend much on image content, but this must be stated in the report.
- Benchmark images are stored in a folder ignored by git (large files) and the generating script is committed,
  so anyone can recreate them.
- Optional extra: a synthetic image with seeded random noise for content-independent timing.

---

## 5. Methodology

| Rule | Reason |
| ---- | ------ |
| Time only `carve()` with `std::chrono::steady_clock`; no file I/O | Disk speed is not what we study |
| 1 warm-up run (discarded) + **at least 5** measured runs per configuration *(proposal)* | Warm-up loads code/data into caches and wakes up the CPU; repetitions show variability |
| Report **median**, plus min and max (or interquartile range) | Median is robust to occasional slow runs |
| Same binary, same build flags, same input for seq and omp | Fair comparison: identical work |
| Check that the omp output equals the seq output (file hash) for each configuration | A fast but wrong result is worthless |
| Run all configurations of one experiment in one session, on the same machine | Hardware conditions must not change between compared runs |
| Laptop on AC power, Windows power mode "Best performance", other programs closed | Battery mode and background load change CPU frequency |
| Record room/thermal conditions if long runs show slowdowns | Laptops reduce clock speed when hot (thermal throttling) |
| Randomize or interleave the order of configurations *(proposal)* | Avoid all runs of one configuration hitting the same thermal state |

**Initialization vs steady state.** OpenMP creates its thread pool at the first parallel region. The warm-up
run absorbs this. In addition, M4 can report the time of the first seam separately from the average
per-seam time of the later seams *(proposal)*.

---

## 6. Hardware Notes for This Machine

- CPU: Intel Core i9-13900H – **hybrid**: 6 performance cores (2 threads each = 12 logical) +
  8 efficiency cores (1 thread each) = 14 cores / 20 logical processors.
- Consequences:
  - Speedup above 6 threads will not be linear even in theory, because additional threads run on
    slower E-cores or share a P-core (Hyper-Threading).
  - With a barrier after every DP row, the slowest thread decides the speed of the row.
  - Windows decides which core runs each thread. Try `OMP_PLACES=cores` and `OMP_PROC_BIND=close/spread`
    and record the settings used for every experiment.
- RAM: about 15.6 GB. Memory needed for 7680 x 4320 (calculation): color 3 B + gray 1 B + energy 4 B +
  `M` 4 B = 12 B per pixel × 33.2 M pixels ≈ 0.4 GB, plus a copy of the input → fits.

Record for every result set: CPU model, cores/logical processors, RAM, OS version, compiler version and
flags, OpenMP version, `OMP_SCHEDULE`, `OMP_PLACES`, `OMP_PROC_BIND`, power mode, date.

---

## 7. Output Format

- The program prints one CSV line per run (format fixed in `architecture.md`, section 6).
- The runner script adds the columns `image,run` and writes one row per run to `results/*.csv`.
  Raw rows are kept; medians are computed by the analysis script, not stored instead of the raw data.

---

## 8. Plots (from `M4.md`)

1. Execution time vs thread count (one line per image size).
2. Speedup vs thread count (with the ideal line `S = p`).
3. Efficiency vs thread count.
4. Run-time breakdown by stage (stacked bars: seq and omp with several thread counts).
5. Performance comparison across image sizes.

Additional *(proposal)*: DP variant A vs B; schedule comparison.

Scripts must be run with a Python that has numpy/pandas/matplotlib (e.g. conda, called by its full path),
because the `python` command on the MSYS2 PATH has no matplotlib/pandas.

---

## 9. Hypotheses to Check (not results)

- Energy stage scales best; removal flattens because of memory bandwidth.
- DP scales worst because of `H - 1` barriers per seam; variant B may reduce the overhead.
- Min endpoint may become *slower* when parallelized (very small work).
- Speedup grows with image size (more work per barrier/region).
- Speedup above 6–8 threads is limited by the hybrid P/E architecture.

Each hypothesis is confirmed or rejected in `docs/benchmark_report.md` using measured data only.
