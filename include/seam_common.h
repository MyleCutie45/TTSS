// seam_common.h - types shared by the sequential and the OpenMP implementation.
// The meaning of every field is part of the team contract (docs/architecture.md, section 5).
#pragma once

#include <chrono>

// Time spent in each stage, in seconds, summed over all removed seams.
// Image loading/saving is never included.
struct StageTimings {
    double energy      = 0.0;  // stage 1: computeEnergy
    double dp          = 0.0;  // stage 2: computeCumulative
    double minEndpoint = 0.0;  // stage 3: findMinEndpoint
    double backtrack   = 0.0;  // stage 4: backtrackSeam
    double removal     = 0.0;  // stage 5: removeSeam
    double total       = 0.0;  // whole carve() call (gray conversion, transposes, crop included)
};

// Which stages of the OpenMP version run in parallel (used from M2 on).
// The schedule is NOT stored here: loops use schedule(runtime) and OMP_SCHEDULE.
struct ParallelConfig {
    int  threads      = 1;
    bool parEnergy    = true;
    bool parDP        = true;
    bool persistentDP = false;  // false: variant A (parallel for per row), true: variant B (one region)
    bool parMin       = true;
    bool parRemoval   = true;
};

// Small timing helper: steady_clock never jumps backwards (unlike the wall clock).
using SeamClock = std::chrono::steady_clock;

inline double secondsSince(SeamClock::time_point start) {
    return std::chrono::duration<double>(SeamClock::now() - start).count();
}
