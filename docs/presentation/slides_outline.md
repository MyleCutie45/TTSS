# Final Presentation – Slide Outline (14 slides, ≈ 15 minutes)

Every number on the slides comes from `docs/final_report.md` (sources: `results/*.csv`). Figures are in
`docs/figures/`. Suggested timing in brackets.

---

### 1. Title [0:30]
- "Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP"
- Team members, course, date.
- Visual: one input photo and its seam-carved version side by side (`tests/data/person_desert.png`).

### 2. What is seam carving? [1:00]
- Resize by removing the least important connected path of pixels, one at a time.
- Compared with scaling (distorts) and cropping (loses content).
- Visual: one image with the first few seams drawn in red; scaled vs. cropped vs. carved.
- Source: Avidan & Shamir, SIGGRAPH 2007.

### 3. The algorithm [1:30]
- Energy *e = |∂I/∂x| + |∂I/∂y|*.
- DP: *M(x,y) = e(x,y) + min(M(x−1,y−1), M(x,y−1), M(x+1,y−1))*.
- Minimum of last row → backtrack → remove → repeat *k* times. Cost Θ(k·W·H).
- Visual: the worked 4 × 4 example from `docs/algorithm.md` §5.

### 4. Where is the parallelism? [1:00]
- Seams are sequential (each depends on the previous image).
- Inside a seam: energy, removal = rows independent; minimum = reduction; backtracking = chain;
  DP = parallel only within a row, **one barrier per row**.
- Visual: Figure 1 of the final report (5-stage pipeline with the type of parallelism per stage).

### 5. Three versions, one answer [1:00]
- Sequential baseline (no OpenMP), basic OpenMP (M2), optimized OpenMP (M5).
- Integer arithmetic + fixed tie rule ⇒ bit-identical output for any thread count.
- Verification numbers: ≈ 237 000 test checks, 18 mutants detected, sanitizers clean, 1 905 / 1 905
  benchmark outputs identical.

### 6. Basic OpenMP implementation [1:00]
- Code excerpt: `parallel for` over rows (energy, removal); DP variant A (region per row).
- Synchronization table (Section 8 of the report): what races are possible, what prevents them.

### 7. Result 1: the parallel DP is slower than sequential [1:30]
- Platform A (i7-1065G7, Windows): all four stages parallel → 0.01–0.68×; more threads = slower.
- DP sequential → 1.40–1.79×.
- Visual: `docs/figures/speedup_vs_threads_main_nodp.png` or a two-bar comparison.

### 8. Why? Synchronization cost [1:30]
- One DP row = 1.84 µs of work; one barrier on Windows = 22–167 µs.
- 414 336 barriers × 45 µs ≈ 18.8 s ≈ measured 19.6 s.
- Same code under Linux: barrier ≈ 1 µs → parallel DP no longer slower.
- Amdahl: with the DP sequential (≈ 40 %), the maximum is ≈ 2.5×.
- Visual: `docs/figures/dp_time_per_row.png`.

### 9. Optimizations (M5) [1:30]
- O2 SIMD kernels, O1 incremental energy (only 2 values per row change), O5 compact memory
  (16-bit energy, 1-byte directions), O3 blocked DP (one barrier per 64 rows, halo cells).
- Each one switchable and measured separately.
- Visual: the trapezoid/halo diagram (Section 13.3 of the report).

### 10. Step-by-step effect [1:00]
- 2.160 s → 1.378 (M2) → 0.944 (+SIMD) → 0.850 (+incr) → 0.764 (+compact) → **0.479 s** (+blocked DP).
- Visual: `docs/figures/m5_ablation_1920x1080.png`.

### 11. Key insight: memory, not threads [1:00]
- Blocked DP alone: 0.547 → 0.512 s. With compact memory: 0.580 → 0.287 s.
- Working set 16.6 MB (> 8 MB L3) → 6.2 MB (< L3) (*calculation*).
- Visual: `docs/figures/m5_dp_block.png`.

### 12. Final speedup [1:00]
- 4.4–4.8× over sequential with 8 threads (Platform B, Ryzen 7 4800H), vs. 1.5–1.6× for M2.
- 2.4× of it already with one thread; 16 threads slower than 8.
- Visual: `docs/figures/m5_speedup_threads.png`.

### 13. Limitations and what we learned [1:00]
- Seam removal at ≈ 18 GB/s vs 19.6 GB/s copy bandwidth: memory-bound.
- Two machines/OSes; M5 in a Linux container; calculations not hardware counters.
- Lessons: measure before optimizing; synchronization granularity; data size matters as much as thread count.

### 14. Conclusion and questions [0:30]
- Correct parallel seam carving with bit-identical output.
- 4.4–4.8× speedup, explained stage by stage.
- Future: avoid physical pixel shifts, forward energy, GPU.
