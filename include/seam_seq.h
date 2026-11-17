// seam_seq.h - sequential seam carving (the correctness baseline).
// Algorithm definition: docs/algorithm.md. Interface contract: docs/architecture.md.
//
// Conventions used by every function:
//  - Matrices keep their ORIGINAL size during carving. Only columns 0..width-1 are valid;
//    columns >= width contain leftovers and are never read.
//  - gray: CV_8UC1, image: CV_8UC3, energy and M: CV_32SC1, all with the same number of rows.
//  - Ties are always broken in favour of the smallest column.
#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "seam_common.h"

namespace seq {

// Stage 1: e(x,y) = |I(x+1,y) - I(x-1,y)| + |I(x,y+1) - I(x,y-1)|, indices clamped to the border
// (right border = width-1). energy must already be allocated as CV_32SC1 with gray's size.
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy);

// Stage 2: M(x,0) = e(x,0); M(x,y) = e(x,y) + min of the (up to 3) upper neighbours.
// M must already be allocated as CV_32SC1 with energy's size.
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M);

// Stage 3: column of the smallest value in the last row of M (smallest column on ties).
int findMinEndpoint(const cv::Mat& M, int width);

// Stage 4: follow the smallest upper neighbour (order x-1, x, x+1; smallest column on ties)
// from (endCol, last row) to row 0. seam is resized to M.rows; seam[y] = column in row y.
void backtrackSeam(const cv::Mat& M, int width, int endCol, std::vector<int>& seam);

// Stage 5: delete pixel seam[y] from every row of image and gray by shifting the pixels to its
// right one position left. Afterwards only columns 0..width-2 are valid (caller decrements width).
void removeSeam(cv::Mat& image, cv::Mat& gray, int width, const std::vector<int>& seam);

// Whole algorithm: reduce a CV_8UC3 image to targetWidth x targetHeight.
// Width is reduced first with vertical seams, then height with transpose + vertical seams.
// Requirements: 1 <= targetWidth <= input.cols, 1 <= targetHeight <= input.rows,
// otherwise std::invalid_argument is thrown. timings may be nullptr; if given, it is
// overwritten (not added to).
cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight, StageTimings* timings);

}  // namespace seq
