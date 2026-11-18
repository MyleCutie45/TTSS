# Testing Report (M3 – Correctness Validation)

**Milestone:** M3
**Status:** In progress – sections 1–3 done (code review, coverage analysis). New tests, sanitizer runs
and final results are not done yet; they will be added to sections 4–6.

All results in this document come from commands that were actually executed. Anything not yet run is
marked *not run*.

---

## 1. Test Environment

| Item | Value |
| ---- | ----- |
| CPU | Intel Core i7-1065G7, 4 cores, 8 logical processors |
| RAM | 31.8 GB |
| OS | Windows 11 Home Single Language 10.0.22631 |
| Compiler | GCC 16.2.0 (MSYS2 UCRT64), `-std=c++17 -O2 -DNDEBUG -Wall -Wextra` |
| OpenMP | 5.2 (`_OPENMP = 202111`, libgomp) |
| OpenCV | 5.0.0 |

Baseline before M3 (2026-11-18): `ctest` 10/10 passed; `test_seq` 65 checks, 0 failed;
`test_equivalence` 147 368 checks, 0 failed (252 s).

---

## 2. Code Review (`src/seam_seq.cpp`, `src/seam_omp.cpp`)

Goal: find race conditions and out-of-range accesses by reading the code *before* testing.
For every parallel loop we asked: which data is shared, which is private, who writes what, and where
the threads wait for each other.

| Stage | Directive | Shared data | Written by a thread | Synchronization | Race possible? |
| ----- | --------- | ----------- | ------------------- | --------------- | -------------- |
| Energy | `parallel for` over rows `y` | `gray` (read), `energy` | only row `y` of `energy` | implicit barrier at loop end | No: each row has exactly one writer, `gray` is read-only |
| DP, variant A | sequential `y` loop, `parallel for` over columns `x` | `M` row `y-1` (read), `M` row `y` | only cell `(x, y)` | implicit barrier after every row | No: row `y-1` is complete before any thread reads it |
| DP, variant B | one `parallel` region, every thread runs the `y` loop, `omp for` over `x` | same as A | only cell `(x, y)` | implicit barrier of `omp for` (no `nowait`) | No; with `nowait` it would be a race (M2 mutation test confirmed this) |
| Min endpoint | `parallel` + `omp for nowait` | last row of `M` (read), `local[]` | only `local[thread_num]` | barrier at end of the region | No: one slot per thread, `alignas(64)` avoids false sharing |
| Backtrack | none (sequential) | – | – | – | – |
| Removal | `parallel for` over rows `y` | `seam` (read), `image`, `gray` | only row `y` of `image` and `gray` | implicit barrier at loop end | No: `memmove` stays inside one row |

Other points checked:

- **Variant B correctness rule:** OpenMP requires all threads of a team to meet the same worksharing
  constructs in the same order. Every thread runs the same `y` loop (same `height`), so this holds.
- **`findMinEndpoint` slot array:** sized with `omp_get_max_threads()` before the region; the team of the
  region can never be larger, so `local[omp_get_thread_num()]` is always in range. Threads that get no
  column (more threads than columns) keep `col = -1` and are skipped in the combine step.
- **Columns `>= width`:** energy reads `cur[xr]` with `xr <= width-1`, DP reads `prev[x±1]` only inside
  `[0, width)`, backtrack checks `c < width`. Stale data right of `width` is never read.
- **Integer overflow:** one pixel has energy `<= 255 + 255 = 510`, so `M <= 510 · rows`. When the height
  is reduced the image is transposed, so "rows" can be the original *width*. For 7680 × 4320 this gives
  `M <= 510 · 7680 ≈ 3.9 million`, far below `2^31 − 1 ≈ 2.1 billion`. No overflow.
- **Global side effect (not a bug):** `omp_impl::carve()` calls `omp_set_num_threads()`, which also changes
  the thread count of later calls to the stage functions. Tests must set the thread count explicitly.

