# Parallelization Strategy (OpenMP)

Goal: speed up seam carving on a multicore CPU **without changing its result**. Every OpenMP variant must produce output byte-identical to the sequential reference.

Target machine: AMD Ryzen 7 4800H, 8 cores / 16 hardware threads (SMT).

---

## 1. Dependency analysis

| Stage | Reads | Writes | Dependency | Parallelizable? |
| ----- | ----- | ------ | ---------- | --------------- |
| Energy | `gray` (rows i-1..i+1) | `energy[i][j]` | none between pixels | **Fully** (embarrassingly parallel) |
| DP | `energy[i][*]`, `M[i-1][j-1..j+1]` | `M[i][j]` | row i needs row i-1 | **Within a row** only; rows are sequential |
| Argmin | `M[H-1][*]` | one index | reduction | Yes (reduction), but only W elements |
| Backtrack | `M`, `s[i+1]` | `s[i]` | chain of length H | **No** |
| Removal | `seam[i]`, row i | row i | none between rows | **Fully** across rows |
| Outer loop | image after seam k-1 | image after seam k | seam k needs seam k-1 | **No** (doing so changes the algorithm) |

Work–span view, for one seam:
- Work = Θ(H·W).
- Span = Θ(H + W): the DP row chain plus the backtrack plus an in-row shift.
- Available parallelism ≈ H·W / (H + W), about 690 for 1920×1080.

Parallelism is therefore not the limiting factor. The expected limits are synchronization overhead, memory bandwidth and the sequential backtrack. These are hypotheses to be checked in M4.

## 2. Stage strategies

### A. Energy

```cpp
#pragma omp parallel for schedule(runtime)
for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j)
        energy[i * stride + j] = energyAt(gray, i, j, H, W, stride);
```

- **Threads:** each thread gets a block of rows. `gray` is read-only; each `energy` element is written by exactly one thread.
- **Races:** none.
- **Synchronization:** the implicit barrier at the end of the loop, before the DP reads `energy`.
- **Notes:** rows (not pixels) are distributed, so each thread scans contiguous memory and the scheduling overhead is per row. `schedule(runtime)` lets M2/M4 compare `static`, `dynamic` and `guided` through `OMP_SCHEDULE` without recompiling.

### B. Dynamic programming

**Version B1 – a parallel for per row.**

```cpp
for (int j = 0; j < W; ++j) M[j] = energy[j];              // row 0
for (int i = 1; i < H; ++i) {
    #pragma omp parallel for schedule(runtime)
    for (int j = 0; j < W; ++j) M[i*stride + j] = energy[i*stride + j] + minParent(M, i, j, W, stride);
}
```

**Version B2 – one persistent parallel region** (investigated per M2 §B).

```cpp
#pragma omp parallel
{
    #pragma omp for schedule(runtime)
    for (int j = 0; j < W; ++j) M[j] = energy[j];
    for (int i = 1; i < H; ++i) {           // i is private (declared inside the region); all threads run the same i sequence
        #pragma omp for schedule(runtime)   // implicit barrier after each row
        for (int j = 0; j < W; ++j) M[i*stride + j] = energy[i*stride + j] + minParent(M, i, j, W, stride);
    }
}
```

- **Synchronization:** the implicit barrier at the end of every row's `omp for` guarantees that row i is completely written before any thread reads it for row i+1. `nowait` must **not** be used here.
- **Race if the barrier is removed:** a thread could read `M[i][j±1]` at the border of a neighbouring thread's block before it is written.
- **Cost:** H-1 barriers per seam, so about k·(H-1) for k seams. For 1080 rows and 576 seams (30 % of 1920) that is about 620 000 barriers per run.
  - B1 additionally activates the thread team H-1 times per seam.
  - B2 activates it once per seam.
- **Expected:** B2 ≤ B1 in time, and both suffer when W is small (little work per barrier). To be measured.
- **Not in M2:** trapezoid tiling to reduce barriers (textbook ch. 25) is an M5 candidate.

### C. Argmin of the last row

The tie rule is the smallest index among equal minima. A plain `reduction(min:)` returns only the value, not the index, so the index is merged explicitly:

```cpp
int bestV = INT_MAX, bestJ = -1;
#pragma omp parallel
{
    int lv = INT_MAX, lj = -1;                 // private
    #pragma omp for schedule(static) nowait
    for (int j = 0; j < W; ++j)
        if (last[j] < lv) { lv = last[j]; lj = j; }
    #pragma omp critical
    if (lj != -1 && (lv < bestV || (lv == bestV && lj < bestJ))) { bestV = lv; bestJ = lj; }
}
```

- Each thread's static chunk is scanned left to right with strict `<`, giving the first local minimum.
- The merge by (value, index) yields the global first minimum regardless of the order in which threads enter `critical`. The result is therefore deterministic and equal to the sequential scan.
- `critical` is entered once per thread (not per element), so contention is negligible.
- An alternative with the same semantics is a user-defined reduction (`#pragma omp declare reduction`, OpenMP 4.0, supported by g++). It may be tried in M2.
- Since W ≤ 7680, the benefit is expected to be small or negative. Kept as a separate variant to measure.

