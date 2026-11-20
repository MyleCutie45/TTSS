// seam_opt.cpp - optimized OpenMP version of seam carving (Milestone 5).
// Must produce exactly the same seams and images as seam_seq.cpp; see docs/optimization_report.md
// for the measurements behind every optimization.
//
// Optimizations (each can be switched off, see OptConfig in seam_opt.h):
//   O1 incremental energy  - after a seam removal only 2 energy values per row are recomputed
//   O2 SIMD kernels        - branch-free inner loops with "#pragma omp simd"
//   O3 blocked parallel DP - one barrier per K rows instead of one per row ("halo" cells)
//   O4 size threshold      - "if" clause: no parallel region for small images
//   O5 compact memory      - 16-bit energy; DP keeps only two rows of M plus a 1-byte direction
//                            per pixel for the backtracking (less memory traffic)
// Unchanged from M2 (measured in M4): minimum search and backtracking stay sequential.
#include "seam_opt.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include <omp.h>
#include <opencv2/imgproc.hpp>

namespace opt_impl {

namespace {

// ---------------------------------------------------------------------------------------------
// Energy: e(x,y) = |I(x+1,y) - I(x-1,y)| + |I(x,y+1) - I(x,y-1)|, indices clamped to the border.
// The value is at most 2 * 255 = 510, so it also fits into 16 bits (O5).
// ---------------------------------------------------------------------------------------------

inline int energyAt(const uchar* up, const uchar* cur, const uchar* down, int width, int x) {
    const int xl = std::max(x - 1, 0);
    const int xr = std::min(x + 1, width - 1);
    return std::abs(int(cur[xr]) - int(cur[xl])) + std::abs(int(down[x]) - int(up[x]));
}

// Energy of columns [x0, x1) of one row; T = int (CV_32S) or uint16_t (CV_16U).
// simd = false: plain loop with clamping in every iteration.
// simd = true : columns 1..width-2 never need clamping, so they use a loop without min/max or
//               branches. "#pragma omp simd" tells the compiler that the iterations are independent,
//               so it may compute several pixels with one vector instruction. Only the border
//               columns 0 and width-1 use energyAt.
template <typename T>
void energyRowT(const uchar* up, const uchar* cur, const uchar* down, int width, T* e, int x0, int x1,
                bool simd) {
    if (!simd) {
        for (int x = x0; x < x1; ++x) e[x] = static_cast<T>(energyAt(up, cur, down, width, x));
        return;
    }
    const int lo = std::max(x0, 1);           // first interior column
    const int hi = std::min(x1, width - 1);   // one past the last interior column
    for (int x = x0; x < std::min(x1, lo); ++x) e[x] = static_cast<T>(energyAt(up, cur, down, width, x));
    #pragma omp simd
    for (int x = lo; x < hi; ++x) {
        e[x] = static_cast<T>(std::abs(int(cur[x + 1]) - int(cur[x - 1])) +
                              std::abs(int(down[x]) - int(up[x])));
    }
    for (int x = std::max(std::max(hi, x0), std::min(x1, lo)); x < x1; ++x)
        e[x] = static_cast<T>(energyAt(up, cur, down, width, x));
}

void energyRowRange(const cv::Mat& gray, int width, cv::Mat& energy, int y, int x0, int x1,
                    bool simd) {
    const int height = gray.rows;
    const uchar* up   = gray.ptr<uchar>(std::max(y - 1, 0));
    const uchar* cur  = gray.ptr<uchar>(y);
    const uchar* down = gray.ptr<uchar>(std::min(y + 1, height - 1));
    if (energy.depth() == CV_16U) energyRowT(up, cur, down, width, energy.ptr<uint16_t>(y), x0, x1, simd);
    else                          energyRowT(up, cur, down, width, energy.ptr<int>(y), x0, x1, simd);
}

// ---------------------------------------------------------------------------------------------
// DP: M(x,y) = e(x,y) + min(M(x-1,y-1), M(x,y-1), M(x+1,y-1)) (neighbours outside the image ignored)
// ---------------------------------------------------------------------------------------------

template <typename E>
inline int dpAt(const int* prev, const E* e, int width, int x) {
    int best = prev[x];
    if (x > 0)         best = std::min(best, prev[x - 1]);
    if (x < width - 1) best = std::min(best, prev[x + 1]);
    return int(e[x]) + best;
}

// Columns [x0, x1) of one DP row. prev = previous row, out = result; both indexed with the real
// column number. min() of integers does not depend on the order of its operands, so the result is
// bit-identical to seq::computeCumulative.
template <typename E>
void dpRowRange(const int* prev, const E* e, int* out, int width, int x0, int x1, bool simd) {
    if (!simd) {
        for (int x = x0; x < x1; ++x) out[x] = dpAt(prev, e, width, x);
        return;
    }
    const int lo = std::max(x0, 1);
    const int hi = std::min(x1, width - 1);
    for (int x = x0; x < std::min(x1, lo); ++x) out[x] = dpAt(prev, e, width, x);
    #pragma omp simd
    for (int x = lo; x < hi; ++x) {
        out[x] = int(e[x]) + std::min(prev[x - 1], std::min(prev[x], prev[x + 1]));
    }
    for (int x = std::max(std::max(hi, x0), std::min(x1, lo)); x < x1; ++x)
        out[x] = dpAt(prev, e, width, x);
}

// O5: direction of the backtracking step from (x, y) to row y-1: -1, 0 or +1.
// Same rule as seq::backtrackSeam: the FIRST smallest of prev[x-1], prev[x], prev[x+1]
// (strict "<" in the order x-1, x, x+1), i.e. ties go to the smallest column.
inline int8_t dirAt(const int* prev, int width, int x) {
    int best = x;
    if (x > 0 && prev[x - 1] <= prev[x]) best = x - 1;
    if (x < width - 1 && prev[x + 1] < prev[best]) best = x + 1;
    return static_cast<int8_t>(best - x);
}

void dirRowRange(const int* prev, int8_t* d, int width, int x0, int x1, bool simd) {
    if (!simd) {
        for (int x = x0; x < x1; ++x) d[x] = dirAt(prev, width, x);
        return;
    }
    const int lo = std::max(x0, 1);
    const int hi = std::min(x1, width - 1);
    for (int x = x0; x < std::min(x1, lo); ++x) d[x] = dirAt(prev, width, x);
    #pragma omp simd
    for (int x = lo; x < hi; ++x) {
        const int a = prev[x - 1], b = prev[x], c = prev[x + 1];
        // left if a is a minimum; else middle if b <= c; else right (selects, no branches needed)
        d[x] = static_cast<int8_t>((a <= b && a <= c) ? -1 : (b <= c ? 0 : 1));
    }
    for (int x = std::max(std::max(hi, x0), std::min(x1, lo)); x < x1; ++x)
        d[x] = dirAt(prev, width, x);
}

// Two private rows per thread, reused across calls (allocated only when a wider image appears).
// thread_local: every OpenMP thread has its own pair, so no synchronization is needed.
struct RowBuffers {
    std::vector<int> a, b;
    void ensure(int width) {
        if (static_cast<int>(a.size()) < width) { a.resize(width); b.resize(width); }
    }
};
RowBuffers& threadBuffers() {
    static thread_local RowBuffers buffers;
    return buffers;
}

// Generic blocked DP driver (O3), used with and without O5.
//   sharedPrev(y)              : shared row y (complete), read in the first row of a block
//   publish(y, prev, cur, a, b): store the thread's own cells [a, b) of row y (into M, or with O5
//                                the directions + the block's last row)
// See computeCumulative for the explanation of the blocking.
template <typename E, typename PrevRow, typename Publish>
void blockedDP(const cv::Mat& energy, int width, int K, bool simd, PrevRow sharedPrev, Publish publish) {
    const int height = energy.rows;
    #pragma omp parallel
    {
        const int T = omp_get_num_threads();
        const int t = omp_get_thread_num();
        // Static, contiguous column split. Everything declared here is private to the thread.
        const int a = static_cast<int>(static_cast<long long>(width) * t / T);
        const int b = static_cast<int>(static_cast<long long>(width) * (t + 1) / T);
        RowBuffers& buf = threadBuffers();
        buf.ensure(width);
        int* bufPrev = buf.a.data();
        int* bufCur  = buf.b.data();

        for (int y0 = 1; y0 < height; y0 += K) {        // every thread runs the same block loop
            const int rows = std::min(K, height - y0);
            if (a < b) {                                 // a thread may own no column (width < T)
                for (int j = 0; j < rows; ++j) {
                    const int y = y0 + j;
                    const int extra = rows - 1 - j;      // halo width still needed below this row
                    const int lo = std::max(0, a - extra);
                    const int hi = std::min(width, b + extra);
                    const int* prev = (j == 0) ? sharedPrev(y - 1) : bufPrev;
                    dpRowRange(prev, energy.ptr<E>(y), bufCur, width, lo, hi, simd);
                    publish(y, prev, bufCur, a, b);      // only the thread's own cells [a, b)
                    std::swap(bufPrev, bufCur);
                }
            }
            // Synchronization point: the last row of this block must be complete in shared memory
            // before any thread reads it as "row y0-1" of the next block.
            #pragma omp barrier
        }
    }
}

// Delete column s from one row of the color image, the gray image and (optionally) the energy map.
inline void removeRow(cv::Mat& image, cv::Mat& gray, cv::Mat* energy, int width, int s, int y) {
    const int count = width - 1 - s;      // number of pixels right of the seam
    if (count <= 0) return;
    uchar* colorRow = image.ptr<uchar>(y);
    std::memmove(colorRow + 3 * s, colorRow + 3 * (s + 1), 3 * static_cast<size_t>(count));
    uchar* grayRow = gray.ptr<uchar>(y);
    std::memmove(grayRow + s, grayRow + s + 1, static_cast<size_t>(count));
    if (energy != nullptr) {
        const size_t es = energy->elemSize();           // 4 bytes (int) or 2 bytes (O5)
        uchar* e = energy->ptr<uchar>(y);
        std::memmove(e + es * s, e + es * (s + 1), es * static_cast<size_t>(count));
    }
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Stage 1: full energy map (used once per carveWidth call with O1, every seam without O1).
// ---------------------------------------------------------------------------------------------
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy, bool simd, bool par) {
    const int height = gray.rows;
    // "if(par)": when par is false the region is executed by the calling thread alone,
    // without waking up the other threads (O4, size threshold).
    #pragma omp parallel for schedule(runtime) if(par)
    for (int y = 0; y < height; ++y) {
        energyRowRange(gray, width, energy, y, 0, width, simd);
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 2: cumulative energy (full matrix M, without O5).
//
// Blocked scheme (O3), blockRows = K. The rows are processed in blocks of K rows. Thread t owns
// the columns [a, b) and writes only those cells of M. To compute its cells in row y0+j of the block
// it needs columns [a-1, b+1) of row y0+j-1, which partly belong to its neighbours. Instead of waiting
// for them (a barrier per row), it computes those cells itself in a private buffer. Going up one row
// the needed range grows by one column on each side, so in local row j the thread computes
//     columns [a - (K-1-j), b + (K-1-j))            ("trapezoid": wide at the top, exact at the bottom)
// and in row 0 of the block it reads [a-K, b+K) of the last row of the PREVIOUS block from M.
// That row is complete because there is a barrier after every block.
//
// Cost: about K*(K-1) extra cells per thread and block, i.e. (K-1) extra cells per row and thread;
// gain: H/K barriers instead of H. Every cell is still computed with the same formula and the same
// inputs, so M is identical to the sequential M.
// ---------------------------------------------------------------------------------------------
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M, bool simd, int blockRows,
                       bool par) {
    const int height = energy.rows;
    std::memcpy(M.ptr<int>(0), energy.ptr<int>(0), sizeof(int) * width);

    if (blockRows <= 0 || !par) {    // sequential DP
        for (int y = 1; y < height; ++y) {
            dpRowRange(M.ptr<int>(y - 1), energy.ptr<int>(y), M.ptr<int>(y), width, 0, width, simd);
        }
        return;
    }
    blockedDP<int>(energy, width, blockRows, simd,
                   [&](int y) { return M.ptr<int>(y); },
                   [&](int y, const int*, const int* cur, int a, int b) {
                       std::memcpy(M.ptr<int>(y) + a, cur + a, sizeof(int) * (b - a));
                   });
}

// ---------------------------------------------------------------------------------------------
// Stage 2 with O5: no full matrix M. For the backtracking only the DIRECTION of the best upper
// neighbour of every pixel is needed (dir, 1 byte per pixel) plus the last row of M for the
// minimum search (lastRow). M itself lives in two rows that are reused for every image row.
// Shared state of the blocked version: two "boundary" rows, used alternately by consecutive blocks
// (double buffering). Block k reads boundary[k % 2] (last row of block k-1) and writes its own last
// row into boundary[(k+1) % 2]. Reading and writing therefore never touch the same buffer inside a
// block, and the barrier after each block makes the written row visible to the next block.
// ---------------------------------------------------------------------------------------------
void computeDirections(const cv::Mat& energy16, int width, cv::Mat& dir, std::vector<int>& lastRow,
                       bool simd, int blockRows, bool par) {
    const int height = energy16.rows;
    lastRow.resize(std::max<size_t>(lastRow.size(), static_cast<size_t>(width)));
    const uint16_t* e0 = energy16.ptr<uint16_t>(0);

    if (blockRows <= 0 || !par) {    // sequential DP, rolling rows
        RowBuffers& buf = threadBuffers();
        buf.ensure(width);
        int* prev = buf.a.data();
        int* cur  = buf.b.data();
        for (int x = 0; x < width; ++x) prev[x] = e0[x];
        for (int y = 1; y < height; ++y) {
            dpRowRange(prev, energy16.ptr<uint16_t>(y), cur, width, 0, width, simd);
            dirRowRange(prev, dir.ptr<int8_t>(y), width, 0, width, simd);
            std::swap(prev, cur);
        }
        std::memcpy(lastRow.data(), prev, sizeof(int) * width);
        return;
    }

    std::vector<int> boundary0(width), boundary1(width);
    for (int x = 0; x < width; ++x) boundary0[x] = e0[x];     // row 0 = "last row of block -1"
    const int K = blockRows;
    // Row y of the image belongs to block (y-1)/K. The last row of block k is read by block k+1.
    // Block k reads boundary[k % 2] and its last row is written into boundary[(k+1) % 2].
    int* bnd[2] = {boundary0.data(), boundary1.data()};
    blockedDP<uint16_t>(energy16, width, K, simd,
        [&](int yPrev) { return bnd[((yPrev - 1 + K) / K) % 2]; },     // yPrev = y0-1 of block k
        [&](int y, const int* prev, const int* cur, int a, int b) {
            dirRowRange(prev, dir.ptr<int8_t>(y), width, a, b, simd);
            const int k = (y - 1) / K;
            const bool lastOfBlock = (y == height - 1) || ((y - 1) % K == K - 1);
            if (lastOfBlock)
                std::memcpy(bnd[(k + 1) % 2] + a, cur + a, sizeof(int) * (b - a));
        });
    const int lastBlock = (height - 2) / K;                       // block containing row height-1
    const int* last = (height == 1) ? bnd[0] : bnd[(lastBlock + 1) % 2];
    std::memcpy(lastRow.data(), last, sizeof(int) * width);
}

// ---------------------------------------------------------------------------------------------
// Stage 5: seam removal + (O1) local energy update, in ONE parallel region.
//
// Why only columns s-1 and s of row y (s = seam[y], new width W' = width-1)?
// After the shift, new column x holds old column x (x < s) or old column x+1 (x >= s). The new
// energy at x uses the row's pixels x-1, x+1 and the pixels x of rows y-1 and y+1, whose seams
// s' satisfy |s' - s| <= 1 (seams are 8-connected).
//   * x <= s-2: all four pixels are left of every seam involved -> unchanged, value = old e[x].
//   * x >= s+1: all four pixels moved one place left -> value = old e[x+1] (the memmove).
//     (The border clamp at the new right edge also maps to the old clamp.)
//   * only x = s-1 and x = s can change and are recomputed.
// tests/test_opt.cpp checks this claim against a full recomputation after every seam.
// ---------------------------------------------------------------------------------------------
void removeSeam(cv::Mat& image, cv::Mat& gray, cv::Mat* energy, int width,
                const std::vector<int>& seam, bool par) {
    const int height = image.rows;
    #pragma omp parallel if(par)
    {
        #pragma omp for schedule(runtime)
        for (int y = 0; y < height; ++y) {
            removeRow(image, gray, energy, width, seam[y], y);   // writes only row y
        }
        // Implicit barrier of "omp for": every gray row is shifted before the loop below reads the
        // rows y-1 and y+1 (which belong to other threads). Without it: race condition.
        if (energy != nullptr) {
            const int newWidth = width - 1;
            #pragma omp for schedule(runtime)
            for (int y = 0; y < height; ++y) {
                const int s = seam[y];
                const int x0 = std::max(0, s - 1);
                const int x1 = std::min(newWidth, s + 1);
                energyRowRange(gray, newWidth, *energy, y, x0, x1, false);  // 1-2 values: no SIMD
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Whole algorithm. Same structure as seq::carve; the outer seam loop is sequential.
// ---------------------------------------------------------------------------------------------
namespace {

int minEndpoint(const int* last, int width) {        // sequential (M4: parallel is slower)
    int endCol = 0;
    for (int x = 1; x < width; ++x)
        if (last[x] < last[endCol]) endCol = x;
    return endCol;
}

void carveWidth(cv::Mat& image, int targetWidth, const OptConfig& cfg, StageTimings& t) {
    if (targetWidth == image.cols) return;

    const int height = image.rows;
    const bool compact = cfg.compactMemory;
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Mat energy(height, image.cols, compact ? CV_16UC1 : CV_32SC1);
    cv::Mat M, dir;                                 // M without O5, dir with O5
    if (compact) dir.create(height, image.cols, CV_8SC1);
    else         M.create(height, image.cols, CV_32SC1);
    std::vector<int> lastRow(image.cols);
    std::vector<int> seam(height);

    int width = image.cols;
    bool energyValid = false;      // O1: true while "energy" matches the current gray image
    while (width > targetWidth) {
        const bool par = cfg.threads > 1 &&
                         static_cast<long long>(height) * width >= cfg.parallelMinPixels;

        auto t0 = SeamClock::now();
        if (!energyValid) computeEnergy(gray, width, energy, cfg.simdKernels, par);
        energyValid = cfg.incrementalEnergy;
        t.energy += secondsSince(t0);

        t0 = SeamClock::now();
        if (compact) computeDirections(energy, width, dir, lastRow, cfg.simdKernels, cfg.dpBlockRows, par);
        else         computeCumulative(energy, width, M, cfg.simdKernels, cfg.dpBlockRows, par);
        t.dp += secondsSince(t0);

        t0 = SeamClock::now();
        const int endCol = minEndpoint(compact ? lastRow.data() : M.ptr<int>(height - 1), width);
        t.minEndpoint += secondsSince(t0);

        t0 = SeamClock::now();
        seam[height - 1] = endCol;
        if (compact) {
            for (int y = height - 1; y > 0; --y) seam[y - 1] = seam[y] + dir.ptr<int8_t>(y)[seam[y]];
        } else {                                       // same code as seq::backtrackSeam
            for (int y = height - 1; y > 0; --y) {
                const int* prev = M.ptr<int>(y - 1);
                const int x = seam[y];
                int best = -1;
                for (int c = x - 1; c <= x + 1; ++c) {
                    if (c < 0 || c >= width) continue;
                    if (best == -1 || prev[c] < prev[best]) best = c;
                }
                seam[y - 1] = best;
            }
        }
        t.backtrack += secondsSince(t0);

        // With O1 this time also contains the local energy update (2 values per row).
        t0 = SeamClock::now();
        removeSeam(image, gray, cfg.incrementalEnergy ? &energy : nullptr, width, seam, par);
        t.removal += secondsSince(t0);

        --width;
    }
    image = image.colRange(0, targetWidth).clone();
}

}  // namespace

cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight, const OptConfig& config,
              StageTimings* timings) {
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
    if (config.threads < 1) throw std::invalid_argument("carve: thread count must be >= 1");
    if (config.dpBlockRows < 0) throw std::invalid_argument("carve: dpBlockRows must be >= 0");

    omp_set_num_threads(config.threads);

    StageTimings t;
    const auto start = SeamClock::now();
    cv::Mat image = input.clone();

    carveWidth(image, targetWidth, config, t);
    if (targetHeight < image.rows) {
        cv::Mat transposed;
        cv::transpose(image, transposed);
        carveWidth(transposed, targetHeight, config, t);
        cv::transpose(transposed, image);
    }

    t.total = secondsSince(start);
    if (timings != nullptr) *timings = t;
    return image;
}

std::string describe(const OptConfig& c) {
    std::string s = c.incrementalEnergy ? "incr" : "full";
    s += c.simdKernels ? "+simd" : "+scalar";
    if (c.compactMemory) s += "+compact";
    s += c.dpBlockRows > 0 ? "+dpblock" + std::to_string(c.dpBlockRows) : "+dpseq";
    s += "+thr" + std::to_string(c.parallelMinPixels);
    return s;
}

}  // namespace opt_impl
