# WORKLOG – Seam Carving OpenMP Project

## Project Information

**Project:** Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP

**Primary Language:** C++17

**Parallel Framework:** OpenMP

**Purpose:** Document all development activities, implementation decisions, experiments, bugs, and progress.

---

## Current Project Status

- Current milestone: M4 finished (branch `m4-benchmark`; M3 on `m3-testing`); next: M5
- Overall status: sequential + OpenMP versions bit-exact equal (M3; all 1191 benchmark runs also gave identical outputs); OpenMP synchronization overhead on Windows makes the parallel DP slow (M4: measured, not present under Linux); best measured speedup 1.79× (DP sequential, 7680×4320)
- Last updated: 2026-11-19

## Milestone Progress

| Milestone | Description                   | Status      |
| --------- | ----------------------------- | ----------- |
| M0        | Research and Architecture     | Completed (contract approved by Member A; Members B/C agreement not yet recorded) |
| M1        | Sequential Implementation     | Completed |
| M2        | OpenMP Implementation         | Completed (correct; parallel DP slower than sequential on this platform) |
| M3        | Correctness Testing           | Completed (2026-11-19, `docs/testing_report.md`) |
| M4        | Performance Benchmarking      | Completed (2026-11-19, `docs/benchmark_report.md`) |
| M5        | Performance Optimization      | Not started |
| M6        | Final Report and Presentation | Not started |

---

# Development History

> Append new entries below. Never delete previous entries.

## Entry Template

### [YYYY-MM-DD HH:MM] – [Task Name]

**Milestone:**

**Status:** Completed / In Progress / Blocked

#### 1. Objective

What was the purpose of this task?

#### 2. Work Performed

Describe exactly what was implemented or changed.

#### 3. Technical Explanation

Explain:

- How the implementation works.
- Important algorithms and formulas.
- Important functions and variables.
- Relevant OpenMP concepts.
- Important design decisions.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
|      |        |             |

#### 5. Commands Executed

```bash
# Record actual commands executed
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
|      |                 |               |        |

#### 7. Problems Encountered

Describe bugs, errors, limitations, and unresolved issues.

#### 8. Decisions and Reasoning

Explain important technical decisions and why alternatives were rejected.

#### 9. Remaining Tasks

List unfinished work.

#### 10. Next Step

Describe the recommended next action.

### [2026-11-13] – Initialize Git repository and publish to GitHub

**Milestone:** Setup (pre-M0)

**Status:** Completed

#### 1. Objective

Put project under version control and push to a public GitHub repository.

#### 2. Work Performed

- Ran `git init`, created `.gitignore`, made initial commit, created public repo `TTSS` on GitHub with `gh`, pushed `main`.

#### 3. Technical Explanation

No code yet. `.gitignore` excludes the 21 MB original paper PDF (copyrighted; `link.md` keeps its URL), future CMake build dirs, editor and Python cache files.

#### 4. Files Created or Modified

| File       | Action   | Description                                          |
| ---------- | -------- | ---------------------------------------------------- |
| .gitignore | Created  | Ignore paper PDF, build outputs, editor/Python files |
| WORKLOG.md | Modified | Added this entry                                     |

#### 5. Commands Executed

```bash
git init -b main
git add .
git commit -m "Initial commit: project plan and docs"
gh repo create TTSS --public --source=. --remote=origin --push
```

#### 6. Testing and Results

| Test           | Expected Result               | Actual Result                                                                                 | Status |
| -------------- | ----------------------------- | --------------------------------------------------------------------------------------------- | ------ |
| Push to GitHub | Remote `main` matches local | Commit 48ffa6a pushed to https://github.com/MyleCutie45/TTSS; `main` tracks `origin/main` | Passed |

#### 7. Problems Encountered

None.

#### 8. Decisions and Reasoning

- Public visibility chosen by user.
- Paper PDF excluded: copyrighted, large binary; URL in `link.md` suffices.

#### 9. Remaining Tasks

- All milestones M0–M6.

#### 10. Next Step

Start Milestone 0.

### [2026-11-13] – Write prerequisite textbook (HTML, Vietnamese)

**Milestone:** M0 – Research and Architecture

**Status:** Completed (first version)

#### 1. Objective

Give the student a self-study textbook, from zero to the full topic: math background, the idea of Seam Carving, an analysis of the Avidan & Shamir 2007 paper, and how to parallelize it.

#### 2. Work Performed

Created `docs/seam_carving_textbook.html`, a single self-contained HTML file with 18 chapters in 4 parts, plus a glossary and references:
- Part I: images as matrices, row-major layout, grayscale, finite differences, gradient, Sobel, DAG/shortest path, DP, Big-O.
- Part II: seam definition, energy e1/eHoG, DP recurrence with a worked 4×5 example, seam removal, enlarging, object removal.
- Part III: the paper section by section, a notation mapping table, critique, follow-up work (forward energy 2008).
- Part IV: OpenMP basics, race conditions, Amdahl, a step-by-step parallelism analysis (energy / DP row barrier / backtrack / removal), project roadmap.
- 3 interactive demos: step-by-step DP, live seam carving in the browser compared with scaling, Amdahl calculator.

#### 3. Technical Explanation

The demos are vanilla JS on canvas. Energy is e1 with central differences and clamped borders; DP tie-breaking favours the middle cell, then left, then right. No external JS libraries; formulas are written in HTML, so the file works offline except for the web fonts.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/seam_carving_textbook.html | Created | Vietnamese textbook + 3 demos |
| WORKLOG.md | Modified | Added this entry |

#### 5. Commands Executed

```bash
node --check <script extracted from the HTML>   # JS syntax check
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| JS syntax | No error | `SYNTAX_OK` | Passed |
| DP worked example (by hand) | Seam [1,1,0,1], cost 10 | Hand-computed M rows: [3,1,4,1,5], [10,3,7,6,4], [8,11,12,11,13], [11,10,14,19,15] | Passed |
| Demos in a browser | Render and run | Not run in a browser during this session | Not tested |

#### 7. Problems Encountered

None. The paper analysis was written from knowledge of the paper; the PDF was not parsed in this session, so check the section/figure correspondences against the PDF.

#### 8. Decisions and Reasoning

- HTML/Unicode formulas instead of KaTeX: keeps the file self-contained and working offline.
- The textbook recommends e1, integer energy and a fixed tie-breaking rule, so the sequential and parallel versions can be compared pixel by pixel.

#### 9. Remaining Tasks

- Open the file in a browser and test the 3 demos.
- M0: settle on the energy function, border handling, tie-breaking and data type.

#### 10. Next Step

Student reads the textbook and does the exercises in chapter 18, then settles the M0 design decisions.

### [2026-11-13] – Check textbook against the paper PDF, fill gaps, fix errors

**Milestone:** M0 – Research and Architecture

**Status:** Completed

#### 1. Objective

The student asked whether the textbook covers everything in the paper. The first version was written from memory and had not been checked against the PDF.

#### 2. Work Performed

- Extracted the PDF text with `pdftotext -layout` (to the scratchpad, not committed) and read all of it: Sections 1–7 and Appendix A.
- Rewrote Chapter 14 to follow the paper's own section numbering: a map of Figures 1–17, a Background table, the 5 strategies (Fig. 2), the ≤k generalization, 3.1 Energy Preservation Measure, 3.2 energy functions, 4.1 (enlarging rows as an alternative), 4.2 equations (5)–(6) with a note on the notation, 4.3 time evolution I^(t), 4.4–4.6, 5 Multi-size (index maps, negative indices, the 2.2 s figure), Appendix A (consistent index maps, diagonal proof, Hungarian), 6, 7 future work, and a coverage table.
- Fixed errors in the first version:
  1. eHoG intuition was wrong (I had said it "lets seams cross texture"). The paper says: the denominator attracts seams to edges, and the numerator keeps the seam parallel to the edge instead of crossing it.
  2. "Negative energy for object removal" was attributed to the paper. The paper only says seams are removed until no marked pixels remain; negative energy is now labelled as a common implementation technique.
- Smaller fixes: the contributions list now matches the paper's 4; the Photoshop remark is marked as outside the paper; the critique adds the thin evaluation and the notation issues; added references (Pérez, Dalal & Triggs, Itti, Kuhn).

#### 3. Technical Explanation

Equation (6) as printed in the paper pairs T(r−1,c) with s^x and has off-by-one image sizes, which contradicts the paper's own "left = vertical, top = horizontal". The textbook uses the consistent reading and explains the discrepancy. The Appendix A proof is restated more rigorously (Δrow ≤ Δcol and Δcol ≤ Δrow ⇒ square, diagonal).

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/seam_carving_textbook.html | Modified | Ch. 14 rewritten; Ch. 9, 12, 13, 15 and references corrected |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```bash
pdftotext -layout seamcarving_original_paper.pdf <scratchpad>/paper.txt
python splice: replace <section id="c14"> ... </section>
node --check <extracted script>
grep counts for section/tr/table balance
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| JS syntax | OK | `SYNTAX_OK` | Passed |
| HTML tag balance | open = close | section 21/21, tr 144/144, table 20/20 | Passed |
| Browser render | — | Not run | Not tested |

#### 7. Problems Encountered

The `Read` tool cannot render the PDF (pdftoppm is missing), so `pdftotext` was used instead. Figures were not viewed; only the captions were read. Some formulas came through pdftotext with garbled symbols and were reconstructed from context.

#### 8. Decisions and Reasoning

The paper's exact details that are unclear (Appendix A edge weights, ≥/> off-by-one in the multi-size gathering) are marked "không rõ / tự kiểm tra" rather than guessed.

#### 9. Remaining Tasks

- Test the demos in a browser.
- Commit and push (not done yet).

#### 10. Next Step

Settle the M0 design decisions.

---

### [2026-11-13] – Expand the textbook into a dedicated book (Parts V–VII, ch. 19–30)

**Milestone:** M0 – Research and Architecture

**Status:** Completed

#### 1. Objective

The student wants a dedicated book with enough knowledge to do the project without gaps, and asked to prioritise **adding** content over rewriting.

#### 2. Work Performed

Kept chapters 1–18 as they were; added 12 new chapters and 2 demos:
- **Part V – From theory to code:** 19 data design (int vs float, overflow bound, fixed stride + logical width, invariants, transposing for horizontal seams); 20 OpenCV (cv::Mat, BGR, ptr/step, shallow-copy trap, internal thread pool); 21 CMake + OpenMP on Windows (sample CMakeLists, MSVC OpenMP 2.0 limits, `_OPENMP` check, OMP_NUM_THREADS); 22 sequential baseline (DP proof by induction, parent array, local energy update derivation, seam insertion via origCol).
- **Part VI – In-depth parallelism:** 23 work–span, Brent, Gustafson, memory-bound; 24 advanced OpenMP (single parallel region, memory model/flush, argmin without reduction(min) for OpenMP 2.0, `if` clause, false sharing, SIMD); 25 trapezoid tiling to reduce barriers (with a correctness argument); 26 forward energy (Rubinstein 2008).
- **Part VII – Testing and benchmarking:** 27 correctness testing (invariants, brute-force oracle, seq↔par equivalence, race hunting); 28 benchmark methodology (+ CSV format, matplotlib script, empty results table); 29 common bugs; 30 summary + 10 defense questions.
- Demo 2: added a "forward energy" option (DP now uses a parent array). New Demo 4: interactive trapezoid tiling.
- Small edits: TOC, title, introduction, cross-reference in 17.4 to the fixed-stride approach, ch. 18 lookup table by milestone, glossary (+9 terms), references (+4).

#### 3. Technical Explanation

- Trapezoid tiling: phase 1 per block computes row t over columns [b_k+t, b_{k+1}−1−t] (no shrinking at the image edges); phase 2 computes triangles [b−t, b+t−1] at each boundary. 2 barriers per strip instead of R. Requires B ≥ 2R.
- Argmin for OpenMP 2.0: local min per thread plus a merge in `critical` by (value, index), which gives the same result as a sequential left-to-right scan with strict `<`.
- Local energy update: band [min(s_{i−1},s_i,s_{i+1})−1, max(...)] for central differences (conservative); marked as needing a test against full recomputation.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/seam_carving_textbook.html | Modified | +Parts V–VII (ch. 19–30), Demo 4, forward-energy option in Demo 2, TOC/intro/glossary/references |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```bash
python splice: insert Parts V–VII before <hr><section id="thuat-ngu">
node <scratchpad>/verify.js          # independent algorithm checks
node --check <extracted script>
grep tag-balance + anchor check
msedge --headless=new --dump-dom <scratchpad>/test_page.html   # test copy with injected test script
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Trapezoid: each cell computed once, parents ready under barrier semantics | All configs OK | 24/24 configs (W∈{24,30,40,64,100}, 2–8 blocks, every R with B≥2R) | Passed |
| DP (backward + forward energy) vs brute-force enumeration on random 1–6 × 1–6 grids | min DP = brute force = seam cost | 600/600 | Passed |
| Ch. 10 example | last row [11,10,14,19,15] | [11,10,14,19,15] | Passed |
| JS syntax | OK | SYNTAX_OK | Passed |
| HTML balance + internal anchors | Balanced, no missing anchors | section 33/33, tr 215/215, table 30/30, pre 25/25, details 15/15; 0 missing anchors | Passed |
| Headless Edge: Demo 1 run to the end | seam [1,1,0,1], cost 10 | Matches | Passed |
| Headless Edge: Demo 2 backward/forward | width decreases 1 per click | 360→355 (backward ×5), reset, 360→353 (forward ×7) | Passed |
| Headless Edge: Demo 4 R=1..5 | phase 2 = 2R(R+1) cells, total = 30R | 4/12/24/40/60; total matches | Passed |
| Headless Edge: JS errors | None | `[]` | Passed |