### D. Seam removal

```cpp
#pragma omp parallel for schedule(runtime)
for (int i = 0; i < H; ++i) {
    const int s = seam[i];
    uint8_t* g = &gray[i * stride];
    for (int j = s; j < W - 1; ++j) g[j] = g[j + 1];
    uint8_t* c = &color[i * stride * 3];
    std::memmove(c + 3 * s, c + 3 * (s + 1), 3 * (W - 1 - s));
}
// after the loop, in sequential code: img.width -= 1
```

- **Races:** none. Row i occupies `[i·stride, (i+1)·stride)` and never overlaps another row, because the stride is fixed. Each row is handled by one thread.
- If the image were compacted to stride W-1 inside the same array, row i would overwrite row i-1's memory. That would be a race, so this layout is avoided by design.
- **Load balance:** rows with a seam near the left do more work. With `static` scheduling over many rows the imbalance averages out; `dynamic` is an experiment.

### E. Combined variant

The stages are called in sequence. Each stage that is a `parallel for` has an implicit barrier at its end. That barrier is the synchronization point between stages, and it also flushes memory so the next stage sees the written data.

The backtrack, the update `width -= 1`, and the decision to continue the loop remain sequential (on the master thread, outside the parallel regions).

M5 may investigate one parallel region around the whole seam loop (`omp single` for the backtrack and the width update). That requires every thread to see the same `width` at each loop test, which is guaranteed by the barrier at the end of `single`.

## 3. Synchronization points (per seam)

| # | Where | Mechanism | Protects |
| - | ----- | --------- | -------- |
| 1 | end of energy loop | implicit barrier | DP reads complete `energy` |
| 2 | end of each DP row (H-1 times) | implicit barrier of `omp for` | row i+1 reads complete row i |
| 3 | argmin merge | `critical` (once per thread) | shared `bestV`, `bestJ` |
| 4 | end of argmin region | implicit barrier | backtrack reads the final index |
| 5 | end of removal loop | implicit barrier | next iteration reads the shifted image |

## 4. Potential race conditions and how they are avoided

| Risk | Avoided by |
| ---- | ---------- |
| Reading row i-1 of M before it is complete | No `nowait` in the DP; barrier per row |
| Several threads updating the min value/index | Private local minimum + `critical` merge |
| An inner loop variable declared outside the parallel region becomes shared | Declare all loop variables in the `for` statement |
| In-place compaction overlapping rows | Fixed stride; shift only inside a row |
| Width changed while other threads use it | `width` updated only in sequential code (or in `single` in M5) |
| OpenCV internal threads disturbing measurements | No OpenCV calls in timed code; `cv::setNumThreads(1)` at startup |

## 5. Determinism

- Integer energy and DP: addition and `min` are exact and associative, so the order of evaluation cannot change any value.
- The same `energyAt()` and parent rule (`seam_common.hpp`) are used by seq and omp.
- The argmin tie rule is the smallest index, implemented identically in seq and omp (§2.C).

As a result, **exact equality** (not a tolerance) is the correctness criterion for the energy, the M matrices, the seams and the output images.

## 6. Thread count and scheduling control

- `--threads N` calls `omp_set_num_threads(N)` before carving. It is tested with 1, 2, 4, 8 and 16 threads.
  - 16 means 2 threads per physical core (SMT), so lower efficiency there is expected and will be reported.
  - Odd counts (3, 7) are used in correctness tests.
- Scheduling is chosen with `schedule(runtime)` plus the environment variable `OMP_SCHEDULE=static|dynamic,64|guided`.
- Thread placement (`OMP_PROC_BIND`, `OMP_PLACES`) is recorded per run and may be compared in M5.
- A warm-up parallel region runs before timing, so thread creation is excluded from steady-state measurements (M4 §3).

## 7. What stays sequential, and why

1. The **outer seam loop**: seam k is defined on the image after seam k-1, so computing several seams at once is a different (approximate) algorithm.
2. **Backtracking**: a chain of H dependent steps.
3. **Row order in the DP**: each row needs the previous one.

By Amdahl's law, these together with the barrier overhead bound the achievable speedup. Their share will be measured per phase in M4.

## 8. Expected bottlenecks (hypotheses, not results)

1. Barrier overhead in the DP, worst for narrow images.
2. Memory bandwidth: each DP cell does about 2 integer operations per 8 bytes of traffic, so these loops are likely memory-bound.
   - At 1080p, `energy` and `M` are 8.3 MB each. The machine reports 8 MB of L3 in total, so the data does not fit in cache.
3. The sequential backtrack and argmin.
4. Diminishing returns beyond 8 threads (SMT).

Each of these will be confirmed or rejected with measurements in M4.
