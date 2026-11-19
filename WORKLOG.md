
# WORKLOG – Seam Carving OpenMP Project

## Project Information

**Project:** Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP

**Primary Language:** C++17

**Parallel Framework:** OpenMP

**Purpose:** Document all development activities, implementation decisions, experiments, bugs, and progress.

---

## Current Project Status

- Current milestone: M0 – Research and Architecture
- Overall status: Planning; blocked on toolchain (Smart App Control blocks executables, OpenCV missing)
- Last updated: 2026-11-13

## Milestone Progress

| Milestone | Description                   | Status      |
| --------- | ----------------------------- | ----------- |
| M0        | Research and Architecture     | In progress |
| M1        | Sequential Implementation     | Not started |
| M2        | OpenMP Implementation         | Not started |
| M3        | Correctness Testing           | Not started |
| M4        | Performance Benchmarking      | Not started |
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

| File | Action | Description |
| ---- | ------ | ----------- |
| .gitignore | Created | Ignore paper PDF, build outputs, editor/Python files |
| WORKLOG.md | Modified | Added this entry |

#### 5. Commands Executed

```bash
git init -b main
git add .
git commit -m "Initial commit: project plan and docs"
gh repo create TTSS --public --source=. --remote=origin --push
```

#### 6. Testing and Results

| Test | Expected Result | Actual Result | Status |
| ---- | --------------- | ------------- | ------ |
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

---

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