#### 7. Problems Encountered

- In the first headless run, a `\n` in the injected test script was turned into a real newline and broke the JS string. Fixed using `String.fromCharCode(10)`. This only affected the test copy in the scratchpad, not the textbook.
- Headless virtual time does not run the requestAnimationFrame animations (the "Xóa 20 seam" button and the slider). Those animations were not tested automatically; only the synchronous path (Xóa 1 seam) was.
- The visual layout (new chapters, Demo 4 on mobile) was not checked by screenshot.
- The MSVC `/openmp:llvm` details depend on the version installed; the book tells the student to check the MSVC documentation.

#### 8. Decisions and Reasoning

- Added content instead of rewriting, as the student asked. Chapters 1–18 only got cross-references.
- The book recommends sticking to OpenMP 2.0 features for the core, because the student is on Windows and the compiler is not yet known.
- All performance claims in the book are qualitative or come from theory; there are no measured numbers. The benchmark table is left empty.

#### 9. Remaining Tasks

- Manually check the animations (20 seams, slider) and the mobile layout.
- Commit/push (not done).
- M0: choose the compiler and record the M0 design decisions.

#### 10. Next Step

Student reads Parts V–VII, installs the toolchain (ch. 21), checks the `_OPENMP` version, and settles the M0 decisions.

---

### [2026-11-13] – Review M0–M6 and write the project plan; environment check

**Milestone:** M0 – Research and Architecture

**Status:** In Progress (plan written; blocked on toolchain decision D1)

#### 1. Objective

Read all milestone files, check the machine's toolchain, and produce a work plan.

#### 2. Work Performed

- Read M0.md–M6.md in full.
- Checked the toolchain and hardware.
- Wrote `docs/project_plan.md`: environment, blockers, gaps per milestone, proposed architecture (5 variants via per-stage flags), tasks and exit criteria per milestone, a benchmark budget estimate, and decisions D1–D6.

#### 3. Technical Explanation

Notable findings in the milestone files:
- M0 requires 5 variants, so the design uses per-stage seq/omp selection.
- M3 requires inspecting intermediate matrices, so stage functions take caller-provided buffers.
- M2 uses M(x,y) = (col,row) notation.
- README.md does not exist (READ.md is a different file).

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/project_plan.md | Created | Plan + environment + decisions |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```bash
cat M0.md ... M6.md
g++ --version; python -c "import matplotlib, pandas, numpy"; python -c "import cv2"
Get-CimInstance Win32_Processor / Win32_ComputerSystem
g++ -std=c++17 -O2 -fopenmp omp.cpp -o omp.exe   # in scratchpad
omp.exe ; cmake --version                       # inside and outside the sandbox
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| g++ available | version | g++ 16.1.0 MinGW-w64 (WinLibs UCRT) | OK |
| Compile OpenMP test | exit 0 | exit 0 (outside sandbox; inside the sandbox the LTO plugin DLL failed to load) | OK |
| Run OpenMP test exe | prints _OPENMP | exit code -1058471934 (0xC0E90002), no output | **Failed** |
| cmake --version | version | exit code -1058471934, no output | **Failed** |
| OpenCV C++ | found | not found | **Missing** |
| Python libs | present | matplotlib 3.11.0, pandas 3.0.6, numpy 2.4.6 | OK |
| Hardware | — | Ryzen 7 4800H, 8C/16T, L2 4096 KB, L3 8192 KB, 23.4 GB RAM | Recorded |

#### 7. Problems Encountered

- Blocker B1: unsigned executables are blocked (0xC0E90002), most likely Windows Smart App Control. Not yet confirmed in Windows Security settings.
- Blocker B2: OpenCV for C++ is not installed.
- `_OPENMP` version unknown until a program can run.

#### 8. Decisions and Reasoning

WSL2 Ubuntu is recommended: it avoids B1, makes OpenCV available via apt, and gives sanitizers for M3. This is pending the student's approval (D1).

#### 9. Remaining Tasks

- Answers to D1–D6.
- M0 deliverables: algorithm.md, architecture.md, parallelization_strategy.md, benchmark_plan.md.

#### 10. Next Step

Student decides D1 (toolchain). Then set up the toolchain, verify OpenMP, and write the M0 docs.

---

### [2026-11-13] – M0 documentation written; decisions D1–D6 approved

**Milestone:** M0 – Research and Architecture

**Status:** In Progress. The documents are written and waiting for the student's review. The toolchain (Ubuntu in WSL) is not yet installed.

#### 1. Objective

Produce the 4 deliverables required by M0 §6, without implementation code (M0 §5).

#### 2. Work Performed

- The student approved all recommendations (D1–D6; D5, the deadline, is still unknown).
- Checked WSL: WSL2 is present (default version 2), and the only distro is `docker-desktop` (stopped). An Ubuntu distro is needed.
- Wrote 4 documents:
  - `docs/algorithm.md`: notation incl. mapping to the paper and M2, energy discretization, seam definition, DP with a correctness proof, tie rules, main loop, height via transpose, worked example, complexity and memory, out-of-scope list.
  - `docs/architecture.md`: toolchain, directory layout, `ImageBuffer` (fixed stride), stage signatures, the `Variant` table (seq, omp-energy, omp-dp, omp-remove, omp-min, omp), carver, CLI, data flow, invariants, test layout.
  - `docs/parallelization_strategy.md`: dependency table, work–span, code sketch + sync + race analysis per stage (energy, DP in two modes, deterministic argmin, removal), sync point table, race table, determinism, thread and schedule control, what stays sequential, bottleneck hypotheses.
  - `docs/benchmark_plan.md`: unit tests U1–U10, equivalence tests, sanitizers, test images, benchmark matrix, measurement method (warm-up, median, per-phase), environment record, run conditions, CSV schema, plots, M5 protocol.
- Recorded the decisions in the Technical Decision History.

#### 3. Technical Explanation

Key design points:
- Integer data everywhere, so exact equality is the correctness criterion.
- Fixed stride, so in-place row shifting is race-free.
- Stage functions write into caller buffers, so M3 can compare the intermediate energy and M matrices.
- `schedule(runtime)` for scheduling experiments.
- Argmin merges by (value, index) for a deterministic smallest-index tie rule.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/algorithm.md | Created | M0 deliverable |
| docs/architecture.md | Created | M0 deliverable |
| docs/parallelization_strategy.md | Created | M0 deliverable |
| docs/benchmark_plan.md | Created | M0 deliverable |
| docs/project_plan.md | Modified | Decisions marked approved |
| WORKLOG.md | Modified | This entry + decision history |

#### 5. Commands Executed

```powershell
wsl --status ; wsl -l -v
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| WSL present | WSL2 | Default version 2; distro `docker-desktop` only (stopped) | Ubuntu missing |
| Arithmetic in the docs (memory, M bound, barrier count) | Consistent | 1080p ≈ 25 MB; 8K ≈ 400 MB; M ≤ 4320·510 = 2 203 200; 576·1079 ≈ 620k barriers | Checked by hand |

No code yet (M0), so there are no build or test runs.

#### 7. Problems Encountered

- Blocker B1/B2 remain until Ubuntu is installed in WSL and OpenCV is installed.
- `_OPENMP` version is still unknown.

#### 8. Decisions and Reasoning

See Technical Decision History (2026-11-13 rows).

#### 9. Remaining Tasks

- Student: `wsl --install -d Ubuntu-24.04`, create a user.
- Then: `sudo apt install build-essential cmake libopencv-dev`, verify OpenMP (`_OPENMP`, team size), record the versions.
- Student reviews the 4 M0 documents; then M1 can start.
- D5 (deadline) still to be provided.

#### 10. Next Step

Install Ubuntu in WSL → verify the toolchain → student approves M0 → start M1.

---

### [2026-11-15] – Session start: project inspection and environment re-check

**Milestone:** M0 – Research and Architecture

**Status:** Completed (inspection only; no code changes)

#### 1. Objective

Re-read project state before continuing work.

#### 2. Work Performed

- Read CLAUDE.md, READ.md, WORKLOG.md, M0–M6, outlines of the 5 docs in `docs/`.
- Re-checked WSL and native toolchain.

#### 3. Technical Explanation