**Result of the review: no race condition or out-of-range access found.** This is a reading result only;
sections 4–5 test it.

---

## 3. Coverage of the M3 Requirements by the Existing Tests

Existing tests: `tests/test_seq.cpp` (12 test functions, sequential only) and
`tests/test_equivalence.cpp` (4 test functions, seq vs omp).

| # | M3 requirement | Already covered | Gap |
| - | -------------- | --------------- | --- |
| 1 | Energy map correctness | 4×4 hand example (includes all borders); uniform image; columns `>= width` ignored | No test with maximum contrast (energy 510); no reference independent of `seq` for larger images |
| 2 | DP matrix correctness | 4×4 hand example; brute force on 5 random 5×5 images | Brute force only for square 5×5; not for very narrow (W = 1, 2) or tall images |
| 3 | Minimum seam selection | ties in 4×4 and uniform image; many-ties test (omp, widths 1…1001, up to 20 threads) | Covered |
| 4 | Seam connectivity | `validSeam` for one seam on a real image and in brute force | Not checked for every seam of a full run, not for omp seams |
| 5 | Seam removal | 4×4 example (color + gray); real image, one seam; lockstep seq vs omp | Seam in column 0 and in the last column (`count = 0`) not forced explicitly |
| 6 | Final image dimensions | seq: several targets incl. 1 column / 1 row / 1×1 | omp `carve()` dimensions on edge cases not tested |
| 7 | Seq vs parallel equivalence | lockstep (every stage, every seam, 20 seams, 3 inputs ≤ 200×160); full carve on 5 real images (12 + 6 seams) | omp never tested on edge-case images (1 column, 1 row, 2 columns, 1×1); height reduction only via full carve |
| 8 | Multiple thread counts | 1, 2, 4, 8, 20 | Odd counts (3, 5, 7) missing; threads > columns / rows only tested for min endpoint, not for energy, DP, removal |
| 9 | Boundary conditions | seq edge cases (see 6) | Same cases for omp; images narrower than the thread count |
| 10 | Small and large images | small: 4×4 … 800×600 | **No large image** (1920×1080, 3840×2160) tested |

Race conditions and memory errors: **no tool-based check yet.** ThreadSanitizer and AddressSanitizer
are not available with MinGW GCC on Windows; so far races are only detected indirectly (repeated
seq-vs-omp comparison, M2 mutation test with `nowait`).

---

## 4. Planned Additional Tests *(not implemented yet)*

New file `tests/test_m3.cpp` (existing test files stay unchanged):

1. **Edge-case equivalence (omp):** images 1×1, 1×N, N×1, 2×N, 3×N, N×3; targets "unchanged", "1 column",
   "1 row"; all stage variants; threads 1–8 and 16 (more threads than rows/columns).
2. **Forced seam positions:** images whose cheapest seam is column 0 or the last column (tests removal
   with `count = 0` and the borders of the DP).
3. **Maximum contrast:** 0/255 checkerboard and stripes; energy values 510; compare with hand-computed values.
4. **Brute-force DP on non-square shapes:** e.g. 7×2, 6×3, 2×6, 1×6, many seeds.
5. **Seam connectivity for every seam** of a whole run, for seq and omp.
6. **Large images:** 1920×1080 and 3840×2160 (generated deterministically), lockstep for a few seams.
7. **Odd thread counts** 3, 5, 7 in lockstep.

Tool-based checks (Linux / WSL, because MinGW has no sanitizers):

8. AddressSanitizer + UndefinedBehaviorSanitizer build of all tests.
9. ThreadSanitizer (or Valgrind/Helgrind) if usable with the OpenMP runtime; otherwise documented why not.
10. Mutation checks: introduce a known bug, confirm the tests fail, restore the code.

Test-time organization: split into a fast and a full test set (the current `test_equivalence` alone takes
252 s).

---

## 5. Results

*Not run yet.*

## 6. Conclusions

*Pending.*
