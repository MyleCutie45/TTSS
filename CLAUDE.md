
# CLAUDE.md – Project Development Instructions

## 1. Project Context

This project implements:

"Parallel Implementation of Seam Carving for Content-Aware Image Resizing using OpenMP"

The project uses:

- C++17
- OpenMP
- OpenCV
- CMake
- Python for benchmarking and visualization

Read README.md and relevant documentation before making changes.

## 2. Mandatory Work Logging

**WORKLOG.md is the permanent development journal of this project.**

You MUST read WORKLOG.md at the beginning of every working session.

You MUST update WORKLOG.md whenever you perform meaningful work, including:

- Creating new files.
- Modifying existing files.
- Implementing functions or algorithms.
- Fixing bugs.
- Changing architecture or implementation decisions.
- Running tests.
- Running benchmarks.
- Optimizing performance.
- Discovering problems or limitations.

Do not wait until a milestone is completed to update the work log.

After completing each meaningful task:

1. Update WORKLOG.md.
2. Explain what was implemented.
3. Explain why the change was necessary.
4. List the files created or modified.
5. Describe the important technical decisions.
6. Record commands executed.
7. Record actual test results.
8. Document known issues and remaining tasks.

Never claim a test or benchmark passed unless it was actually executed successfully.

If a task is incomplete, explicitly state its current status.

## 3. Mandatory Explanation Rules

I am a university student learning Parallel Computing.

Do not treat this project as a black-box coding task.

Whenever you implement or modify code, explain:

- What the code does.
- Why it is needed.
- How the algorithm works.
- How the data flows through the implementation.
- What important variables represent.
- What OpenMP directives do.
- How threads cooperate.
- Where synchronization happens.
- What potential race conditions exist.
- What performance implications the implementation has.

Explanations should be clear and understandable to an undergraduate student.

Avoid unnecessarily advanced terminology without explaining it.

## 4. Code Modification Rules

- Read existing code before modifying it.
- Do not overwrite working implementations without justification.
- Preserve the sequential implementation as the correctness baseline.
- Keep sequential and parallel implementations separate.
- Prefer simple, readable C++.
- Avoid unnecessary abstractions.
- Avoid premature optimization.
- Do not introduce unverified algorithmic changes.
- Maintain consistent naming and formatting.

## 5. Testing Rules

After modifying code:

1. Build the affected targets.
2. Run relevant tests.
3. Check for compilation errors.
4. Check for runtime errors.
5. Verify sequential and parallel correctness where applicable.
6. Record actual results in WORKLOG.md.

If tests cannot be executed, explain why.

## 6. Benchmarking Rules

Never fabricate performance numbers.

Record:

- Hardware configuration.
- Compiler and compiler flags.
- OpenMP thread count.
- Input image dimensions.
- Target image dimensions.
- Execution time.
- Number of repetitions.
- Measurement methodology.

Clearly distinguish measured results from estimates.

## 7. Session Completion Summary

At the end of every working session, provide a concise summary containing:

1. What was accomplished.
2. Files created or modified.
3. Important implementation details.
4. Tests and commands executed.
5. Actual results.
6. Problems encountered.
7. Remaining tasks.
8. Recommended next step.

Update WORKLOG.md before ending the session.

## 8. Important Restrictions

- Do not silently skip requirements.
- Do not hide implementation problems.
- Do not claim completion when functionality is incomplete.
- Do not modify unrelated files without a reason.
- Do not delete historical work log entries.
- Always append new entries chronologically.
- If information is unknown, explicitly mark it as unknown.

**These instructions are mandatory throughout the entire project.**
