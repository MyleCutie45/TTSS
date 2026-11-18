# Testing Report (M3 – Correctness Validation)

**Milestone:** M3
**Status:** Completed (2026-11-19). Every result below was measured; see section 6 for limitations.

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

Second environment (only for the sanitizer runs, section 5.4): WSL 2, Ubuntu 26.04 LTS on the same
machine; g++ 15.2.0 (libgomp) for ASan/UBSan, clang++ 21.1.8 (LLVM libomp + Archer) for TSan,
OpenCV 4.10.0, CMake 4.2.3, build type RelWithDebInfo (`-O2 -g`). Passing there also shows that the
results do not depend on one compiler, OpenMP runtime or OpenCV version.

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

Race conditions and memory errors: before M3 there was **no tool-based check**. ThreadSanitizer and
AddressSanitizer are not available with MinGW GCC on Windows; races were only detected indirectly
(repeated seq-vs-omp comparison, M2 mutation test with `nowait`). M3 adds both (section 5.4).

---

## 4. New Tests and Tools

### 4.1 `tests/test_m3.cpp`

The existing test files are unchanged. `test_m3` has two modes: `test_m3 --quick` (reduced parameter
sets, ~15 s) and `test_m3` (full).

**Independent reference implementation** (namespace `ref` in `test_m3.cpp`). All earlier equivalence tests
compare omp with seq. If both contained the *same* mistake, those tests could not see it. The reference
is written in a deliberately different style: element access with `at<>()`, an explicit clamp function,
**64-bit** cumulative sums (an int32 overflow would show up as a mismatch), tie-breaking by comparing
`(value, column)` pairs, and seam removal by *copying* into a new image instead of shifting in place.

**Lockstep run** (used by most tests): seq, omp and (optionally) the reference process the same image
seam by seam; after **every stage of every seam** energy map, `M` matrix, endpoint, seam and image are
compared exactly, and the seam is checked for connectivity (one pixel per row, inside `[0, width)`,
neighbours differ by at most one column).

| Test | What it checks | M3 requirement(s) |
| ---- | -------------- | ----------------- |
| 1 Edge-case images | omp `carve()` = seq `carve()` and correct dimensions for images 1×1, 8×1, 1×8, 2×9, 3×9, 9×2, 9×3, 5×5, 13×17; targets unchanged / 1 column / 1 row / 1×1 / half; all 8 stage variants; 1, 2, 3, 5, 8, 16 threads; `static`, `static,1`, `dynamic` | 6, 8, 9 |
| 2 Forced border seams | gray ramps whose only zero-energy column is the first (or last) column → the seam must be that column; removal with `count = 0`; zig-zag seam touching both borders; seq = omp = reference | 4, 5, 9 |
| 3 Patterns vs reference | 2×2-block checkerboard (reaches the maximum energy **510**), 1×1 checkerboard, stripes, gradients, constant image; energy in `[0, 510]`; lockstep with reference | 1, 2 |
| 4 Brute force, non-square | every connected seam enumerated for 1×7, 2×7, 3×6, 6×2, 6×1, 7×4, 4×8 random images (10 seeds); omp DP minimum = best seam cost; 1, 3, 8 threads, DP variants A and B | 2, 3 |
| 5 Long run | `building_windows` (533×800): 100 seams, every seam checked (connectivity + seq = omp), all(A)/all(B), 4 and 8 threads; first 3 seams also vs reference | 4, 7 |
| 6 Large images | 1920×1080 and 3840×2160 (deterministic random content): first seam vs reference, 2 seams per stage variant (8 threads), all(A) with 3 threads | 10, 7 |
| 7 Thread counts | 3, 5, 7, 16 threads (16 = oversubscription on 8 logical processors); inputs 61×47, **9×120** (fewer columns than threads), **130×5** (fewer rows than threads); all 8 variants; 4 schedules; 2 repetitions | 8, 9 |
| 8 Repeated runs | the same whole run 50 times per configuration (all(A)/all(B), static/dynamic, 8 threads); every result identical | 7 (races) |
| 9 Real images | the benchmark task "−20 % width, −10 % height" on all 5 test images, all(B), 4 threads, omp = seq | 6, 7, 10 |

Floating-point tolerance: **none needed.** Energy and `M` are integers (`CV_32SC1`), the gray conversion is
done once by OpenCV before the algorithm; every comparison in every test is exact.

### 4.2 Test sets in CTest

