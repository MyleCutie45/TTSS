# Parallelization Strategy

**Milestone:** M0 – Research and Architecture (design). M2 will add an "Implementation notes" section.
**Status:** Draft for team review (2026-11-17)
**Prerequisite:** `algorithm.md` (stages, formulas, tie rule).

This document answers, for every stage of the algorithm: *Can it run in parallel? Why? Which OpenMP
directive? Where must threads synchronize? What race conditions are possible?*

Everything here is a **design**. No code exists yet, and no performance numbers have been measured.

---

## 1. Short OpenMP Glossary

| Term | Meaning |
| ---- | ------- |
| Parallel region (`#pragma omp parallel`) | A block of code executed by a team of threads. Creating/waking the team has a cost (overhead). |
| Work-sharing loop (`#pragma omp for`) | Inside a parallel region: split the loop iterations among the threads. |
| `#pragma omp parallel for` | Shortcut: create a team **and** split the next loop. |
| Implicit barrier | At the end of `omp for` and of a parallel region, every thread waits until all threads arrive. |
| `nowait` | Removes the implicit barrier at the end of an `omp for`. |
| `schedule(static / dynamic / guided)` | How iterations are given to threads: fixed equal blocks / on demand in chunks / on demand with shrinking chunks. |
| `schedule(runtime)` | The schedule is read from the environment variable `OMP_SCHEDULE` at run time. |
| Race condition | Two threads access the same memory location, at least one writes, and there is no synchronization → result depends on timing. |
| False sharing | Threads write *different* variables that lie in the *same* cache line (64 bytes) → the cache line bounces between cores → slow (not incorrect). |

---

## 2. Where the Parallelism Is

```
for each seam (k times)          <-- SEQUENTIAL: seam i+1 needs the image after seam i is removed
 |
 +-- 1. Energy        every pixel independent              -> parallel over rows
 +-- 2. DP            row y needs row y-1; a row is free   -> rows sequential, columns parallel
 +-- 3. Min endpoint  reduction over one row               -> parallel reduction (deterministic)
 +-- 4. Backtrack     chain of H dependent steps           -> SEQUENTIAL
 +-- 5. Removal       every row independent                -> parallel over rows
```

| Stage | Parallel? | Granularity | Synchronization | Expected benefit |
| ----- | --------- | ----------- | --------------- | ---------------- |
| Outer seam loop | **No** | – | – | – |
| 1. Energy | Yes | Rows (`H` iterations) | 1 barrier at end | High (`O(WH)`, independent) |
| 2. DP | Columns only | `W` iterations per row | 1 barrier **per row** | Medium (many barriers) |
| 3. Min endpoint | Yes | `W` elements | Combine step | Low (`O(W)`, small) |
| 4. Backtrack | **No** | – | – | – (`O(H)`, small) |
| 5. Removal | Yes | Rows (`H` iterations) | 1 barrier at end | Medium–High (memory-bound) |

---

## 3. Stage 1 – Energy Map

**Dependency analysis.** `e(x,y)` reads only the gray image (`I`) and writes only `e(x,y)`.
`I` is not modified during this stage. So all `W * H` computations are independent.

**Strategy.** Parallelize the loop over rows; each thread computes complete rows.

```cpp
#pragma omp parallel for schedule(runtime)
for (int y = 0; y < H; ++y) {
    // compute e(x, y) for x = 0 .. width-1  (reads gray rows y-1, y, y+1)
}
```

- **Why rows and not pixels:** a row is a contiguous block in memory, so each thread reads and writes
  consecutive addresses (good for the cache). `H` (450–4320) is much larger than the thread count (≤ 20),
  so there is enough work to share.
- **Race conditions:** none. Threads only *read* shared data (`I`) and each writes its own rows of `e`.
  Loop variables declared inside the loop are private automatically.
- **Synchronization:** the implicit barrier at the end of the loop guarantees the whole energy map is
  finished before the DP starts.
- **False sharing:** only at the boundary between two threads' blocks of rows (at most one cache line);
  negligible because each thread writes whole rows.
- **Performance expectation:** light arithmetic per pixel → probably limited by memory bandwidth on large images.

---

## 4. Stage 2 – Dynamic Programming (the difficult stage)

### 4.1 The dependency

```
row y-1:   M(x-1,y-1)   M(x,y-1)   M(x+1,y-1)
                  \         |         /
row y:                   M(x,y)
```

- `M(x,y)` needs three values from row `y-1` → **row `y` cannot start before row `y-1` is finished**.
  Rows must be processed in order. This is a true data dependency; no directive can remove it.
- Inside one row, `M(x,y)` only reads row `y-1` and writes its own cell → **all cells of a row are
  independent** and can be computed in parallel.

So the parallelism is "row by row, columns in parallel", with a **barrier between consecutive rows**.