No code. Found: `docs/` (M0 deliverables + plan + textbook) and the last WORKLOG entries are **not committed** yet (`git status`: `?? docs/`, `M WORKLOG.md`).

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```bash
wsl.exe -l -v
g++ -std=c++17 -O2 -fopenmp omp.cpp -o omp.exe   # scratchpad
./omp.exe
cmake --version
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| WSL Ubuntu installed | Ubuntu distro listed | Only `docker-desktop` (Stopped) | **Missing** |
| Native OpenMP compile | exit 0 | exit 0 | OK |
| Native OpenMP run | prints `_OPENMP` | `Permission denied` (exit 126) | **Failed** (B1 still active) |
| cmake --version | version | no output | **Failed** |

#### 7. Problems Encountered

Blockers B1 (unsigned exe blocked) and B2 (no OpenCV) unchanged. M1 cannot be built/run until Ubuntu WSL is installed.

#### 8. Decisions and Reasoning

None new.

#### 9. Remaining Tasks

- Student: install Ubuntu 24.04 in WSL.
- Commit `docs/` + WORKLOG.
- Student review of M0 docs; D5 deadline still unknown.

#### 10. Next Step

Install Ubuntu WSL → apt toolchain → verify OpenMP → start M1.

---

# Technical Decision History

Record important architecture and algorithmic decisions here.

| Date | Decision | Reason | Alternatives |
| ---- | -------- | ------ | ------------ |
| 2026-11-13 | D1: Build/run in WSL2 Ubuntu 24.04 (g++, CMake, apt OpenCV) | Native Windows blocks our unsigned executables (0xC0E90002); OpenCV via apt; sanitizers available | Native Windows with Smart App Control off (irreversible) + OpenCV built for MinGW |
| 2026-11-13 | D2: Energy = e1, central differences without division, integer gray `(299R+587G+114B+500)/1000`, clamped borders; tie rule: last-row argmin = smallest index, parent = middle > left > right | Integer data makes seq/omp exactly comparable; matches the paper's e1 | Sobel; float energy; per-channel energy |
| 2026-11-13 | D3: Test images = seeded synthetic + 2–3 CC0/public-domain photos | Reproducible + realistic | Only synthetic / only photos |
| 2026-11-13 | D4: Minimal custom test runner + CTest, no external test framework | No extra dependency | doctest / Catch2 |
| 2026-11-13 | D6: Seam insertion, forward energy, optimal order are out of scope | Not required by M0–M6 | Implement as extensions if time remains |
| 2026-11-13 | Architecture: fixed stride + logical width; seq/omp in separate files with identical signatures; per-stage `Variant` flags give the 5 required variants | Race-free in-place removal, no reallocation, per-stage benchmarks | Separate full pipelines per variant (code duplication) |
| 2026-11-13 | Height reduction: width first, then transpose + width carving + transpose back | One code path to test; contiguous memory access | Separate horizontal-seam code; optimal order (transport map) |

# Known Issues

| Issue | Description | Priority | Status |
| ----- | ----------- | -------- | ------ |
|       |             |          |        |

# Benchmark History

Record actual benchmark experiments.

| Date | Image Size | Version | Threads | Execution Time | Notes |
| ---- | ---------- | ------- | ------- | -------------- | ----- |
|      |            |         |         |                |       |

**Important:** Only record experimentally measured values. Never insert estimated or fabricated results.

# Learning Notes

Record important concepts learned during development.

Examples:

- Dynamic Programming dependencies.
- OpenMP parallel for.
- OpenMP reduction.
- Synchronization and barriers.
- Race conditions.
- Cache locality.
- Speedup and parallel efficiency.
- Amdahl's Law.

---

# Current Development Summary

This section must always reflect the latest project state.

**Completed:**

- Git repo + GitHub (public, MyleCutie45/TTSS)
- Prerequisite textbook `docs/seam_carving_textbook.html`
- M0 docs written: algorithm.md, architecture.md, parallelization_strategy.md, benchmark_plan.md, project_plan.md (not yet committed)

**In Progress:**

- M0 – Research and Architecture (waiting for student review)

**Blocked:**

- Toolchain: native Windows blocks our executables (B1); OpenCV missing (B2); Ubuntu not yet installed in WSL (as of 2026-11-15)

**Next Recommended Task:**

- Install Ubuntu 24.04 in WSL, install build-essential/cmake/libopencv-dev, verify OpenMP, then start M1.

### [2026-11-17 08:10] – Toolchain setup, smoke test and test images (work done 2026-11-16)

**Milestone:** Setup (pre-M0)

**Status:** Completed

#### 1. Objective

Prepare a modern C++17 + OpenMP + OpenCV toolchain that all three team members can install identically, and collect test images. This entry records preparation work done on 2026-11-16 (during planning no repo files were edited, so it is logged now).

#### 2. Work Performed

- Chose MSYS2 **UCRT64** as the common toolchain (installed at `C:\tools\msys64`).
- Installed GCC, CMake, Ninja and OpenCV from MSYS2 packages.
- Put `C:\tools\msys64\ucrt64\bin` before the old `C:\MinGW\bin` (GCC 6.3, 32-bit) in the system PATH.
- Built and ran a smoke-test program (in a scratch folder, not in the repo).
- Downloaded 5 Unsplash photos into `images/original/` (ignored by git) and created 800-px PNG copies in `tests/data/`.
- Wrote `images/SOURCES.md` (link, photographer, original size, purpose of each image).
- Added `images/original/` to `.gitignore`.

#### 3. Technical Explanation

- **Why not the old compiler:** GCC 6.3 (MinGW.org) is 32-bit and has incomplete C++17 support; 32-bit processes cannot use enough memory for 8K images.
- **Why MSYS2 instead of Visual Studio:** MSVC only supports OpenMP 2.0 by default; MSYS2 installs GCC + OpenCV with one `pacman` command, so all members get the same versions.
- **Why PNG test copies:** PNG is lossless, so every decoder produces exactly the same pixels. This is required for bit-exact comparison of outputs in M1–M3.

Installed versions (checked again on 2026-11-17 with `g++ --version`, `cmake --version`, `ninja --version`):

| Component | Version |
| --------- | ------- |
| GCC (MSYS2 UCRT64) | 16.2.0 (Rev7) |
| CMake | 4.5.0 |
| Ninja | 1.13.2 |
| OpenCV | 5.0.0 (`mingw-w64-ucrt-x86_64-opencv`) |
| OpenMP | 5.2 (`_OPENMP = 202111`) |
| CPU | Intel Core i9-13900H, 14 cores (6 P + 8 E), 20 logical processors |
| RAM | ~15.6 GB |
| OS | Windows 11 Home 10.0.26200 |

Test images (`tests/data/`, sizes measured with Pillow):

| File | Size (W x H) | Case |
| ---- | ------------ | ---- |
| boat_sea.png | 800 x 600 | Easy |
| tree_field.png | 800 x 533 | Easy |
| person_desert.png | 800 x 533 | Easy |
| building_windows.png | 533 x 800 | Hard (straight lines) |
| face_portrait.png | 800 x 450 | Hard (face) |

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| .gitignore | Modified | Ignore `images/original/` (large source photos) |
| images/SOURCES.md | Created | Image sources and purposes |
| tests/data/*.png (5 files) | Created | 800-px lossless test images |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```bash
# MSYS2 UCRT64 shell
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-opencv
# Smoke test (scratch folder outside repo)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Version check (2026-11-17)
g++ --version; cmake --version; ninja --version
```

The exact Pillow resize script was not saved (scratch work); method: longest side 800 px, Lanczos resampling.

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| C++17 | `__cplusplus == 201703` | 201703 | Passed |
| OpenMP | All logical processors used | 20 threads ran | Passed |
| OpenCV PNG round trip | imwrite → imread gives identical pixels | Identical | Passed |
| OpenCV `cvtColor` to gray | Type `CV_8UC1` | `CV_8UC1` | Passed |
| Smoke test with new system PATH | Builds and runs | Passed (run twice) | Passed |

#### 7. Problems Encountered

- ThreadSanitizer (race detector) does not work with MinGW on Windows. Races must be found with repeated seq-vs-omp comparison tests (M2/M3).
- New finding (2026-11-17): because the MSYS2 `ucrt64\bin` folder is first in PATH, the command `python` starts **MSYS2's Python**, which has no Pillow/numpy/matplotlib. Use the full path of another Python installation (e.g. conda) for Python scripts. Recorded in Known Issues.
- The old GCC 6.3 in `C:\MinGW` is still installed (not used).

#### 8. Decisions and Reasoning

See Technical Decision History (toolchain row).

#### 9. Remaining Tasks

- Members B and C install the same toolchain and report versions.

#### 10. Next Step

Milestone 0 documentation.

### [2026-11-17 08:45] – M0: Research and architecture documents

**Milestone:** M0

**Status:** Completed on 2026-11-17 (see update at the end of this entry). Members B and C have not yet confirmed the contract.

#### 1. Objective

Complete the M0 tasks from `M0.md` without writing implementation code: explain the algorithm and its
complexity, analyse dependencies, design the OpenMP strategy, identify races and synchronization points,
propose the directory structure and interface contract, and design the benchmark methodology.

#### 2. Work Performed

- `docs/algorithm.md`: paper summary (energy `e1`, seams, DP, removal, height via transpose), exact
  pseudocode of our variant, a hand-computed 4x4 example (energy, `M`, endpoint, backtrack with a tie,
  image after removal), a uniform-image example (all ties), implementation decisions D1–D7, complexity.
- `docs/parallelization_strategy.md`: OpenMP glossary, per-stage dependency analysis, directives, DP
  variant A (parallel for per row) and DP variant B (persistent region), the `nowait` race, deterministic
  `(value, column)` minimum reduction, why backtracking stays sequential (Amdahl), synchronization point
  table S1–S4, race checklist, bottleneck hypotheses.
- `docs/architecture.md`: directory structure, data types, function signatures (`seq` / `omp_impl`),
  `StageTimings` and `ParallelConfig`, CLI and CSV contract, data flow, build system, team workflow.
- `docs/benchmark_plan.md`: metrics, configurations from `M4.md`, image preparation, methodology
  (warm-up, ≥5 runs, median), hybrid-CPU notes, output format, plots, hypotheses.

#### 3. Technical Explanation

- **Parallel stages:** energy (independent pixels, parallel over rows), DP (rows sequential, columns
  parallel, barrier after each row), minimum endpoint (per-thread local minimum + sequential combine),
  seam removal (independent rows, `memmove`). **Sequential:** the outer seam loop (seam `i+1` needs the
  image after seam `i`) and backtracking (each row depends on the row below).
- **Key synchronization:** the implicit barrier at the end of `omp for` in DP; removing it (`nowait`)
  would let a thread read row `y` before it is complete.
- **Determinism:** integer energy/`M` (int32) and the "smallest column wins" tie rule; the parallel
  minimum compares `(value, column)` pairs, which is a total order, so the result does not depend on
  thread count or schedule.
- **4x4 example verification:** values were computed by hand and re-checked with a small independent Python
  script in the session scratch folder (not part of the repo). Both agree: seam `[1, 2, 1, 0]`, cost 140.
- **CSV detail (new, proposal):** `schedule` and `stages` values contain commas in natural form, so they
  are written with `:` and `+` (e.g. `dynamic:64`, `energy+dp+min+removal`); DP variant B is written
  `dp_persistent`. The 15 column names agreed during planning are unchanged.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/algorithm.md | Created | Algorithm, pseudocode, 4x4 example, decisions, complexity |
| docs/parallelization_strategy.md | Created | OpenMP design per stage, synchronization, races |
| docs/architecture.md | Created | Directory layout and interface contract (draft for review) |
| docs/benchmark_plan.md | Created | Preliminary benchmark methodology (M4 completes it) |
| WORKLOG.md | Modified | This entry, status tables, decisions, known issues |

#### 5. Commands Executed

```bash
g++ --version; cmake --version; ninja --version      # 16.2.0 / 4.5.0 / 1.13.2
git fetch origin; git status -sb                     # main up to date with origin/main
python ex4.py                                        # scratch check of the 4x4 example (outside repo)
<conda python> -c "...Pillow..."                     # measured test image sizes
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| 4x4 example cross-check (hand vs script) | Same energy, `M`, seam | Identical; seam `[1,2,1,0]`, cost 140 | Passed |
| Uniform 4x4 example | Seam `[0,0,0,0]` | `[0,0,0,0]` | Passed |

No C++ code exists yet, so no build or unit tests were run.

#### 7. Problems Encountered

- None blocking. The paper does not specify the discrete gradient, border handling, or tie-breaking;
  these are documented as our decisions D1–D7 in `docs/algorithm.md`.

#### 8. Decisions and Reasoning

Recorded in the Technical Decision History table below. Member A approved all proposals on 2026-11-17
(including the CSV `:`/`+` encoding and exit codes 1/2); agreement of Members B and C not yet recorded.

#### 9. Remaining Tasks

- Members B and C review `docs/architecture.md` and the other M0 documents; agree on the contract.
- After agreement: mark decisions as "frozen", set M0 to Completed.
- Commit M0 documents + test images + `images/SOURCES.md` + `.gitignore` + `WORKLOG.md` (after user approval).

#### 10. Next Step

Start M1 (Member A approved the M0 proposals; review by Members B and C still pending).

### [2026-11-17 10:30] – M1: Sequential seam carving implementation

**Milestone:** M1

**Status:** Completed

#### 1. Objective

Implement the complete sequential algorithm (`M1.md`) exactly as defined in `docs/algorithm.md`, with
the API of `docs/architecture.md`, as the correctness baseline for M2/M3.

#### 2. Work Performed