| Label | Tests | Time (measured) |
| ----- | ----- | --------------- |
| `quick` | `test_seq`, `test_m3_quick`, 8 CLI tests | 31 s |
| `full` | `test_equivalence`, `test_m3_full` | about 4 + 4 min |

```bash
ctest --test-dir build -L quick     # after every change
ctest --test-dir build -L full      # before a commit / merge
```

### 4.3 Mutation check (`scripts/mutation_check.py`)

Question: *would the tests notice a real bug?* The script copies the sources to `build/mutants/<id>/`
(the real sources are never touched), inserts one known bug, builds, runs the tests and records whether
they fail.

### 4.4 Sanitizers (`scripts/run_sanitizers.sh`, Linux/WSL)

| Tool | Finds | Build |
| ---- | ----- | ----- |
| AddressSanitizer + UndefinedBehaviorSanitizer | out-of-bounds reads/writes, use-after-free, leaks, integer overflow and other undefined behaviour | g++ 15.2, libgomp, `-fsanitize=address,undefined -fno-sanitize-recover=all` |
| ThreadSanitizer + Archer | data races (two threads access the same memory, at least one writes, no synchronization between them) | clang++ 21.1.8, LLVM libomp, `-fsanitize=thread`; Archer (`libarcher.so`) tells TSan about OpenMP barriers |

TSan needs a suppression file (`tests/tsan.supp`) for libraries that are not compiled with TSan; see 5.4.

---

## 5. Results

### 5.1 Native tests (Windows, GCC 16.2.0, Release)

| Command | Result | Time |
| ------- | ------ | ---- |
| `test_m3 --quick` | **6 801 checks, 0 failed** | 15 s |
| `test_m3` (full) | **71 580 checks, 0 failed** | 245 s (machine busy with other runs at the same time) |
| `ctest -L quick` | 10/10 passed | 31 s |
| `ctest` before M3 (baseline) | 10/10 passed; `test_seq` 65 / 0, `test_equivalence` 147 368 / 0 | 254 s |

Full-mode detail (checks / time): 1 edge cases 13 005 / 8.0 s; 2 border seams 129 / 0.0 s; 3 patterns
2 115 / 0.9 s; 4 brute force 420 / 0.2 s; 5 long run 2 431 / 32.3 s; 6 large images 212 / 8.1 s;
7 thread counts 53 248 / 26.6 s; 8 repeated runs 5 / 114.4 s (each check = 50 identical runs);
9 real images 15 / 54.3 s.

### 5.2 Mutation check

`python scripts/mutation_check.py` (2026-11-18). Each mutant was tested with `test_m3 --quick`;
M8 additionally with `test_equivalence`.

| ID | Bug inserted | Test | Result |
| -- | ------------ | ---- | ------ |
| M1 | DP variant B: `nowait` removes the barrier between rows (**race condition**) | `test_m3 --quick` | detected: 75 of 6 421 checks failed |
| M2 | min endpoint: every thread writes slot 0 (**lost update**) | `test_m3 --quick` | detected: program **crashed** (exit 0xC0000005, access violation) |
| M3 | min endpoint (omp): tie rule reversed | `test_m3 --quick` | detected: 91 / 6 073 failed |
| M4 | energy (omp): clamp with `gray.cols` instead of current width | `test_m3 --quick` | detected: 144 / 5 265 failed |
| M5 | removal (omp): one pixel too few shifted (off-by-one) | `test_m3 --quick` | detected: 527 / 2 579 failed |
| M6 | DP (omp): reads up-right neighbour also in the last column | `test_m3 --quick` | detected: 283 / 3 319 failed |
| M7 | endpoint (seq): `<=` instead of `<` | `test_m3 --quick` | detected: 220 / 6 073 failed |
| M8 | **same** bug in seq **and** omp: forward difference `I(x+1)-I(x)` | `test_m3 --quick` | detected: 120 / 3 981 failed |
| M8 | (same mutant) | `test_equivalence` | **not detected: 0 / 147 368 failed** |

(The number of checks differs between mutants because a lockstep run stops at the first seam where the
states diverge.)

Observations:

- **All 8 bugs are detected by `test_m3 --quick`.**
- **M8 shows why the independent reference was needed:** when seq and omp contain the same mistake, the
  seq-vs-omp test (`test_equivalence`) passes completely; only the comparison with the reference (and the
  hand-computed 4×4 example in `test_seq`) can find it.
- **M2 shows how a race can turn into a crash:** with all threads writing slot 0, the value that survives
  is from whichever thread wrote last. If that thread received no column at all (more threads than
  columns), the stored column is `-1`; backtracking then reads column `-1` → access outside the matrix.