(Columns cannot be processed independently either: cell `(x,y)` depends on columns `x-1..x+1` above,
and after `y` rows its "cone of dependency" covers `2y+1` columns.)

### 4.2 Variant A – one `parallel for` per row

```cpp
// copy row 0 of energy into M
for (int y = 1; y < H; ++y) {                 // sequential
    #pragma omp parallel for schedule(runtime)
    for (int x = 0; x < width; ++x) {
        M(x,y) = e(x,y) + min3(M(x-1,y-1), M(x,y-1), M(x+1,y-1));
    }                                         // implicit barrier: row y finished
}
```

Simple and obviously correct. Cost: the thread team is started and joined `H - 1` times per seam.

### 4.3 Variant B – one persistent parallel region

```cpp
#pragma omp parallel                          // team created ONCE per seam
{
    for (int y = 1; y < H; ++y) {             // every thread runs this loop
        #pragma omp for schedule(runtime)
        for (int x = 0; x < width; ++x) {
            M(x,y) = e(x,y) + min3(...);
        }                                     // implicit barrier: row y finished
    }
}
```

- Every thread executes the outer `y` loop itself (`y` is private: declared inside the region).
  At each row the `omp for` splits the columns among them.
- The **implicit barrier at the end of `omp for`** is exactly the required synchronization point:
  no thread starts row `y+1` until all threads have finished row `y`.
- It avoids repeatedly creating/waking a thread team, but the **barrier per row still remains**.
  Whether B is faster than A is an experiment for M2/M4 (both variants will be selectable at run time).

### 4.4 The race condition we must avoid

If the barrier is removed with `nowait`:

```cpp
#pragma omp for nowait      // WRONG
```

a fast thread could start row `y+1` and read `M(x±1, y)` from a cell in another thread's part of row `y`
that has **not been written yet** → it reads an old value from a previous seam → wrong `M`, wrong seam.
The program would usually still "work" and produce a plausible image, so this bug is hard to see by eye.
The equivalence tests (compare `M` with the sequential version, many repetitions) are designed to catch it.

### 4.5 Performance concerns

- **Barrier count:** `H - 1` barriers per seam. Example (calculation, not measurement): 3840 x 2160 image
  reduced by 30 % → `1152 seams * 2159 ≈ 2.5 million` barriers.
- **Small work per barrier:** a row has only `W` cells (≈ 800 for test images), split over up to 20 threads
  → about 40 cells per thread between two barriers. The barrier cost may be comparable to the work.
  We expect DP to scale worse than the energy stage, especially on small images.
- **Schedule:** the work per cell is identical, so `static` is the natural choice; `dynamic` adds
  bookkeeping for no balance gain. We will still measure it (via `OMP_SCHEDULE`).
- **Hybrid CPU (i9-13900H, P- and E-cores):** with a barrier every row, every row waits for the slowest
  thread. With `static` schedule, E-cores receive the same amount of work as P-cores → P-cores wait.
  This is a reason to test `dynamic`/`guided` here (M4/M5).
- Ideas *not* used in M2 (possible M5 topics): processing several rows per synchronization with
  triangular/trapezoid blocks, or pipelining. They change the code structure significantly and must be
  justified by measurements first.

---

## 5. Stage 3 – Minimum Endpoint (parallel reduction)

**Problem.** Find the column with the smallest `M(x, H-1)`; ties → smallest column (decision D5).

A plain `reduction(min: value)` gives the minimum *value* but not its *column*, and OpenMP does not
guarantee which thread's result "wins" in a tie. We need a deterministic `(value, column)` minimum.

**Strategy: per-thread local minimum, then a sequential combine.**

```cpp
std::vector<PaddedMin> local(numThreads);     // one slot per thread, padded to 64 bytes
#pragma omp parallel
{
    int t = omp_get_thread_num();
    int bestVal = INT_MAX, bestCol = -1;
    #pragma omp for schedule(runtime) nowait
    for (int x = 0; x < width; ++x) {
        int v = M(x, H-1);
        if (v < bestVal) { bestVal = v; bestCol = x; }   // within a thread: x increases
    }
    local[t] = {bestVal, bestCol};            // each thread writes only its own slot
}                                             // implicit barrier at end of region
// sequential combine, lexicographic comparison of (value, column)
```

- **Why the result is deterministic:** the combine compares pairs `(value, column)` lexicographically:
  smaller value wins; equal value → smaller column wins. This defines a **total order**, so the minimum
  is unique no matter how the columns were divided or in which order threads finish.
  (Inside one thread, `x` may not be visited in increasing order with `dynamic` scheduling, so the local
  comparison also uses `(value, column)` – the code will compare both fields, not just `<` on value.)