- `CMakeLists.txt`: C++17, Release `-O2 -DNDEBUG -Wall -Wextra`, OpenCV (core, imgproc, imgcodecs);
  static library `seam_seq` (no OpenMP flags), program `seam_carving`, test program `test_seq`, 5 CLI tests.
- `include/seam_common.h`: `StageTimings`, `ParallelConfig`, timing helper on `std::chrono::steady_clock`.
- `include/seam_seq.h`, `src/seam_seq.cpp`: `computeEnergy`, `computeCumulative`, `findMinEndpoint`,
  `backtrackSeam`, `removeSeam`, `carve` (width with vertical seams, then height via transpose).
- `src/main.cpp`: CLI with the agreed options, `--csv` line, exit codes 1/2. `--mode omp` returns
  exit code 1 with "not implemented yet (M2)".
- `tests/test_seq.cpp`: 12 test functions, 65 checks.
- `README.md`: installation, build, test, run, options, CSV format.

#### 3. Technical Explanation

- **Energy:** for each row, pointers to rows `y-1`, `y`, `y+1` (clamped) are taken once; for each
  column, left/right neighbours are clamped to `width-1` (current width, not `gray.cols`).
- **DP:** row 0 copied with `memcpy`; each next row reads only the previous row. `min` over the 2–3
  existing upper neighbours.
- **Endpoint / backtrack:** strict `<` comparisons, candidates visited in increasing column order →
  smallest column wins every tie (decision D5).
- **Removal:** per row, `memmove` the pixels right of the seam one position left, in the color image
  (3 bytes/pixel) and the gray image (1 byte/pixel). `memmove` because source and destination overlap.
- **carve:** checks arguments (throws `std::invalid_argument`), clones the input, `carveWidth` converts to
  gray once, allocates energy/`M`/seam once, loops `width > targetWidth`, crops with
  `colRange(0, targetWidth).clone()`. Height: `cv::transpose` → `carveWidth` → transpose back. The gray
  image for the height pass is converted from the already carved color image; since gray conversion is
  per-pixel, this equals carving the gray image.
- **Timing:** each stage measured with `steady_clock` and summed; `total` covers the whole `carve()`.
- **Tests use a custom `CHECK` macro, not `assert`:** Release defines `NDEBUG`, which disables `assert`.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| CMakeLists.txt | Created | Build, tests |
| include/seam_common.h | Created | Shared types and timer |
| include/seam_seq.h | Created | Sequential API |
| src/seam_seq.cpp | Created | Sequential implementation (no OpenMP) |
| src/main.cpp | Created | Command-line program |
| tests/test_seq.cpp | Created | Unit tests |
| README.md | Created | User documentation (READ.md unchanged) |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
.\build\test_seq.exe
# exit-code checks (no args, --width 801, missing input, --mode omp, unwritable output)
# each of the 5 test images, width -20 %, run twice, compare SHA-256 of outputs:
.\build\seam_carving.exe --input tests\data\<name>.png --output out\<name>_seq_a.png --width <0.8*W> --csv
.\build\seam_carving.exe --input tests\data\person_desert.png --output out\person_desert_wh.png --width 600 --height 400
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build (Release, `-Wall -Wextra`) | No errors, no warnings | No errors, no warnings | Passed |
| `ctest` | All pass | 6/6 passed (test_seq + 5 CLI tests) | Passed |
| `test_seq` checks | 0 failed | 65 checks, 0 failed (12/12 test functions PASS) | Passed |
| Exit codes | 1,1,2,1,2 | no args 1, width 801 1, missing file 2, mode omp 1, bad output path 2 | Passed |
| Determinism, 5 real images × 2 runs | Identical files | SHA-256 identical for all 5 | Passed |
| Output sizes (−20 % width) | 640/426 wide, height unchanged | 640×600, 640×533, 640×533, 426×800, 640×450 | Passed |
| Width + height (800×533 → 600×400) | 600×400 | 600×400, 200 vertical + 133 horizontal seams | Passed |
| Visual check (boat_sea, building_windows, face_portrait) | Main objects preserved | Boat and face unchanged, sea/background narrowed; building lines remain straight | Passed (subjective) |
| Mutation check: backtrack `<` → `<=` | Tests fail | 2 checks failed (4x4 tie, uniform image) | Passed (tests detect bug) |
| Mutation check: clamp with `gray.cols` | Tests fail | 2 checks failed (current width test) | Passed (tests detect bug) |

After the mutation check the original source was restored and all tests passed again.
`src/seam_seq.cpp` contains no `#pragma omp`.

Single-run timings printed by the CLI (one run each, not a benchmark; i9-13900H, Release `-O2`):
boat_sea 800×600 → 640: `t_total` 0.227 s; building_windows 533×800 → 426: 0.141 s. In every run,
energy + DP take > 90 % of the total. These are informal observations; real measurements are M4's task.

#### 7. Problems Encountered

- None.

#### 8. Decisions and Reasoning

- `CMAKE_CXX_FLAGS_RELEASE` overridden to `-O2 -DNDEBUG` (CMake's default for GCC is `-O3`) to follow the contract.
- `carve()` requires `CV_8UC3`; the CLI loads with `IMREAD_COLOR` so gray PNGs are expanded to 3 channels.
- `--mode omp` is parsed but rejected until M2, so the CLI format is already final.

#### 9. Remaining Tasks

- M2 (OpenMP). Members B/C review of the contract still not recorded.

#### 10. Next Step

M2: `include/seam_omp.h`, `src/seam_omp.cpp`, equivalence tests.

### [2026-11-17 15:30] – M2: OpenMP implementation and equivalence tests

**Milestone:** M2

**Status:** Completed for correctness (all configurations bit-exact equal to the sequential version).
Performance: the parallel DP is much SLOWER than the sequential DP on this machine (measured, see
sections 6 and 7).

#### 1. Objective

Parallelize energy, DP (two variants), minimum endpoint and seam removal with OpenMP as designed in
`docs/parallelization_strategy.md`, keep the sequential code unchanged, allow thread count and schedule
experiments, and prove equivalence with the sequential version.

#### 2. Work Performed

- `include/seam_omp.h`, `src/seam_omp.cpp` (namespace `omp_impl`): same functions as `seq` plus a
  `ParallelConfig` parameter per stage; `carve()` with `config.threads`; helpers `defaultThreadCount`,
  `scheduleDescription`, `useStaticScheduleUnlessSet`.
- `CMakeLists.txt`: `find_package(OpenMP)`, library `seam_omp` (only this library is compiled with
  `-fopenmp`), test `test_equivalence`, CLI tests `cli_bad_stage`, `cli_zero_threads` (must fail),
  `cli_omp_run`.
- `src/main.cpp`: `--mode omp`, `--threads`, `--stages` (list or `none`), `--persistent-dp`; CSV columns
  `threads`, `schedule` (`:` for chunk), `stages` (`+`, `dp_persistent`).
- `tests/test_equivalence.cpp`: 4 tests (lockstep per stage and per seam; full carve on the 5 real images;
  minimum endpoint with many ties; DP stress).
- Docs: `parallelization_strategy.md` §11 "Implementation Notes (M2)" (directives, default schedule, what
  stays sequential, measured OpenMP overhead); `architecture.md` (final `omp_impl` signatures, `--stages none`,
  static default schedule). `README.md` updated.

#### 3. Technical Explanation

- **Energy / removal:** `#pragma omp parallel for schedule(runtime)` over rows; each thread writes only
  its rows (energy rows; color+gray rows with `memmove`). Implicit barrier at loop end.
- **DP variant A:** sequential loop over rows; for each row a `parallel for` over the columns. The implicit
  barrier at the end of each `parallel for` guarantees row `y` is complete before row `y+1`.
- **DP variant B:** one `#pragma omp parallel` region; all threads run the row loop (row pointers declared
  inside → private); `#pragma omp for` over columns WITHOUT `nowait` → one barrier per row.
- **Minimum endpoint:** each thread keeps a private `(value, column)` best; `omp for ... nowait` (safe: the
  region-end barrier follows); result stored in its own `alignas(64)` slot (no false sharing); sequential
  combine with lexicographic `(value, column)` comparison → same column as the sequential version for any
  thread count and schedule.
- **Backtrack, outer seam loop, DP row 0, combine:** sequential (dependencies / tiny work).
- **Switched-off stages** use the same row/cell helper functions without any directive.
- **Default schedule:** with `OMP_SCHEDULE` unset, libgomp's run-time schedule is `dynamic,1` (observed in
  the CSV output). The CLI therefore sets `static` unless `OMP_SCHEDULE` is set.
- No `critical`, `atomic` or locks anywhere.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| include/seam_omp.h | Created | OpenMP API |
| src/seam_omp.cpp | Created | OpenMP implementation |
| tests/test_equivalence.cpp | Created | seq-vs-omp equivalence tests |
| CMakeLists.txt | Modified | OpenMP library, new tests |
| src/main.cpp | Modified | omp mode, stages, threads, CSV fields |
| docs/parallelization_strategy.md | Modified | §11 implementation notes + measured overhead; old §11 renumbered to §12 |
| docs/architecture.md | Modified | final omp signatures, `--stages none`, default schedule |
| README.md | Modified | M2 status, OpenMP usage |
| WORKLOG.md | Modified | This entry |

`src/seam_seq.cpp` and `include/seam_seq.h` were not changed in M2.

#### 5. Commands Executed

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\test_equivalence.exe
ctest --test-dir build --output-on-failure
# timing: OMP_SCHEDULE=static / unset, threads 1..20, stages energy|removal|dp|dp+persistent|none|all
.\build\seam_carving.exe --input tests\data\boat_sea.png --output out\t_omp.png --width 640 --mode omp --stages <s> --threads <t> --csv
# scratch micro-benchmark (outside repo): g++ -O2 -fopenmp ompcost.cpp; OMP_WAIT_POLICY passive/active
# mutation check: "omp for schedule(runtime)" -> "... nowait" in DP variant B, run test_equivalence, restore
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build (`-Wall -Wextra`) | No warnings | No warnings | Passed |
| `test_equivalence` | 0 failed | 147 368 checks, 0 failed (run twice; about 3–5 min per run) | Passed |
| `ctest` (10 tests) | All pass | 10/10 passed | Passed |
| Mutation: `nowait` on DP variant B | Tests fail | Immediate failures "M differs: random 47x61, dp(B), 2 threads, static, seam 0" and many more | Passed (race detected) |
| CLI omp runs (18 configurations, boat_sea → 640) | Output identical to seq | SHA-256 identical in all 18 runs | Passed |

Lockstep covers: 3 inputs (random 47×61, uniform 30×40, 200×160 crop of boat_sea) × 8 stage variants
(none, energy, DP variant A, DP variant B, min, removal, all stages with DP variant A, all stages with DP variant B) × threads {1,2,4,8,20} × schedules {static, static:1,
dynamic, guided} × 3 repetitions × 20 seams; at every seam energy, `M`, endpoint, seam and image are compared exactly.

**Informal timing** (ONE run per configuration, not a benchmark; i9-13900H, Windows 11, AC power,
"Balanced" power plan, GCC 16.2.0 `-O2`, `OMP_SCHEDULE` unset → static; boat_sea 800×600 → 640×600,
160 seams; seconds):

| Configuration | t_energy | t_dp | t_min | t_removal | t_total |
| ------------- | -------- | ---- | ----- | --------- | ------- |
| seq | 0.086 | 0.056 | 0.00026 | 0.0025 | 0.149 |
| omp, none, 8 thr | 0.088 | 0.058 | 0.00026 | 0.0032 | 0.152 |
| omp, energy, 2 / 4 / 8 / 20 thr | 0.055 / 0.043 / 0.042 / 0.041 | – | – | – | 0.124 / 0.124 / 0.117 / 0.114 |
| omp, removal, 2 / 8 / 20 thr | – | – | – | 0.014 / 0.019 / 0.026 | 0.191 / 0.206 / 0.183 |
| omp, DP variant A, 2 / 8 thr | – | 2.89 / 5.20 | – | – | 2.99 / 5.30 |
| omp, DP variant B, 2 / 8 thr | – | 1.79 / 5.05 | – | – | 1.88 / 5.15 |
| omp, energy+min+removal, 8 thr | 0.042 | – | 0.017 | 0.012 | 0.150 |
| omp, all stages (DP variant A), 8 thr | 0.073 | 10.24 | 0.018 | 0.020 | 10.36 |

