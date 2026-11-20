// seam_opt.h - OPTIMIZED OpenMP seam carving (Milestone 5).
//
// Three versions exist side by side and are never mixed:
//   seq::      original sequential algorithm (correctness baseline, src/seam_seq.cpp)
//   omp_impl:: basic OpenMP version from M2   (src/seam_omp.cpp)
//   opt_impl:: optimized OpenMP version (this file, src/seam_opt.cpp)
//
// opt_impl produces exactly the same images as seq:: (bit for bit). Every optimization can be
// switched on and off separately (OptConfig) so that each one can be measured on its own
// (docs/optimization_report.md).
#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>

#include "seam_common.h"

struct OptConfig {
    int  threads = 1;

    // O1: after a seam is removed, shift the energy rows like the image and recompute only the
    //     two columns per row next to the removed pixel, instead of the whole energy map.
    bool incrementalEnergy = true;

    // O2: inner loops of energy and DP without border checks ("#pragma omp simd"), so the
    //     compiler can use vector (SIMD) instructions. Border columns are handled separately.
    bool simdKernels = true;

    // O3: parallel DP with fewer barriers. 0 = DP stays sequential. K >= 1 = rows are processed in
    //     blocks of K rows with ONE barrier per block; every thread also computes a few extra
    //     ("halo") cells so that it does not need its neighbours' results inside a block.
    //     K = 1 is the M2 scheme (one barrier per row).
    //     Default 64: fastest region of the M5 measurement (docs/optimization_report.md, section 4.3).
    int  dpBlockRows = 64;

    // O4: run the parallel regions only if the current image has at least this many pixels
    //     (OpenMP "if" clause). Small images are processed by one thread without any start-up cost.
    long long parallelMinPixels = 0;

    // O5: compact memory layout. Energy is stored as 16-bit values (max 510) instead of 32-bit, and
    //     the DP does not store the whole matrix M: it keeps two rows of M and one byte per pixel
    //     with the direction (-1, 0, +1) of the backtracking step. Less data is written per seam.
    bool compactMemory = true;
};

namespace opt_impl {

// ---- Stage functions (public so that tests can compare them with seq::) ----

// Full energy map of columns 0..width-1 (parallel over rows if par is true).
// energy may be CV_32SC1 or CV_16UC1 (O5).
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy, bool simd, bool par);

// Cumulative energy M (same result as seq::computeCumulative).
// blockRows = 0: sequential; blockRows >= 1: blocked parallel scheme with the current thread count.
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M, bool simd, int blockRows,
                       bool par);

// O5 version of the DP: energy16 is CV_16UC1, dir (CV_8SC1, rows 1..H-1 written) receives the
// backtracking direction of every pixel, lastRow receives the last row of M.
// Backtracking: seam[y-1] = seam[y] + dir(y, seam[y]) gives exactly seq::backtrackSeam's seam.
void computeDirections(const cv::Mat& energy16, int width, cv::Mat& dir, std::vector<int>& lastRow,
                       bool simd, int blockRows, bool par);

// Remove the seam from image and gray (like seq::removeSeam). If energy is not empty, also shift
// the energy rows and recompute the energy of the columns next to the seam, so that afterwards
// energy equals a full computeEnergy(gray, width - 1). Parallel over rows if par is true.
void removeSeam(cv::Mat& image, cv::Mat& gray, cv::Mat* energy, int width,
                const std::vector<int>& seam, bool par);

// Whole algorithm, same requirements, exceptions and result as seq::carve.
cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight, const OptConfig& config,
              StageTimings* timings);

// Short text for the CSV "stages" column, e.g. "incr+simd+dpblock16+thr0".
std::string describe(const OptConfig& config);

}  // namespace opt_impl