- **Race conditions:** none – each thread writes only `local[t]`. The `nowait` is safe because the
  region's own implicit barrier comes right after.
- **False sharing:** `local[t]` slots are padded to 64 bytes so that two threads never write the same cache line.
- **Alternative considered:** `#pragma omp declare reduction` with a custom `(value, column)` combiner
  (OpenMP 4.0+, supported by GCC). Equivalent result; the explicit version is easier to explain and debug.
- **Performance expectation:** only `W` elements – the parallel region overhead may cost more than it
  saves. This stage is parallelized mainly because M0/M2 require it; M4 will measure whether it helps.

---

## 6. Stage 4 – Backtracking (stays sequential)

`s[y-1]` depends on `s[y]`: to know where the seam is in row `y-1`, we must know where it is in row `y`.
This is a chain of `H - 1` dependent steps, so it **cannot be parallelized**.

It costs `O(H)` (3 comparisons per row), compared with `O(W * H)` for the other stages, so its
share of the run time should be small. By **Amdahl's law**, if a fraction `f` of the run time is
sequential, the speedup with `p` threads is at most

```
Speedup(p) <= 1 / ( f + (1 - f) / p )
```

The sequential parts here are backtracking, the outer seam loop control, and the per-row
synchronization in DP. M4 will measure `f` from the stage timings.

---

## 7. Stage 5 – Seam Removal

**Dependency analysis.** In row `y`, pixels right of `s[y]` move one position left. Row `y` only
touches row `y` (of both the color image and the gray image). Different rows are independent.

```cpp
#pragma omp parallel for schedule(runtime)
for (int y = 0; y < H; ++y) {
    // memmove color row y: positions s[y]+1 .. width-1  ->  s[y] .. width-2   (3 bytes per pixel)
    // memmove gray  row y: same, 1 byte per pixel
}
```

- **Why `memmove`, not `memcpy`:** source and destination overlap within the same row; `memmove`
  is defined for overlapping ranges.
- **Race conditions:** none, because every row is written by exactly one thread and no thread reads
  another thread's row. The seam vector `s` is only read.
- **Load balance:** the amount of data to shift depends on `s[y]` (a seam near the left edge means
  more shifting). Rows have different costs → `dynamic`/`guided` may help slightly; to be measured.
- **Performance expectation:** pure memory copying → limited by memory bandwidth, so speedup will
  flatten once the memory bus is saturated.

---

## 8. Combined Implementation and Configuration

To produce the five versions required by `M0.md` without copying code, the OpenMP version reads a
`ParallelConfig` (details in `architecture.md`) that switches each stage on or off:

| Required version | Configuration |
| ---------------- | ------------- |
| 1. Sequential baseline | Separate code in `namespace seq` (no OpenMP at all) |
| 2. OpenMP energy | only `parEnergy` |
| 3. OpenMP DP | only `parDP` (+ `persistentDP` chooses variant A or B) |
| 4. OpenMP removal | only `parRemoval` |
| 5. Combined | all stages |

A stage that is switched off runs the same sequential loop inside `omp_impl` (no pragma), so the
comparison isolates the effect of each stage.

Thread count is set with `omp_set_num_threads(config.threads)` once before carving. Schedule is
chosen with `schedule(runtime)` + `OMP_SCHEDULE` so experiments need no recompilation.

---

## 9. Summary of Synchronization Points (per seam)

| # | Where | Mechanism | Protects |
| - | ----- | --------- | -------- |
| S1 | End of energy loop | implicit barrier of `parallel for` | DP must not read unfinished energy |
| S2 | End of every DP row | implicit barrier of `omp for` (A: of `parallel for`) | Row `y` must be complete before row `y+1` reads it |
| S3 | End of min-endpoint region | implicit barrier at end of `parallel` | Combine step must see all local results |
| S4 | End of removal loop | implicit barrier of `parallel for` | Next seam's energy must see the fully shifted image |

No `critical` sections, locks or atomics are planned: every thread writes only data it owns.

## 10. Potential Race Conditions Checklist (for M2 review and M3 tests)

| Risk | Where | Prevented by | Test that catches it |
| ---- | ----- | ------------ | -------------------- |
| Reading row `y-1` before it is complete | DP | Barrier S2; never use `nowait` there | Compare `M` with seq, many repetitions |
| Shared loop/temporary variable | All loops | Declare variables inside the loop body (private) | Compare all stages with seq |
| Non-deterministic tie in minimum | Min endpoint | `(value, column)` lexicographic combine | Uniform image, many thread counts |
| Two threads writing the same row | Removal | One row per iteration | Compare final images bit-exact |
| False sharing in per-thread slots | Min endpoint | 64-byte padding | (performance only, not correctness) |

ThreadSanitizer is not available with MinGW on Windows, so detection relies on these comparison
tests repeated many times with different thread counts and schedules.