Earlier run with `OMP_SCHEDULE` unset (libgomp default `dynamic,1`, before the static default was added),
boat_sea → 760 (40 seams): `t_dp` 0.51 s (1 thread), 1.48 s (4), 3.16 s (20) vs 0.016 s sequential.

OpenMP overhead micro-benchmark: see `docs/parallelization_strategy.md` §11.4 (region 8–261 µs,
barrier 15–213 µs for 1–20 threads; `OMP_WAIT_POLICY=active` no effect).

#### 7. Problems Encountered

- **Parallel DP is 30–90× slower than sequential DP here.** Cause (measured): every OpenMP barrier /
  region costs tens to hundreds of µs with this libgomp build on Windows, while one DP row has only a few µs
  of work. One barrier per row × 599 rows × 160 seams ≈ 96 000 synchronizations. Exact cause inside libgomp
  not verified (hypothesis: OS-level synchronization via winpthreads instead of Linux futexes).
- Parallel removal and minimum are also slower than sequential (work per seam is only a few µs).
- Only the energy stage gains (about 2.1× with 4–20 threads in one run).
- libgomp default schedule `dynamic,1` → fixed by the CLI's static default.
- Because of the synchronization cost, the full-carve test uses only 12 + 6 seams per run (lockstep still
  covers 20 seams per configuration).

#### 8. Decisions and Reasoning

- Static schedule as CLI default when `OMP_SCHEDULE` is unset (libgomp `dynamic,1` default is pathological for DP).
- Kept both DP variants and all stage switches although the DP is slow: correctness first (M2.md), the
  performance study belongs to M4/M5; the slowdown is a real, documented result.
- Per-stage parallel functions take `ParallelConfig` with a default argument, so tests can call them alone.

#### 9. Remaining Tasks

- M3 (correctness validation), M4 (benchmarking: analyse the synchronization overhead),
  M5 (optimization: parallel region management, synchronization overhead) – as defined in M3.md–M5.md.
- Commit/push of M0–M2; Members B/C agreement on the contract not yet recorded.

#### 10. Next Step

Commit and push M0–M2, then hand over to Member B (M3) and Member C (M4/M5).

### [2026-11-18] – M3/M4 planning and reproduction of M0–M2 tests on the M3/M4 machine

**Milestone:** M3, M4 (planning)

**Status:** Planning completed; existing tests reproduced. No M3/M4 code written yet.

#### 1. Objective

The owner of M3 (correctness) and M4 (benchmarking) reviewed M0–M2, planned M3/M4 and checked that the
existing build and tests work on the machine that will be used for M3/M4.

#### 2. Work Performed

- Read `M3.md`, `M4.md`, `docs/benchmark_plan.md`, `CMakeLists.txt`, `include/seam_omp.h`, test function list.
- Built the project (Release) and ran `ctest` on this machine.

#### 3. Technical Explanation

M3/M4 machine (a **different machine** from the one used in M0–M2):

| Item | Value |
| ---- | ----- |
| CPU | Intel Core i7-1065G7 @ 1.30 GHz, 4 cores, 8 logical processors (not hybrid) |
| RAM | 31.8 GB |
| OS | Windows 11 Home Single Language 10.0.22631 |
| Compiler / libraries | GCC 16.2.0 (MSYS2 UCRT64), CMake 4.5.0, Ninja 1.13.2, OpenCV 5.0.0, OpenMP 5.2 (`_OPENMP = 202111`) – identical to the team contract |

Consequences for M4: thread counts 1, 2, 4, 8 (16 > 8 logical processors is not supported; may only be
shown as an oversubscription experiment). All M4 results must come from this one machine; M2's informal
timings (i9-13900H) are not comparable.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| WORKLOG.md | Modified | This entry, Current Development Summary |

No source files changed.

#### 5. Commands Executed

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build (Release, `-Wall -Wextra`) | No errors, no warnings | No errors, no warnings | Passed |
| `ctest` | 10/10 pass | 10/10 passed, total 254.17 s | Passed |
| `test_seq` | 0 failed | 65 checks, 0 failed (0.34 s) | Passed |
| `test_equivalence` | 0 failed | 147 368 checks, 0 failed (252.02 s) | Passed |

Same check counts as on the M2 machine, so the M2 results are reproduced. `test_equivalence` uses up to
20 threads; on this 8-logical-processor CPU that is oversubscription (more threads than hardware threads)
and the results are still bit-exact.

#### 7. Problems Encountered

- None.

#### 8. Decisions and Reasoning

Plan (agreed with the user, details may change):

- **M3:** coverage review of existing tests vs the 10 M3 requirements; new tests for gaps (very narrow
  images, threads > columns, 0 seams, target width 1, synthetic patterns, large images, odd thread counts,
  oversubscription, height carving in omp, many repetitions); sanitizer builds on Linux/WSL (ASan/UBSan,
  TSan/Valgrind where usable, because MinGW has no sanitizers); mutation checks; split quick/full tests;
  `docs/testing_report.md`.
- **M4:** scripts for benchmark images, a runner (warm-up + ≥5 runs, randomized order, output hash check,
  environment metadata) and analysis/plots; pilot run first to estimate total time (parallel DP is very
  slow, see M2); then main grid, stage study, schedule study, DP A vs B; `docs/benchmark_report.md`.

#### 9. Remaining Tasks

- Execute M3 and M4 plans.

#### 10. Next Step

M3 step 1–2: code review of `seam_seq.cpp` / `seam_omp.cpp` and test-coverage gap analysis.

### [2026-11-18] – M3 step 1–2: code review and test-coverage analysis

**Milestone:** M3

**Status:** Completed (steps 1–2 of the M3 plan). M3 as a whole: in progress.

#### 1. Objective

Before writing new tests: read both implementations to look for race conditions and out-of-range
accesses, and compare the existing tests with the 10 requirements of `M3.md` to find gaps.

#### 2. Work Performed

- Reviewed every parallel loop in `src/seam_omp.cpp` and the matching code in `src/seam_seq.cpp`.
- Reviewed `tests/test_seq.cpp` and `tests/test_equivalence.cpp` (all test functions).
- Created `docs/testing_report.md` with environment, review table, coverage table and planned tests.
- Created branch `m3-testing` (team rule: one branch per milestone).

#### 3. Technical Explanation

- **Review method:** for each `#pragma omp` loop: shared vs private data, who writes which memory,
  where the barrier is. Result: every parallel loop writes disjoint memory (one row or one cell per
  iteration, one result slot per thread) and every read of another thread's data happens after a
  barrier. **No race or out-of-range access found by reading.**
- **Overflow bound:** `M <= 510 · rows`; with height reduction the image is transposed, so rows can be the
  original width: 7680 → `M <= 3.9 M`, far below the int32 limit.
