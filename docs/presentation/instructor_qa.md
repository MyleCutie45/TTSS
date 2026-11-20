# Potential Instructor Questions and Suggested Answers

Numbers refer to `docs/final_report.md` (FR), `docs/benchmark_report.md` (M4) and
`docs/optimization_report.md` (M5).

---

## Algorithm

**Q1. Why can't you remove several seams in parallel?**
Seam *i + 1* is defined on the image after seam *i* has been removed; its energy and DP depend on that
image. Computing several seams on the same image would give a different (non-optimal) result. We kept
the original semantics so that the output is identical to the sequential algorithm. Removing several
non-overlapping seams per pass is listed as future work because it changes the algorithm.

**Q2. Why is the DP the hard part?**
Row *y* of *M* needs three cells of row *y − 1*. Rows must be processed in order; only the cells of one
row are independent. That needs one synchronization per row: 1080 per seam for a 1920 × 1080 image, about
414 000 per run (FR §6).

**Q3. Is your energy function the one from the paper?**
Yes, *e₁ = |∂I/∂x| + |∂I/∂y|*. The discretisation is our decision: central differences without division by
2, on the gray image, indices clamped at the border, integers (FR Table 1). Dividing by 2 would scale all
energies equally and not change the seam.

**Q4. What happens with ties?**
The smallest column wins, both for the endpoint and in backtracking (candidates in the order
*x − 1, x, x + 1*). Without a fixed rule the parallel minimum could return different columns depending
on the thread timing, and the outputs would no longer be comparable.

## Parallelization and synchronization

**Q5. Where are the race conditions, and how did you prevent them?**
(1) DP: reading row *y − 1* while another thread still writes it → barrier per row (M2) or per block (M5).
(2) Parallel minimum: threads writing the same variable → one slot per thread, combined after the region.
(3) M5 removal + local energy update: the update reads rows *y ± 1* that another thread may still be
shifting → implicit barrier between the two `omp for` loops. (4) M5 blocked DP: reading the boundary row
that is being written → two boundary rows used alternately plus one barrier per block (FR §8).

**Q6. How do you know there are no races?**
Three independent kinds of evidence: (a) exact comparison with the sequential version for many thread
counts and schedules (≈ 237 000 checks); (b) mutation tests that remove each barrier – the tests fail;
(c) ThreadSanitizer with the Archer OpenMP tool reports no race on the original code but does report the
race when a barrier is removed (positive control). Testing cannot prove absence of races, but every
barrier we rely on is shown to be necessary and present.

**Q7. Why `schedule(static)`?**
Every row of an image has the same width and therefore the same cost; there is no load imbalance for
`dynamic` or `guided` to fix, and they add bookkeeping. Measured: all schedules within 5 % (M4 with the
DP sequential; M5 0.465–0.487 s), and `dynamic` with chunk 1 1.4–2.1× slower with the parallel DP (M4).

**Q8. What is the difference between DP variant A and variant B?**
A opens a parallel region for every row; B opens one region per seam and uses the implicit barrier of
`omp for` per row. B avoids repeated thread wake-up and was faster with 1–2 threads on Windows; with
8–16 threads it was slower (M4 §6.5). Both are replaced in M5 by the blocked DP.

**Q9. Explain the blocked DP. Isn't it computing things twice?**
Yes, deliberately. Each thread computes a slightly wider range than it owns ("halo"), shrinking by one
column per side per row, so within a block of *K* rows it never needs its neighbours' results. Cost:
about *K − 1* extra cells per row and thread (≈ 1 % at K = 64); gain: *H/K* barriers instead of *H*. The
results are identical because every cell uses the same formula and inputs (FR §13.3).

## Performance

**Q10. Why was the parallel version slower than the sequential one?**
On Windows with GCC's libgomp, one barrier costs 22–167 µs while one DP row has 1.84 µs of work.
414 336 barriers × 45 µs ≈ 18.8 s, matching the measured DP time of 19.6 s (M4 §7.1). Under Linux on the
same laptop a barrier costs about 1 µs and the problem disappears. So it is the runtime, not the
algorithm.

**Q11. Why is your speedup only ≈ 4.6× on 8 cores?**
Two reasons. First, the remaining stages are memory-bound: the seam removal reaches about 18 GB/s against
a measured copy bandwidth of 19.6 GB/s (calculation, FR §12.2), so more threads cannot help. Second,
the per-block barriers and the sequential backtracking remain. Note also that 2.4× of the 4.6× comes from
doing less work, which is also a valid optimization.

**Q12. Isn't comparing the optimized version against the original sequential program unfair?**
We report both views. Against the original sequential program (what the user gets): 4.4–4.8× with
8 threads. Against the optimized code on one thread (pure parallel scalability): 1.9–2.0×. The
optimizations O1, O2 and O5 would also speed up a sequential program; we kept the original sequential
version unchanged as the correctness baseline, as the project rules require (FR §11).

**Q13. Why are 16 threads slower than 8?**
The CPU has 8 cores; 16 threads means two hardware threads share one core's caches and execution units.
The stages are memory-bound, so the extra threads add no bandwidth, and every barrier waits for the
slowest thread. On 512 × 512 the times also became very unstable (0.043–0.337 s).

**Q14. What is Amdahl's law telling you here?**
In M2 with the DP sequential, the DP is ≈ 40 % of the time, so the speedup can never exceed ≈ 2.5×
(1/0.4), regardless of the thread count. The measured 1.4–1.8× is consistent. To go beyond, the DP itself
had to become parallel and cheap – that is what M5 did.

**Q15. How did you measure, and how reliable are the numbers?**
One warm-up run plus 5 measured runs per configuration (3 for 7680 × 4320), random order, median,
`carve()` only, output hash checked every run. Variation between runs is typically 1–5 %; we only call
a difference real if it is larger than that. Hardware, compiler, flags, commit and environment are stored
in `results/*.meta.json`.

**Q16. Why are M4 and M5 on different machines?**
M5 was developed on another team member's laptop where Windows Smart App Control blocks locally compiled
programs, so we used a Linux container. We therefore never compare numbers across machines; every
speedup uses a sequential run from the same session.

## Optimizations

**Q17. Why does the incremental energy update give exactly the same result?**
After removing the seam pixel at column *s*, a new column *x ≤ s − 2* has the same four neighbours as
before, and a column *x ≥ s + 1* has the same four neighbours shifted left by one (adjacent seams differ by
at most one column). Only columns *s − 1* and *s* can change. The test suite checks this after every seam
against a full recomputation (FR §13.2).

**Q18. Why did the blocked DP only help together with 16-bit energy and direction bytes?**
With 32-bit *M* the DP works on ≈ 16.6 MB per seam (1920 × 1080), more than the 8 MB L3 cache; the
threads then wait for memory. With 16-bit energy and 1-byte directions it is ≈ 6.2 MB and stays in the
cache. Measured: blocked DP alone 0.547 → 0.512 s; with compact memory 0.580 → 0.287 s.

**Q19. Which optimizations did you reject?**
The size threshold (no gain at ≤ 12 threads under Linux; kept as an option, off by default), non-static
schedules (within noise), `OMP_WAIT_POLICY` (no clear effect), parallel minimum and backtracking
(slower or impossible).

**Q20. What would you do next?**
Avoid physically shifting every pixel after every seam (the remaining bottleneck), measure natively on
Windows with another OpenMP runtime, confirm the memory-traffic calculations with hardware counters, and
try a GPU version of the blocked DP.