## 11. Implementation Notes (M2)

Code: `include/seam_omp.h`, `src/seam_omp.cpp` (namespace `omp_impl`). The sequential code
(`src/seam_seq.cpp`) was not changed.

### 11.1 How the design was implemented

| Stage | Code | Directive(s) |
| ----- | ---- | ------------ |
| Energy | `computeEnergy` → `energyRow(y)` | `parallel for schedule(runtime)` over rows |
| DP variant A | `computeCumulative`, `persistentDP = false` | `parallel for schedule(runtime)` over columns, once per row |
| DP variant B | `computeCumulative`, `persistentDP = true` | one `parallel` region; inside, every thread loops over rows; `omp for schedule(runtime)` over columns (no `nowait`) |
| Min endpoint | `findMinEndpoint` | `parallel` + `omp for schedule(runtime) nowait`; per-thread `LocalMin` slots (`alignas(64)`), sequential combine with `(value, column)` order |
| Backtrack | `backtrackSeam` | none (sequential) |
| Removal | `removeSeam` → `removeRow(y)` | `parallel for schedule(runtime)` over rows |

- The per-row / per-cell arithmetic (`energyRow`, `dpCell`, `removeRow`) is written **once** and used by
  both the parallel and the non-parallel loop of each stage, so a switched-off stage computes exactly the
  same thing without any OpenMP directive.
- Data sharing: matrices and `width` are shared; they are either only read, or every thread writes a
  disjoint part (its rows, or its cells of the current DP row). Loop counters and temporaries are declared
  inside the loops, so they are private. No `critical`, `atomic` or locks are used.
- `carve()` calls `omp_set_num_threads(config.threads)` once; the stage functions use whatever thread
  count and schedule are currently set.

### 11.2 Default schedule

`schedule(runtime)` reads the run-time schedule. **If `OMP_SCHEDULE` is not set, GCC's libgomp uses
`dynamic,1`** (observed: the CSV line printed `dynamic:1`). That hands out one iteration at a time, which
is extremely slow for the DP rows. Therefore the CLI calls `omp_impl::useStaticScheduleUnlessSet()`: when
`OMP_SCHEDULE` is unset the schedule is `static`; when it is set, its value is used unchanged.

### 11.3 What remains sequential and why

| Part | Reason |
| ---- | ------ |
| Outer seam loop | Seam `i+1` is computed on the image after seam `i` was removed |
| DP row order | `M(.,y)` needs the complete row `y-1` |
| Row 0 of DP (`memcpy`) | `O(W)`, too small to be worth a parallel region |
| Backtracking | Chain of `H-1` dependent steps |
| Combine step of the minimum | At most one entry per thread |
| Gray conversion, final crop, transposes | Done once per `carve()` (OpenCV calls) |

### 11.4 Measured OpenMP overhead on the development machine (important limitation)

Micro-benchmark (scratch program, 20 000 repetitions, GCC 16.2.0 `-O2 -fopenmp`, libgomp, Windows 11,
i9-13900H, AC power, "Balanced" power plan; one run, so values vary by roughly ±30 % between runs):

| Threads | Entering a (nearly empty) parallel region | One barrier |
| ------- | ----------------------------------------- | ----------- |
| 1 | 8 µs | ≈ 0 µs |
| 2 | 26 µs | 15 µs |
| 4 | 36 µs | 28 µs |
| 8 | 66 µs | 51–83 µs |
| 20 | 261 µs | 172–213 µs |

`OMP_WAIT_POLICY=active` did not change these values. On Linux, libgomp synchronizes with *futexes* and a
barrier typically costs around 1 µs or less (literature value, **not measured here**). Our hypothesis is that
the Windows build of libgomp synchronizes through slower operating-system primitives (winpthreads); we have
not verified the exact cause.

Consequence: one DP row of an 800-pixel-wide image needs only a few microseconds of real work, but every
row costs one barrier (variant B) or one region + barrier (variant A). **On this platform the parallel DP is
therefore slower than the sequential DP**, for both variants (see `WORKLOG.md`, M2 entry, for the measured
run times). Energy and removal have one region per seam, not per row, and are not affected in the same way.
This is a key input for M4 (analysis) and M5 (parallel region management, synchronization overhead).

## 12. Expected Bottlenecks (hypotheses to be tested in M4, not results)

1. **DP barriers** – `H - 1` barriers per seam with little work between them.
2. **Memory bandwidth** – energy and removal do little arithmetic per byte.
3. **Sequential outer loop** – `k` seams always run one after another; the parallel regions are entered `k` times.
4. **Hybrid P/E cores** – unequal core speeds with barriers → threads wait for the slowest one.
5. **Small images** – on 512 x 512 the work per parallel region may be too small to hide overhead.