- **Main gaps:** no large images (1920×1080, 3840×2160); omp never tested on edge-case images
  (1 column / 1 row / 1×1 / narrower than the thread count); no odd thread counts; seam connectivity not
  checked for every seam; no tool-based race/memory check (no sanitizers in MinGW).

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| docs/testing_report.md | Created | M3 report (sections 1–4; results pending) |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
git switch -c m3-testing
git commit -m "Record M3/M4 plan and test reproduction on M3/M4 machine"   # 55e06af
```

#### 6. Testing and Results

No new tests run in this step (review only). Baseline from the previous entry: `ctest` 10/10.

#### 7. Problems Encountered

- None. Note: `omp_impl::carve()` changes the global thread count (`omp_set_num_threads`); not a bug, but
  tests must set the thread count explicitly.

#### 8. Decisions and Reasoning

- New tests go into a new file `tests/test_m3.cpp`; the existing test files (written by Member A) stay
  unchanged, so their results remain comparable with M2.
- Sanitizers will be run on Linux (WSL), since MinGW GCC supports neither ASan nor TSan.

#### 9. Remaining Tasks

- M3 steps 3–6: implement `tests/test_m3.cpp`, sanitizer runs, mutation checks, fast/full test split,
  finish `docs/testing_report.md`.

#### 10. Next Step

Implement `tests/test_m3.cpp` (edge cases for omp, forced seam positions, maximum contrast, brute force on
non-square shapes, connectivity of every seam, large images, odd thread counts).

### [2026-11-19] – M3 steps 3–6: new tests, mutation check, sanitizers, testing report

**Milestone:** M3

**Status:** Completed. No defect found in the implementation; no source file in `src/` or `include/` changed.

#### 1. Objective

Close the coverage gaps found in steps 1–2, prove that the tests can find real bugs, check for memory errors
and data races with tools, and write `docs/testing_report.md`.

#### 2. Work Performed

- `tests/test_m3.cpp`: independent reference implementation + 9 test groups (edge-case images, forced border
  seams, patterns / maximum energy 510, brute force on non-square shapes, 100-seam connectivity run, large
  images 1920×1080 / 3840×2160, odd thread counts / oversubscription / threads > columns or rows, 50× repeated
  runs, real images −20 % width −10 % height). Modes `--quick` and full.
- `CMakeLists.txt`: target `test_m3`, CTest entries `test_m3_quick`, `test_m3_full`, labels `quick` / `full`.
- `scripts/mutation_check.py`: 8 mutants built in `build/mutants/` (real sources untouched).
- `scripts/run_sanitizers.sh` (WSL): ASan+UBSan (g++), TSan+Archer (clang/libomp), memcheck mode (not run).
- `tests/tsan.supp`: TSan suppressions for uninstrumented libomp / libtbb internals.
- `docs/testing_report.md` completed; `README.md` test section and status table updated.
- Environment for sanitizers: WSL Ubuntu 26.04 with g++ 15.2.0, clang 21.1.8, OpenCV 4.10.0, valgrind 3.26.0.

#### 3. Technical Explanation

- **Why an independent reference:** all earlier equivalence tests compare omp with seq; a mistake present in
  both cannot be seen that way. The reference uses `at<>()`, 64-bit sums, `(value, column)` pair comparison
  and copy-based removal – a different way to write the same contract.
- **Lockstep:** seq, omp and reference run seam by seam; after every stage everything is compared exactly and
  each seam is checked for connectivity. Integer data → no tolerance.
- **Mutation testing idea:** if a test suite passes on correct code and fails on every inserted bug, it is
  able to detect that kind of bug.
- **ThreadSanitizer + Archer:** TSan records which thread touched which memory and reports two accesses to the
  same address (one a write) that are not ordered by synchronization. Archer tells TSan where OpenMP
  barriers are. Libraries not compiled with TSan (libomp internals, libtbb used by OpenCV) produce false
  reports, which are suppressed with `called_from_lib:` rules only.
- **Positive control lesson:** a first suppression file with `race:libomp.so` silenced everything, including
  the real race of mutant M1 – because a `race:` rule matches any stack frame and every OpenMP worker is
  started by libomp. Found only because the mutant was tested with the same setup; rule removed.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| tests/test_m3.cpp | Created | M3 tests + reference implementation |
| tests/tsan.supp | Created | TSan suppressions (libomp/libtbb internals only) |
| CMakeLists.txt | Modified | `test_m3`, CTest labels quick/full |
| scripts/mutation_check.py | Created | Mutation check |
| scripts/run_sanitizers.sh | Created | ASan / TSan / memcheck runs in WSL |
| docs/testing_report.md | Modified | Sections 4–6 (tests, results, conclusions), environment |
| README.md | Modified | Test section, milestone table |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
cmake --build build
.\build\test_m3.exe --quick ; .\build\test_m3.exe
ctest --test-dir build -L quick ; ctest --test-dir build -N -L full
python scripts\mutation_check.py
# Linux tools needed by the sanitizer runs: see docs/testing_report.md, section 1
wsl -u root -- bash scripts/run_sanitizers.sh asan . /root/ttss-sanitizers
wsl -u root -- bash scripts/run_sanitizers.sh tsan . /root/ttss-sanitizers
wsl -u root -- bash scripts/run_sanitizers.sh tsan-quick build/mutants/M1 /root/ttss-sanitizers
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Build with `test_m3` (`-Wall -Wextra`) | No warnings | No warnings | Passed |
| `test_m3 --quick` (Windows) | 0 failed | 6 801 checks, 0 failed, 15 s | Passed |
| `test_m3` full (Windows) | 0 failed | 71 580 checks, 0 failed, 245 s (machine busy) | Passed |
| `ctest -L quick` | 10/10 | 10/10, 31 s | Passed |
| Mutation check (8 mutants) | all detected | 8/8 detected by `test_m3 --quick`; M8 (same bug in seq and omp) NOT detected by `test_equivalence` (0/147 368) | Passed |
| ASan+UBSan: test_seq / test_m3 / test_equivalence | no report, 0 failed | 65 / 71 580 / 147 368 checks, 0 failed, no sanitizer report | Passed |
| TSan, no suppressions, test_m3 --quick | – | 4 reports, all inside libomp / libtbb (false positives), 0 failed | Analysed |
| TSan positive control, mutant M1 | race reported in seam_omp.cpp | 83 reports, all in `seam_omp.cpp` (`dpCell`, line 39) | Passed |
| TSan (final suppressions), test_m3 --quick / full | 0 reports | 0 / 0 reports; 6 801 / 71 580 checks, 0 failed | Passed |

#### 7. Problems Encountered

- TSan false positives from uninstrumented libraries; first suppression attempt too broad (see section 3).
- Unsuppressed TSan `test_equivalence` was still running after > 40 min → stopped; not repeated (test_m3
  covers the same configurations). ASan `test_equivalence` took 2 650 s because it ran concurrently with TSan.
- Editing `run_sanitizers.sh` while the ASan run used it gave a harmless bash "unexpected EOF" after the last
  test had finished (all results were already written).
- Valgrind memcheck mode exists in the script but was not run (ASan already covers memory errors).

#### 8. Decisions and Reasoning

- Do not modify `test_seq.cpp` / `test_equivalence.cpp` (Member A's), so their M2 results stay comparable.
- TSan suppressions limited to `called_from_lib:` and validated by a positive control.
- No change to `src/` because no defect was found (M3.md: do not change the algorithm to make tests pass).

#### 9. Remaining Tasks

- M4 (benchmarking).

#### 10. Next Step

M4 pilot run on an idle machine.

### [2026-11-19] – M4 preparation: benchmark scripts, images, pilot runs

**Milestone:** M4

**Status:** In progress. Scripts done and tested; pilot done; main benchmark not yet run.

#### 1. Objective

Build a reproducible benchmark system (M4.md) and estimate how long the real experiments take.

#### 2. Work Performed

- `scripts/make_bench_images.py`: benchmark images in `bench_images/` (ignored by git) + `manifest.json`.
- `scripts/run_bench.py`: experiments `pilot`, `pilot_threads`, `main`, `main_nodp`, `stages`, `schedule`,
  `content`; raw CSV rows in `results/`, metadata in `results/<exp>.meta.json`.
- `scripts/analyze_bench.py`: medians, min/max, IQR, speedup, efficiency, Markdown tables, plots
  (`docs/figures/`); tested on a small real run written to a scratch folder (not kept).
- `scripts/run_all_bench.py`: the overnight plan chosen by the user.
- Pilot runs: `results/pilot.csv` (12 runs), `results/pilot_threads.csv` (6 runs).

#### 3. Technical Explanation

- **Images:** the full-resolution Unsplash photos are not on this machine and Unsplash blocks automated
  downloads (HTTP 401 / bot check), so the 800-px PNGs in `tests/data/` are the sources: main series from
  `person_desert` (512² scale 0.96; 1920×1080 ×2.4; 3840×2160 ×4.8; 7680×4320 ×9.6 upscaled), content
  series 1920×1080 from the other photos + seeded noise. Run time per seam is O(W·H) with no data-dependent
  loops, so content should matter little; the `content` experiment checks this.
- **Runner:** round 0 = warm-up (excluded), then measured rounds; configurations shuffled inside each round
  (seed 2026); every output image hashed and compared with the seq output; resumable.
- **Pilot (1 run each, machine idle, power plan "HP Recommended", AC; planning data only):**

| Size, −10 % | seq t_total | omp 4 thr, all stages | omp 4 thr, DP sequential |
| ----------- | ----------- | --------------------- | ------------------------ |
| 512×512 | 0.036 s | 1.44 s | 0.041 s |
| 1920×1080 | 1.02 s | 10.95 s | 0.86 s |
| 3840×2160 | 9.15 s | 45.1 s | 5.91 s |
| 7680×4320 | 71.2 s | 193.0 s | 41.8 s |

  All 12 outputs had the same hash as seq. Parallel DP synchronization cost per image row (from `t_dp`,
  1920×1080): 1 thread ≈ 12 µs, 2 ≈ 40 µs, 4 ≈ 51 µs, 8 ≈ 82 µs, 16 ≈ 118 µs, variant B with 16 ≈ 190 µs
  (sequential DP ≈ 2 µs of work per row).
- **Time budget (estimate from the pilot):** all experiments with 1 warm-up + 5 rounds ≈ 22 h. User chose
  the overnight variant (≈ 8–9 h): `main` (all stages) with 3 rounds, no warm-up, 7680×4320 only at −10 %;
  `schedule` with 3 rounds; the rest 1 + 5 rounds.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| scripts/make_bench_images.py | Created | Benchmark images |
| scripts/run_bench.py | Created | Benchmark runner |
| scripts/analyze_bench.py | Created | Analysis and plots |
| scripts/run_all_bench.py | Created | Overnight plan |
| results/pilot*.csv, *.meta.json | Created | Pilot data (planning only) |
| .gitignore | Modified | `bench_images/` |
| WORKLOG.md | Modified | This entry |

#### 5. Commands Executed

```powershell
python scripts\make_bench_images.py
python scripts\run_bench.py pilot --rounds 1 --warmup 0
python scripts\run_bench.py pilot_threads --rounds 1 --warmup 0
python scripts\analyze_bench.py <scratch csv files> --out <scratch folder>   # script test
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Runner smoke test (512², 3 configs) | CSV rows, hash_ok 1 | 3 rows, hash_ok 1 | Passed |
| Pilot outputs vs seq | identical hash | 12/12 identical | Passed |
| Analysis script on small real data | tables + plots | tables + 8 plots; fixed a crash in the schedule plot (empty panel) | Passed after fix |

#### 7. Problems Encountered

- Unsplash blocks automated downloads → upscaled 800-px sources (documented in the manifest and report).
- Parallel DP is 3–40× slower than sequential DP (pilot) → full plan would take ~22 h.

#### 8. Decisions and Reasoning

- Primary speedup series = `main_nodp` (energy, min, removal parallel; DP sequential) in addition to the
  required "combined" version with all stages (`main`), because the parallel DP alone makes the combined
  version slower than sequential; both are reported.
- Pilot data are never reported as results; `analyze_bench.py` ignores `results/pilot*.csv` by default.

#### 9. Remaining Tasks

- Run `scripts/run_all_bench.py` (user prepares the machine first), analyse, write `docs/benchmark_report.md`.

#### 10. Next Step

Main benchmark run.

### [2026-11-19] – M4: benchmark run, Linux comparison, analysis and report

**Milestone:** M4

**Status:** Completed. All experiments of the overnight plan ran without a single failed run or wrong output;
two supplementary measurements were added; `docs/benchmark_report.md` is final.

#### 1. Objective

Run the M4 benchmark, verify the data, analyse speedup, efficiency, stages, DP variants, schedules, image
size, content and initialization cost, and write the benchmark report (M4.md).

#### 2. Work Performed

- **Overnight run** `scripts/run_all_bench.py`, 01:19–08:43 (7 h 24 min, machine idle, AC, Best performance).
  Experiments: `content` 2.2 min, `stages` 58.7 min, `main_nodp` 200.0 min, `main` 162.6 min, `schedule`
  20.7 min. In total 1053 runs.
  - After every experiment the CSV was checked with Python's `csv` module (row count, exit code, `hash_ok`).
- **`scripts/run_bench.py`** (commit `dc4bde2`):
  - Records the *effective* compile flags from `build.ninja`.
  - Records the compiler named in the CMake cache.
  - Writes `unknown` instead of "clean" when git fails.
  - New option `--exe`, Linux metadata, and the supplementary experiment `wsl`.
- **Linux build in WSL2** (`/root/ttss-release`, same commit), quick tests, then the `wsl` experiment
  (138 runs, 22 min).
- **`scripts/membw.cpp`**: memory-bandwidth reference (read / write / copy, 1–8 threads), run twice
  → `results/membw.txt`.
- **`scripts/analyze_bench.py`**:
  - Plot layout: legends outside the plot area, plain numbers on log axes, "(over)" tick for 16 threads,
    linear speedup axis when the speedups are small.
  - New tables: stage speedups (only-this-stage and by image size), Amdahl's law, DP time per row,
    Windows vs Linux.
  - Fixed part of a run: pixel-work model, median-based fits, direct measurement.
  - New plot `dp_time_per_row.png`.
  - `--out` no longer changes the input folder.
- **`docs/benchmark_report.md`** written; **`README.md`**: M4 Done, benchmark section, limitation text,
  layout.
- **WORKLOG M3 entry**: removed two machine-installation command lines (WSL move / apt-get), as the user
  asked on 2026-11-18. They are replaced by a pointer to `docs/testing_report.md` §1.

#### 3. Technical Explanation

Key results (all measured; tables in `results/summary.md`, explanation in the report):

| Question | Result |
| -------- | ------ |
| All four stages parallel (DP variant A) | Slower than sequential everywhere: speedup 0.01–0.68 (512×512 … 7680×4320, 1–16 threads) |
| DP sequential (energy+min+removal parallel) | 1.40–1.79× for ≥ 1920×1080 with 4–16 threads; best 1.79× at 7680×4320, −10 %, 8 threads; 512×512: 0.81–1.05× |
| Stages (1920×1080, 4 threads) | Energy 3.31×; removal 1.11× (2.8–3.1× at 7680×4320); min 0.03× (slower); parallel DP 0.04× |
| DP per row on Windows | 1.84 µs of work; +9.2 µs (1 thread, variant A) and +22…+167 µs (2–16 threads) of synchronization |
| Same code under Linux (WSL2) | −0.4…+0.1 µs per row (2–4 threads); variant B 1.27× faster than the sequential DP; all stages 1.36× (1920×1080) to 1.71× (7680×4320) with 4 threads |
| Amdahl (energy+min+removal) | f = 0.596 → limit 2.48×; measured 1.52× at 4 threads (84 % of the limit) |
| Schedules | DP sequential: all within 5 %. Parallel DP: `dynamic` 1.4–2.1× slower, `static,1` 10–14 % slower (false sharing) |
| Fixed part of a run | 1.4–58 ms (≤ 0.5 % of the run for ≥ 1920×1080), measured as `t_total − Σ stages` |
| Content | 6 images within 4.4 % (sequential) |
| Memory bandwidth | Read 14.5 GB/s (1 thread) → 41–46 GB/s (4–8 threads); write ≈ 17 GB/s; copy ≈ 24 GB/s |

**Why the parallel DP is slow** (report §7.1).
- Each image row needs one synchronization (variant A: fork + join; variant B: one barrier).
- At 1920×1080, −20 % that is 414 336 synchronizations per run. Each costs 22–167 µs on Windows, against
  1.84 µs of work per row.
