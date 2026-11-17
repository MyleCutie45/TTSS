// seam_seq.cpp - sequential seam carving. This file must never contain OpenMP code:
// it is the reference that the OpenMP version (M2) is compared against, bit for bit.
#include "seam_seq.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include <opencv2/imgproc.hpp>

namespace seq {

// ---------------------------------------------------------------------------------------------
// Stage 1: energy map
// ---------------------------------------------------------------------------------------------
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy) {
    const int height = gray.rows;
    for (int y = 0; y < height; ++y) {
        // Rows above and below, clamped at the top and bottom border.
        const uchar* up   = gray.ptr<uchar>(std::max(y - 1, 0));
        const uchar* cur  = gray.ptr<uchar>(y);
        const uchar* down = gray.ptr<uchar>(std::min(y + 1, height - 1));
        int* e = energy.ptr<int>(y);
        for (int x = 0; x < width; ++x) {
            // Left/right neighbours clamped with the CURRENT width (not gray.cols).
            const int xl = std::max(x - 1, 0);
            const int xr = std::min(x + 1, width - 1);
            e[x] = std::abs(int(cur[xr]) - int(cur[xl])) + std::abs(int(down[x]) - int(up[x]));
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 2: cumulative minimum energy (dynamic programming)
// ---------------------------------------------------------------------------------------------
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M) {
    const int height = energy.rows;

    // Row 0: a seam that ends in row 0 consists of one pixel only.
    std::memcpy(M.ptr<int>(0), energy.ptr<int>(0), sizeof(int) * width);

    // Row y needs the finished row y-1, so the rows are processed in order.
    for (int y = 1; y < height; ++y) {
        const int* prev = M.ptr<int>(y - 1);
        const int* e    = energy.ptr<int>(y);
        int* m          = M.ptr<int>(y);
        for (int x = 0; x < width; ++x) {
            int best = prev[x];                                   // straight up
            if (x > 0)         best = std::min(best, prev[x - 1]);  // up-left (if it exists)
            if (x < width - 1) best = std::min(best, prev[x + 1]);  // up-right (if it exists)
            m[x] = e[x] + best;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 3: endpoint of the cheapest seam
// ---------------------------------------------------------------------------------------------
int findMinEndpoint(const cv::Mat& M, int width) {
    const int* last = M.ptr<int>(M.rows - 1);
    int bestCol = 0;
    for (int x = 1; x < width; ++x) {
        if (last[x] < last[bestCol]) {  // strict '<': on a tie the earlier (smaller) column stays
            bestCol = x;
        }
    }
    return bestCol;
}

// ---------------------------------------------------------------------------------------------
// Stage 4: backtracking
// ---------------------------------------------------------------------------------------------
void backtrackSeam(const cv::Mat& M, int width, int endCol, std::vector<int>& seam) {
    const int height = M.rows;
    seam.resize(height);
    seam[height - 1] = endCol;

    for (int y = height - 1; y > 0; --y) {
        const int* prev = M.ptr<int>(y - 1);
        const int x = seam[y];
        int best = -1;
        // Candidates in the order x-1, x, x+1. Strict '<' keeps the first (smallest) column on ties.
        for (int c = x - 1; c <= x + 1; ++c) {
            if (c < 0 || c >= width) continue;
            if (best == -1 || prev[c] < prev[best]) best = c;
        }
        seam[y - 1] = best;
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 5: seam removal
// ---------------------------------------------------------------------------------------------
void removeSeam(cv::Mat& image, cv::Mat& gray, int width, const std::vector<int>& seam) {
    const int height = image.rows;
    for (int y = 0; y < height; ++y) {
        const int s = seam[y];
        const int count = width - 1 - s;  // pixels to the right of the seam
        if (count <= 0) continue;         // seam is the last valid column: nothing to shift

        // Color: 3 bytes per pixel. Source and destination overlap -> memmove, not memcpy.
        uchar* colorRow = image.ptr<uchar>(y);
        std::memmove(colorRow + 3 * s, colorRow + 3 * (s + 1), 3 * static_cast<size_t>(count));

        uchar* grayRow = gray.ptr<uchar>(y);
        std::memmove(grayRow + s, grayRow + s + 1, static_cast<size_t>(count));
    }
}

// ---------------------------------------------------------------------------------------------
// Whole algorithm
// ---------------------------------------------------------------------------------------------
namespace {

// Remove vertical seams from 'image' (CV_8UC3) until it is targetWidth wide.
// On return, 'image' is a new matrix of exactly targetWidth columns.
void carveWidth(cv::Mat& image, int targetWidth, StageTimings& t) {
    if (targetWidth == image.cols) return;

    // Gray image computed ONCE; later seams are removed from it directly (decision D2).
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // All work buffers allocated once at the original size (no allocation in the loop).
    cv::Mat energy(image.rows, image.cols, CV_32SC1);
    cv::Mat M(image.rows, image.cols, CV_32SC1);
    std::vector<int> seam(image.rows);

    int width = image.cols;
    while (width > targetWidth) {
        auto t0 = SeamClock::now();
        computeEnergy(gray, width, energy);
        t.energy += secondsSince(t0);

        t0 = SeamClock::now();
        computeCumulative(energy, width, M);
        t.dp += secondsSince(t0);

        t0 = SeamClock::now();
        const int endCol = findMinEndpoint(M, width);
        t.minEndpoint += secondsSince(t0);

        t0 = SeamClock::now();
        backtrackSeam(M, width, endCol, seam);
        t.backtrack += secondsSince(t0);

        t0 = SeamClock::now();
        removeSeam(image, gray, width, seam);
        t.removal += secondsSince(t0);

        --width;
    }

    // Keep only the valid columns. clone() makes a compact, continuous copy.
    image = image.colRange(0, targetWidth).clone();
}

}  // namespace

cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight, StageTimings* timings) {
    if (input.empty() || input.type() != CV_8UC3) {
        throw std::invalid_argument("carve: input must be a non-empty 8-bit 3-channel image");
    }
    if (targetWidth < 1 || targetWidth > input.cols) {
        throw std::invalid_argument("carve: target width must be in [1, " +
                                    std::to_string(input.cols) + "]");
    }
    if (targetHeight < 1 || targetHeight > input.rows) {
        throw std::invalid_argument("carve: target height must be in [1, " +
                                    std::to_string(input.rows) + "]");
    }

    StageTimings t;  // local accumulator; copied to *timings at the end
    const auto start = SeamClock::now();

    cv::Mat image = input.clone();  // never modify the caller's image

    // 1) Width: vertical seams.
    carveWidth(image, targetWidth, t);

    // 2) Height: a horizontal seam of the image is a vertical seam of the transposed image.
    if (targetHeight < image.rows) {
        cv::Mat transposed;
        cv::transpose(image, transposed);
        carveWidth(transposed, targetHeight, t);
        cv::transpose(transposed, image);
    }

    t.total = secondsSince(start);
    if (timings != nullptr) *timings = t;
    return image;
}

}  // namespace seq