### 5.3 What the review predicted vs what the tests found

The code review (section 2) found no race and no out-of-range access. Native tests, mutation check and
sanitizers (5.4) agree: **no defect was found in `src/seam_seq.cpp` or `src/seam_omp.cpp`**, so no source
file was changed in M3.

### 5.4 Sanitizers (WSL, Ubuntu 26.04)

**AddressSanitizer + UBSan** (`bash scripts/run_sanitizers.sh asan`, 2026-11-19):

| Program | Result | Time under ASan |
| ------- | ------ | --------------- |
| `test_seq` | 65 checks, 0 failed, **no sanitizer report** | 3 s |
| `test_m3` (full) | 71 580 checks, 0 failed, **no sanitizer report** | 168 s |
| `test_equivalence` | 147 368 checks, 0 failed, **no sanitizer report** | 2 650 s (ran in parallel with the TSan build) |

→ no out-of-bounds access, use-after-free, leak or undefined behaviour in any test.

**ThreadSanitizer + Archer**, step by step:

1. *Without suppressions* (`test_m3 --quick`): 4 reports, all outside the project code (symbolized with
   `llvm-symbolizer`): reports 1–3 = `pthread_mutex_lock` vs `pthread_mutex_init` / `memset`, both called
   from inside `libomp` (its own lock initialization); report 4 = `operator delete[]` vs `memcmp`, both in
   threads created by `libtbb` (the thread pool OpenCV uses on Ubuntu; the project does not use TBB).
   These libraries are not compiled with TSan, so TSan cannot see their internal synchronization:
   **false positives**. Tests: 6 801 checks, 0 failed.
2. *First suppression file* (`race:libomp.so` …): 0 reports on the original code — but also **0 reports on
   mutant M1** (barrier removed), although 42 checks failed there. The positive control showed that this
   suppression hides real races: a `race:` rule matches any stack frame, and every OpenMP worker thread
   is started by libomp. Rejected.
3. *Final suppression file* (`tests/tsan.supp`: only `called_from_lib:libomp.so`,
   `called_from_lib:libtbb.so`, plus `OPENCV_FOR_THREADS_NUM=1`):

| Run | Tests | TSan reports |
| --- | ----- | ------------ |
| original code, `test_m3 --quick` | 6 801 checks, 0 failed | **0** |
| **mutant M1** (positive control), `test_m3 --quick` | 76 failed of 5 773 | **83 reports, all 83 in `src/seam_omp.cpp`** (`dpCell`, line 39, called from the DP loop, line 127) |
| original code, `test_m3` (full) | 71 580 checks, 0 failed (186 s) | **0** |

→ TSan does detect a real OpenMP race in this setup (M1), and reports **no race in the original code**.

`test_equivalence` was not run under TSan: an unsuppressed attempt was still running after more than
40 min and was stopped; `test_m3` covers the same stage variants, thread counts and schedules.

---

## 6. Conclusions

- All 10 requirements of `M3.md` are covered (table in section 3 + tests in section 4.1); every
  comparison is exact (integers, no tolerance).
- Sequential and OpenMP versions give **bit-identical** energy maps, `M` matrices, seams and images for every
  tested image size (1×1 … 3840×2160), thread count (1–8, 16, 20), schedule and stage combination, on two
  compilers / OpenMP runtimes (GCC + libgomp on Windows, clang + libomp on Linux).
- **No defect was found**; no source file had to be changed. The tests are shown to be able to find bugs
  (8/8 mutants detected), and TSan is shown to find a real race in this code base (M1 positive control).

Limitations (honest list):

- Testing can show the presence of bugs, not their absence. Races depend on timing; the tests and TSan
  only observe the schedules that actually happened.
- TSan was run with suppressions for libomp/libtbb internals; races *inside* those libraries are not
  examined (they are outside the project).
- The sanitizers ran on Linux (clang/libomp, gcc/libgomp), not on the Windows/MinGW build used for
  benchmarks; the Windows build is covered by the native tests only.
- The large-image tests use deterministic random content and only a few seams per configuration.

How to repeat everything:

```bash
ctest --test-dir build -L quick && ctest --test-dir build -L full     # Windows (MSYS2 UCRT64)
python scripts/mutation_check.py                                       # Windows
bash scripts/run_sanitizers.sh asan                                    # WSL / Linux
bash scripts/run_sanitizers.sh tsan
bash scripts/run_sanitizers.sh tsan-quick build/mutants/M1             # positive control
```