- Under Linux the same code needs about 1 µs per row. So the cost comes from the Windows OpenMP runtime, not
  from the algorithm.
- *Hypothesis:* the MinGW build of libgomp waits on OS mutexes and semaphores, while the Linux build spins
  and uses futexes. Not verified with a profiler.

**Fixed part** (report §6.8, §7.8).
- The cost of one seam shrinks with the width: seam k processes H·(W − k) pixels. A straight line
  `t = a + b·seams` through the three reductions therefore has a large positive intercept: 17–21 % of the run,
  e.g. 14.1 s at 7680×4320.
- The direct measurement `t_total − Σ stages` shows only 1.4–58 ms. The report uses the direct value and keeps
  the regressions as a lesson.

#### 4. Files Created or Modified

| File | Action | Description |
| ---- | ------ | ----------- |
| results/content.csv, stages.csv, main_nodp.csv, main.csv, schedule.csv (+ .meta.json) | Created | Raw benchmark data (Windows) |
| results/wsl.csv, wsl.meta.json, wsl.stdout.txt | Created | Supplementary Linux run |
| results/membw.txt | Created | Memory-bandwidth reference |
| results/run_all.log, run_all.stdout.txt | Created | Log of the overnight run |
| results/summary.md, summary.csv | Created | Generated tables |
| docs/figures/*.png (11 plots) | Created | Generated plots |
| scripts/run_bench.py | Modified | Effective flags, compiler from cache, git "unknown", `--exe`, Linux metadata, `wsl` experiment (commit `dc4bde2`) |
| scripts/analyze_bench.py | Modified | Layout fixes, new tables and plot, fixed-part analysis |
| scripts/membw.cpp | Created | Bandwidth reference program |
| docs/benchmark_report.md | Created | M4 report (final) |
| README.md | Modified | M4 status, benchmark section, limitation, layout |
| WORKLOG.md | Modified | This entry; status sections; M3 entry without install lines |

#### 5. Commands Executed

```powershell
python scripts\run_all_bench.py                      # started detached; log results\run_all.log
python <scratch>\check_csv.py results\<exp>.csv     # row / exit / hash check after every experiment
ninja -C build -n                                    # "no work to do": benchmark binary matches the sources
python scripts\run_bench.py main_nodp --only "<512x512 seq + omp t4>" --rounds 1 --warmup 0 --out <scratch>   # smoke test
g++ -std=c++17 -O2 -fopenmp -Wall -Wextra scripts\membw.cpp -o build\membw.exe ; build\membw.exe   # twice
python scripts\analyze_bench.py
```

```bash
# WSL2 (Ubuntu), repository folder /mnt/e/hus/ttss/TTSS
cmake -S . -B /root/ttss-release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build /root/ttss-release
/root/ttss-release/test_seq ; /root/ttss-release/test_m3 --quick
# GIT_CONFIG_COUNT=2 safe.directory=/mnt/e/hus/ttss/TTSS core.autocrlf=true (environment only)
python3 scripts/run_bench.py wsl --exe /root/ttss-release/seam_carving --warmup 1 --rounds 5
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Overnight run, 5 experiments | Every run exit 0 and output identical to seq | 1053/1053 exit 0, `hash_ok` 1; `git_dirty: false` in all 5 metadata files | Passed |
| Benchmark binary vs sources | Up to date | `ninja -n`: "no work to do" | Passed |
| Effective compile flags | `-O2` | `build.ninja`: `-O2 -DNDEBUG -std=c++17 -Wall -Wextra` (+ `-fopenmp`) | Passed (metadata field showed the CMake cache default `-O3` → script fixed) |
| `run_bench.py` after the changes | Rows and metadata correct | Windows smoke test 2/2 OK (flags recorded); Linux smoke test 3/3 OK | Passed |
| Linux Release build | 0 failed | `test_seq` 65 checks, 0 failed; `test_m3 --quick` 6 801 checks, 0 failed | Passed |
| `wsl` experiment | Every run OK | 138/138 exit 0, `hash_ok` 1, commit `dc4bde2`, `git_dirty: false` | Passed |
| `membw` | Two consistent runs | Differences ≤ 2 % except 4-thread read (44.7 / 40.9 GB/s) | Passed |
| `analyze_bench.py` | Tables + plots, no error | Exit 0, 11 plots, layout checked by viewing every plot | Passed |

#### 7. Problems Encountered

- **A quick `awk` check reported 360 "failed" runs.** False alarm: the `config` field contains commas inside
  quotes, so `awk -F,` split it into the wrong columns. The check was repeated with Python's `csv` module:
  0 failures.
- **Metadata field `CMAKE_CXX_FLAGS_RELEASE = -O3`.** It is the CMake cache default and is not used (the build
  used `-O2`). The raw files were not edited; the problem is explained in report §2 and fixed for future runs.
- **Git inside WSL** refuses the repository ("dubious ownership"), and with CRLF files it would report a
  dirty tree. Solved with temporary `GIT_CONFIG_*` environment variables; no configuration file was changed.
  `run_bench.py` no longer reports "clean" when git fails.
- **Fixed-part regression.** It was biased by the shrinking width and by one slow run (7680×4320 seq,
  90.1 s vs 70.4–71.2 s). It is now fitted on medians and complemented by the direct measurement.
- **Plot layout.** Legends covered data, the "oversubscribed" label covered points, log labels overlapped, and
  the ideal line squashed the speedup curves. All fixed and checked visually.
- **First report draft.** Some numbers were wrong (min region cost per seam, idle-gap classification,
  thread-creation bound, mixed thread ranges in the Windows/Linux comparison). They were corrected against the
  data before the commit.
- `analyze_bench.py` and `scripts/membw.cpp` were edited while the `wsl` experiment was running. Neither takes
  part in the measurement (no build, no CPU load).
- The WSL timestamps are UTC (Windows: UTC+7).

#### 8. Decisions and Reasoning

- **Supplementary `wsl` experiment.** Without it the main finding (parallel DP 1.8–92× slower) could only be
  explained by a hypothesis. The same code on the same CPU with another OpenMP runtime separates "algorithm"
  from "platform". It is reported separately and labelled as supplementary.
- **`membw` reference.** The memory-bandwidth analysis needs the machine's real bandwidth, not only the
  theoretical 51.2 GB/s.
- **Speedups inside each experiment.** `T_seq` is taken from the same session and platform.
- **Direct measurement of the fixed part.** An intercept is an extrapolation, so the direct value is used.

#### 9. Remaining Tasks

- Pull requests for `m3-testing` and `m4-benchmark` and a team review (nothing pushed yet).
- Not tested: `OMP_PLACES` / `OMP_PROC_BIND`; DP variant B at sizes other than 1920×1080; CPU frequency
  logging; hardware counters.
- M5 suggestions (report §7.10): keep DP and min sequential on Windows, use a size threshold for parallel
  regions, one region per seam, a DP scheme with fewer barriers, or another runtime.

#### 10. Next Step

Open the pull requests (M3, M4), then M5 (optimization) based on report §7.

### [2026-11-19] – Publish M3 + M4 for review (one pull request)

**Milestone:** M3, M4

**Status:** Completed (branch pushed; pull request opened by the user on GitHub, review pending).

#### 1. Objective

Make the M3 and M4 work available to the team for review and merge into `main`.

#### 2. Work Performed

- Checked that `origin/main` (3eae021) has no commits missing from `m4-benchmark`, so the merge has no
  conflicts.
- Pushed `m4-benchmark`. It was created from `m3-testing`, so it contains both milestones (9 commits).
  Decision (user): **one** pull request `m4-benchmark → main` for M3 and M4 together.

#### 3. Technical Explanation

No code change. The pull request lets the other members read the diff, rerun `ctest -L quick`, and
comment before the work enters `main`.

#### 4. Files Created or Modified

| File       | Action   | Description       |
| ---------- | -------- | ----------------- |
| WORKLOG.md | Modified | Added this entry  |

#### 5. Commands Executed

```bash
git fetch origin
git log --oneline main..m4-benchmark      # 9 commits (M3: 4, M4: 5)
git push -u origin m4-benchmark
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
| Push `m4-benchmark` | Branch on GitHub | `origin/m4-benchmark` created, local branch tracks it | Passed |

#### 7. Problems Encountered

- The GitHub CLI (`gh`) is not available on this machine, so the pull request is opened in the browser
  from https://github.com/MyleCutie45/TTSS/pull/new/m4-benchmark.

#### 8. Decisions and Reasoning

- One pull request instead of two: `m4-benchmark` already contains all M3 commits; a single review avoids
  reviewing M3 twice.
- The branch `m3-testing` is kept (it is fully contained in `m4-benchmark`) and not pushed.

#### 9. Remaining Tasks

- Team review and merge of the pull request.

#### 10. Next Step

After the merge: M5 (optimization) based on `docs/benchmark_report.md` §7.10.

---

# Technical Decision History

Record important architecture and algorithmic decisions here.

| Date | Decision | Reason | Alternatives |
| ---- | -------- | ------ | ------------ |
| 2026-11-16 | Toolchain: MSYS2 UCRT64 (GCC 16.2.0, CMake 4.5.0, Ninja 1.13.2, OpenCV 5.0.0, OpenMP 5.2) for all members | Modern C++17/OpenMP, one-command install, identical versions | Visual Studio + vcpkg (OpenMP 2.0 only, long OpenCV build); old MinGW GCC 6.3 (32-bit, incomplete C++17) |
| 2026-11-17 | Integer central-difference energy `|I(x+1)-I(x-1)| + |I(y+1)-I(y-1)|` on gray image, borders clamped to current width | Exact, deterministic, follows paper's `e1` | Sobel, float gradients (rounding differences) |
| 2026-11-17 | Gray image computed once, seams removed from color and gray | Less work, no repeated rounding | Re-convert gray after each seam |
| 2026-11-17 | Energy and `M` as int32 (`CV_32SC1`) | Bit-exact seq/omp comparison; no overflow (max ≈ 2.2 M) | float / double |
| 2026-11-17 | Tie rule "smallest column wins" (endpoint and backtrack) | Unique result independent of threads | First found by any thread (non-deterministic) |
| 2026-11-17 | Matrices allocated once; `width` variable tracks current width | No allocation inside the seam loop | Reallocate/crop every seam |
| 2026-11-17 | Same API in `seq` and `omp_impl`; all stages public | Stage-by-stage equivalence tests | Only `carve()` public |
| 2026-11-17 | `ParallelConfig` switches stages; `schedule(runtime)` + `OMP_SCHEDULE` | 5 required versions without duplicated code; schedule experiments without rebuild | Separate binaries per version |
| 2026-11-17 | DP: two variants (DP variant A: parallel for per row; DP variant B: persistent region) | M2.md asks to investigate region overhead | Only DP variant A |
| 2026-11-17 | Min endpoint: per-thread `(value, column)` + sequential combine | Deterministic tie-breaking | `reduction(min)` (no column); `declare reduction` |
| 2026-11-17 | Backtracking and outer seam loop stay sequential | True data dependencies | – |
| 2026-11-17 | Height reduction by transpose, after width | Reuse vertical-seam code | Separate horizontal implementation; paper's optimal order |
| 2026-11-17 | CLI + fixed 15-column CSV line (`:` / `+` inside fields) | Stable interface for M3/M4 scripts | Free-form output |
| 2026-11-17 | CLI uses `static` schedule when `OMP_SCHEDULE` is unset | libgomp default `dynamic,1` made DP extremely slow (measured) | Keep libgomp default; hard-code schedule |
| 2026-11-17 | `-O2` set explicitly for Release | Contract; CMake's GCC default is `-O3` | `-O3` |
| 2026-11-19 | Benchmark metadata takes the compile flags from `build.ninja` | `CMakeCache.txt` keeps CMake's default `-O3`, which `CMakeLists.txt` overrides with `-O2` | Read the cache (wrong value) |
| 2026-11-19 | Speedup always against `T_seq` of the same experiment (session, platform) | Equal conditions for both sides of the ratio | One global `T_seq` per size |
| 2026-11-19 | Fixed part of a run measured directly (`t_total − Σ stages`); regressions only as illustration | The per-seam cost shrinks with the width; intercepts are extrapolations | `t = a + b·seams` (biased: 17–21 % "fixed part") |
| 2026-11-19 | Supplementary Linux run in WSL2 (`wsl`) and memory-bandwidth reference (`membw`) | Separate algorithm from platform for the DP overhead; real bandwidth for the memory analysis | Profiling libgomp on Windows (no tools set up); theoretical bandwidth only |

# Known Issues

| Issue | Description | Priority | Status |
| ----- | ----------- | -------- | ------ |
| No ThreadSanitizer | TSan is not supported by MinGW on Windows; races must be detected by repeated seq-vs-omp comparison tests | Medium | Mitigated in M3: TSan + Archer and ASan run on Linux/WSL (`scripts/run_sanitizers.sh`); Windows build itself still has no sanitizer |
| TSan false positives | libomp / libtbb (OpenCV) are not built with TSan → false reports; `tests/tsan.supp` uses `called_from_lib:` only (a `race:libomp.so` rule hides real races) | Info | Handled (validated with mutant M1) |
| `python` resolves to MSYS2 Python | MSYS2 is first in PATH, so `python` has no Pillow/numpy/matplotlib. Call another Python (e.g. conda) by its full path | Low | Open (workaround known) |
| Hybrid CPU | i9-13900H has P- and E-cores; speedup beyond 6 threads will not be linear; affects M4 analysis | Info | Not applicable to the M4 results: the benchmark laptop (i7-1065G7) has 4 equal cores + Hyper-Threading; hypothesis not testable (benchmark report §8) |
| 8K benchmark image | Largest source photo is 6000x4000; 7680x4320 needs upscaling | Low | Closed: all large benchmark images are upscaled from the 800-px sources; documented in benchmark report §3; the content experiment shows no timing effect |
| Expensive OpenMP synchronization | libgomp on Windows: region 8–261 µs, barrier 15–213 µs (1–20 threads, measured). Parallel DP (one barrier per row) is 30–90× slower than sequential DP. M4: +22…+167 µs per DP row (2–16 threads, i7-1065G7); the same code under Linux (WSL2) needs −0.4…+0.1 µs (2–4 threads) → caused by the Windows OpenMP runtime | High | Analysed in M4 (benchmark report §7.1); open for M5 |
| libgomp default schedule | `schedule(runtime)` without `OMP_SCHEDULE` = `dynamic,1` | Medium | Fixed in CLI (static default) |
| Benchmark metadata flags | `results/*.meta.json` of the 2026-11-19 Windows run show `CMAKE_CXX_FLAGS_RELEASE = -O3` (CMake cache default, not used; the build used `-O2`) | Low | Fixed in `run_bench.py` (`dc4bde2`); explained in benchmark report §2 |
| CPU frequency not recorded | Laptop turbo / power limits may change the clock between configurations; some effects in the report are hypotheses for this reason | Info | Open (limitation) |

# Benchmark History

Record actual benchmark experiments.

| Date | Image Size | Version | Threads | Execution Time | Notes |
| ---- | ---------- | ------- | ------- | -------------- | ----- |
| 2026-11-19 | 1920×1080 → −20 % | seq | – | 1.934 s | Median of 5 (`main_nodp`); Windows, i7-1065G7, GCC 16.2 `-O2` |
| 2026-11-19 | 1920×1080 → −20 % | omp energy+min+removal | 4 | 1.347 s (speedup 1.44) | `main_nodp`, 5 runs |
| 2026-11-19 | 1920×1080 → −20 % | omp all stages (DP variant A) | 4 | 20.317 s (speedup 0.10) | `main`, 3 runs |
| 2026-11-19 | 512×512 → −20 % | omp energy+min+removal | 4 | 0.062 s (speedup 1.03) | `main_nodp`; seq 0.064 s |
| 2026-11-19 | 7680×4320 → −10 % | seq | – | 70.737 s | `main_nodp`, 5 runs (one outlier 90.1 s) |
| 2026-11-19 | 7680×4320 → −10 % | omp energy+min+removal | 8 | 39.602 s (speedup 1.79, best) | `main_nodp` |
| 2026-11-19 | 7680×4320 → −10 % | omp all stages (DP variant A) | 4 | 184.084 s (speedup 0.39) | `main`, 3 runs |
| 2026-11-19 | 1920×1080 → −20 % | Linux/WSL2 seq | – | 2.101 s | `wsl`, 5 runs; GCC 15.2, Linux libgomp |
| 2026-11-19 | 1920×1080 → −20 % | Linux/WSL2 omp all stages (DP variant A) | 4 | 1.548 s (speedup 1.36) | `wsl` |
| 2026-11-19 | 7680×4320 → −10 % | Linux/WSL2 omp all stages (DP variant A) | 4 | 37.975 s (speedup 1.71) | `wsl`; seq 64.793 s |

Execution time = median `t_total` (algorithm only, no file I/O). Full tables: `results/summary.md`; analysis:
`docs/benchmark_report.md`.

**Important:** Only record experimentally measured values. Never insert estimated or fabricated results.

# Learning Notes

Record important concepts learned during development.

Examples:

- Dynamic Programming dependencies.
- OpenMP parallel for.
- OpenMP reduction.
- Synchronization and barriers.
- Race conditions.
- Cache locality.
- Speedup and parallel efficiency.
- Amdahl's Law.

Notes so far (2026-11-17):

- **DP dependency:** `M(x,y)` needs `M(x-1..x+1, y-1)`. Rows must run in order; cells of one row are
  independent. So the DP is parallel only *inside* a row, with a barrier between rows.
- **Barrier:** a point where every thread waits for all others. `omp for` has an implicit barrier at its
  end; `nowait` removes it. Removing the DP barrier gave wrong `M` immediately in the tests (race condition).
- **Deterministic reduction:** comparing `(value, column)` pairs defines a total order, so the parallel
  minimum gives the same column as the sequential loop for any thread count or schedule.
- **False sharing:** per-thread result slots are `alignas(64)` so two threads never write the same cache line.
- **Overhead vs work:** a parallel region or barrier costs 8–260 µs here (measured); a DP row has only a few
  µs of work. Parallelism pays off only when the work between synchronizations is much larger than the
  synchronization cost (energy stage: one region per seam → faster; DP: one barrier per row → slower).
- **`schedule(runtime)`:** the schedule is taken from `OMP_SCHEDULE`; libgomp's default when it is unset is
  `dynamic,1` (one iteration at a time, high overhead for small loops).
- **Testing without ThreadSanitizer:** races are detected by comparing every intermediate result with the
  sequential version, many times, with many thread counts and schedules.

Notes from M4 (2026-11-19, measured on the i7-1065G7):

- **Amdahl's law in practice:** with the DP sequential, 60 % of the run is parallel → at most 2.5× even with
  infinitely many threads; measured 1.5× with 4 threads (84 % of the Amdahl limit for 4 threads).
- **Synchronization cost depends on the runtime / OS:** the same barrier costs about 45 µs on Windows
  (libgomp/MinGW) and about 1 µs under Linux on the same CPU. Rule: parallelize only when the work between two
  synchronizations is much larger than one synchronization (break-even DP width ≈ 56 000 px on Windows,
  ≈ 1 200–1 800 px under Linux, with 4 threads).
- **Region start-up cost:** a parallel region that follows a long sequential phase costs about 0.2 ms more
  (threads must be woken up) → parallel min and small-image removal are slower than sequential.
- **Schedules:** `dynamic,1` costs about 25 ns per iteration (shared counter); `static,1` makes neighbouring
  columns belong to different threads → false sharing on the cache lines of `M` (10–14 % slower).
- **Compute-bound vs memory-bound test:** if the time per element stays the same when the data no longer fit
  in the cache (3 MB → 398 MB), the loop is compute-bound (energy, DP); if it grows, it is memory-bound
  (removal: 6.5× slower per pixel from RAM).
- **Hyper-Threading:** 8 threads on 4 cores gave only +2–3 % here; the useful parallelism is 4.
- **Models:** the intercept of a fitted line is an extrapolation; with the wrong model (constant cost per
  seam) it suggested a 20 % "fixed part" that the direct measurement (≤ 0.5 %) does not show.

---

# Current Development Summary

This section must always reflect the latest project state.

**Completed:**

- Toolchain setup and smoke test (MSYS2 UCRT64, GCC 16.2.0, OpenCV 5.0.0, OpenMP 5.2).
- Test images in `tests/data/` and `images/SOURCES.md`.
- M0 documents: `docs/algorithm.md`, `docs/parallelization_strategy.md`, `docs/architecture.md`, `docs/benchmark_plan.md`.
- M1: sequential implementation, CLI, README, unit tests (65 checks pass).
- M2: OpenMP implementation (all stages, DP variants A/B), equivalence tests (147 368 checks pass).

- M0–M2 tests reproduced on the M3/M4 machine (i7-1065G7): build OK, `ctest` 10/10 (2026-11-18).
- M3: `tests/test_m3.cpp` (71 580 checks), mutation check 8/8, ASan + TSan clean, `docs/testing_report.md` (2026-11-19).
- M4: overnight benchmark (1053 runs) + Linux/WSL2 comparison (138 runs) + memory-bandwidth reference, all runs correct; `results/`, `docs/figures/`, `docs/benchmark_report.md` (2026-11-19). DP-sequential version 1.4–1.8× faster (≥ 1920×1080); all-stages version slower on Windows (Windows OpenMP synchronization cost), 1.36–1.71× under Linux.

**In Progress:**

- Review of the pull request `m4-benchmark → main` (contains M3 and M4), opened 2026-11-19.

**Blocked:**

- None.

**Next Recommended Task:**

- Team review and merge of the M3 + M4 pull request; then M5 (optimization) using `docs/benchmark_report.md` §7.10. (Agreement of the other members on `docs/architecture.md` still not recorded.)

### [2026-11-19] – Sync local repository with GitHub (after PR #1)

**Goal:** Bring the local clone up to date with contributors' work on GitHub without losing uncommitted local work.

**What was found:**

- `origin` = `https://github.com/MyleCutie45/TTSS.git` (correctly linked).
- Local `main` was 12 commits behind `origin/main` (PR #1 `m4-benchmark`: M0–M4).
- Local had uncommitted work: modified `WORKLOG.md` (entries 2026-11-13 → 2026-11-15) and untracked `docs/` (6 files). Four of those docs (`algorithm.md`, `architecture.md`, `benchmark_plan.md`, `parallelization_strategy.md`) also exist upstream with different content, so `git pull` would have been blocked.

**What was done:**

1. Committed all local work to a backup branch `local-m0-work` and pushed it to GitHub (nothing lost).
2. Fast-forwarded `main` to `origin/main` (`b0aa857`).
3. Restored the local-only files onto `main`: `docs/project_plan.md`, `docs/seam_carving_textbook.html`.
4. Merged the local WORKLOG entries 2026-11-13 → 2026-11-15 into the upstream WORKLOG in chronological order (pure insertion; no upstream entry changed).

**Decision:** for the four duplicated docs, `main` keeps the upstream (contributor) versions, since the code (M1–M4) was written against them. The older local versions remain on branch `local-m0-work` for comparison.

**Commands executed:** `git fetch --all --prune`, `git switch -c local-m0-work`, `git commit`, `git push -u origin local-m0-work`, `git merge --ff-only origin/main`, `git checkout local-m0-work -- docs/project_plan.md docs/seam_carving_textbook.html`.

**Tests:** none run (repository sync only; no code changed).

**Known issues / remaining:**

- Decide whether anything from the local versions of the four duplicated docs should be merged into the upstream versions.
- Status sections at the top of WORKLOG reflect upstream state.
