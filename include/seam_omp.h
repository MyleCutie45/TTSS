// seam_omp.h - OpenMP seam carving. Same functions and same results (bit for bit) as seam_seq.h.
// Design: docs/parallelization_strategy.md. Interface contract: docs/architecture.md.
//
// Each stage function takes a ParallelConfig that says whether THAT stage runs in parallel.
// When a stage is switched off, the same loop runs without any OpenMP directive.
// Stage functions use the current OpenMP thread count and schedule (OMP_SCHEDULE / omp_set_schedule);
// only carve() changes the thread count (config.threads).
#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>

#include "seam_common.h"

namespace omp_impl {

// Stage 1 (parallel over rows if config.parEnergy).
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy,
                   const ParallelConfig& config = ParallelConfig());

// Stage 2 (rows in order, columns in parallel if config.parDP).
// config.persistentDP = false: variant A, one "parallel for" per row.
// config.persistentDP = true : variant B, one parallel region for all rows, "omp for" per row.
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M,
                       const ParallelConfig& config = ParallelConfig());

// Stage 3 (per-thread (value, column) minimum + sequential combine if config.parMin).
int findMinEndpoint(const cv::Mat& M, int width, const ParallelConfig& config = ParallelConfig());

// Stage 4: always sequential (each row depends on the row below). Same code as seq::backtrackSeam.
void backtrackSeam(const cv::Mat& M, int width, int endCol, std::vector<int>& seam);

// Stage 5 (parallel over rows if config.parRemoval).
void removeSeam(cv::Mat& image, cv::Mat& gray, int width, const std::vector<int>& seam,
                const ParallelConfig& config = ParallelConfig());

// Whole algorithm with config.threads threads (must be >= 1; calls omp_set_num_threads).
// Same requirements and exceptions as seq::carve.
cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight,
              const ParallelConfig& config, StageTimings* timings);

// Helpers for the command-line program (keeps OpenMP headers out of main.cpp).
int defaultThreadCount();            // omp_get_max_threads()
std::string scheduleDescription();   // e.g. "static", "dynamic:64", "guided:1" (from omp_get_schedule)

// If the environment variable OMP_SCHEDULE is NOT set, select schedule(static) for all
// schedule(runtime) loops. Reason: GCC's libgomp otherwise uses "dynamic,1", which hands out
// one loop iteration at a time and is very slow for the short DP rows (measured, WORKLOG M2).
void useStaticScheduleUnlessSet();

}  // namespace omp_impl
