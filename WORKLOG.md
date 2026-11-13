
# WORKLOG – Seam Carving OpenMP Project

## Project Information

**Project:** Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP

**Primary Language:** C++17

**Parallel Framework:** OpenMP

**Purpose:** Document all development activities, implementation decisions, experiments, bugs, and progress.

---

## Current Project Status

- Current milestone: Not started
- Overall status: Initial setup
- Last updated: Not started

## Milestone Progress

| Milestone | Description                   | Status      |
| --------- | ----------------------------- | ----------- |
| M0        | Research and Architecture     | Not started |
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
| Push to GitHub | Remote `main` matches local | See session output | Done |

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

# Technical Decision History

Record important architecture and algorithmic decisions here.

| Date | Decision | Reason | Alternatives |
| ---- | -------- | ------ | ------------ |
|      |          |        |              |

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

- None

**In Progress:**

- None

**Blocked:**

- None

**Next Recommended Task:**

- Start Milestone 0.
