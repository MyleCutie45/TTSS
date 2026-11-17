// seam_omp.cpp - OpenMP version of seam carving.
// Must produce exactly the same energy map, M matrix, seams and images as seam_seq.cpp.
// Every parallel loop uses schedule(runtime): the schedule comes from OMP_SCHEDULE (or
// omp_set_schedule), so schedule experiments need no recompilation.
#include "seam_omp.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include <omp.h>
#include <opencv2/imgproc.hpp>

namespace omp_impl {

namespace {

// ---- Per-row / per-cell work, shared by the parallel and the sequential loop of each stage ----
// Keeping the arithmetic in one place guarantees that both loops compute exactly the same thing.

inline void energyRow(const cv::Mat& gray, int width, cv::Mat& energy, int y) {
    const int height = gray.rows;
    const uchar* up   = gray.ptr<uchar>(std::max(y - 1, 0));
    const uchar* cur  = gray.ptr<uchar>(y);
    const uchar* down = gray.ptr<uchar>(std::min(y + 1, height - 1));
    int* e = energy.ptr<int>(y);
    for (int x = 0; x < width; ++x) {
        const int xl = std::max(x - 1, 0);
        const int xr = std::min(x + 1, width - 1);
        e[x] = std::abs(int(cur[xr]) - int(cur[xl])) + std::abs(int(down[x]) - int(up[x]));
    }
}

inline int dpCell(const int* prev, const int* e, int width, int x) {
    int best = prev[x];
    if (x > 0)         best = std::min(best, prev[x - 1]);
    if (x < width - 1) best = std::min(best, prev[x + 1]);
    return e[x] + best;
}

inline void removeRow(cv::Mat& image, cv::Mat& gray, int width, int s, int y) {
    const int count = width - 1 - s;
    if (count <= 0) return;
    uchar* colorRow = image.ptr<uchar>(y);
    std::memmove(colorRow + 3 * s, colorRow + 3 * (s + 1), 3 * static_cast<size_t>(count));
    uchar* grayRow = gray.ptr<uchar>(y);
    std::memmove(grayRow + s, grayRow + s + 1, static_cast<size_t>(count));
}

// (value, column) is "better" if the value is smaller, or equal with a smaller column.
// This is a total order, so the minimum is unique: the result cannot depend on how the
// columns were split among threads or in which order the threads finish.
inline bool better(int value, int col, int bestValue, int bestCol) {
    return value < bestValue || (value == bestValue && col < bestCol);
}

// One slot per thread for the minimum search. alignas(64) gives every slot its own cache line,
// so threads writing their slots do not slow each other down (no false sharing).
struct alignas(64) LocalMin {
    int value = INT_MAX;
    int col = -1;
};

}  // namespace

// ---------------------------------------------------------------------------------------------
// Stage 1: energy map – every pixel is independent; threads take whole rows.
// ---------------------------------------------------------------------------------------------
void computeEnergy(const cv::Mat& gray, int width, cv::Mat& energy, const ParallelConfig& config) {
    const int height = gray.rows;
    if (config.parEnergy) {
        // Shared (read-only): gray, width. Each thread writes only its own rows of energy.
        // Implicit barrier at the end: the whole map is finished before the function returns.
        #pragma omp parallel for schedule(runtime)
        for (int y = 0; y < height; ++y) {
            energyRow(gray, width, energy, y);
        }
    } else {
        for (int y = 0; y < height; ++y) energyRow(gray, width, energy, y);
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 2: cumulative energy – row y needs the COMPLETE row y-1, so rows are processed in order;
// the cells of one row are independent and are split among the threads.
// ---------------------------------------------------------------------------------------------
void computeCumulative(const cv::Mat& energy, int width, cv::Mat& M, const ParallelConfig& config) {
    const int height = energy.rows;
    std::memcpy(M.ptr<int>(0), energy.ptr<int>(0), sizeof(int) * width);  // row 0 (O(W), sequential)

    if (!config.parDP) {
        for (int y = 1; y < height; ++y) {
            const int* prev = M.ptr<int>(y - 1);
            const int* e = energy.ptr<int>(y);
            int* m = M.ptr<int>(y);
            for (int x = 0; x < width; ++x) m[x] = dpCell(prev, e, width, x);
        }
        return;
    }

    if (!config.persistentDP) {
        // Variant A: a new parallel loop for every row.
        for (int y = 1; y < height; ++y) {               // sequential: dependency between rows
            const int* prev = M.ptr<int>(y - 1);
            const int* e = energy.ptr<int>(y);
            int* m = M.ptr<int>(y);
            #pragma omp parallel for schedule(runtime)
            for (int x = 0; x < width; ++x) {
                m[x] = dpCell(prev, e, width, x);         // reads row y-1, writes only cell x of row y
            }
            // Implicit barrier: row y is complete before the next iteration reads it.
        }
    } else {
        // Variant B: the thread team is created once; every thread runs the y loop itself
        // (y, prev, e, m are declared inside the region -> private to each thread).
        #pragma omp parallel
        {
            for (int y = 1; y < height; ++y) {
                const int* prev = M.ptr<int>(y - 1);
                const int* e = energy.ptr<int>(y);
                int* m = M.ptr<int>(y);
                #pragma omp for schedule(runtime)
                for (int x = 0; x < width; ++x) {
                    m[x] = dpCell(prev, e, width, x);
                }
                // Implicit barrier of "omp for" (NO nowait!): no thread may start row y+1
                // before ALL threads have finished row y. Removing it is a race condition.
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 3: minimum of the last row with deterministic tie-breaking (smallest column).
// ---------------------------------------------------------------------------------------------
int findMinEndpoint(const cv::Mat& M, int width, const ParallelConfig& config) {
    const int* last = M.ptr<int>(M.rows - 1);

    if (!config.parMin) {
        int bestCol = 0;
        for (int x = 1; x < width; ++x)
            if (last[x] < last[bestCol]) bestCol = x;
        return bestCol;
    }

    std::vector<LocalMin> local(omp_get_max_threads());  // >= team size of the region below
    #pragma omp parallel
    {
        int bestValue = INT_MAX, bestCol = -1;            // private to each thread
        #pragma omp for schedule(runtime) nowait
        for (int x = 0; x < width; ++x) {
            if (better(last[x], x, bestValue, bestCol)) { bestValue = last[x]; bestCol = x; }
        }
        // nowait is safe: the next line only touches this thread's own slot,
        // and the implicit barrier at the end of the parallel region follows.
        local[omp_get_thread_num()] = {bestValue, bestCol};
    }

    // Sequential combine over at most "threads" entries.
    int bestValue = INT_MAX, bestCol = -1;
    for (const LocalMin& lm : local) {
        if (lm.col >= 0 && better(lm.value, lm.col, bestValue, bestCol)) {
            bestValue = lm.value;
            bestCol = lm.col;
        }
    }
    return bestCol;
}

// ---------------------------------------------------------------------------------------------
// Stage 4: backtracking – a chain of dependent steps, stays sequential.
// ---------------------------------------------------------------------------------------------
void backtrackSeam(const cv::Mat& M, int width, int endCol, std::vector<int>& seam) {
    const int height = M.rows;
    seam.resize(height);
    seam[height - 1] = endCol;
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

// ---------------------------------------------------------------------------------------------
// Stage 5: seam removal – rows are independent; each row is shifted by exactly one thread.
// ---------------------------------------------------------------------------------------------
void removeSeam(cv::Mat& image, cv::Mat& gray, int width, const std::vector<int>& seam,
                const ParallelConfig& config) {
    const int height = image.rows;
    if (config.parRemoval) {
        #pragma omp parallel for schedule(runtime)
        for (int y = 0; y < height; ++y) {
            removeRow(image, gray, width, seam[y], y);   // writes only row y of image and gray
        }
    } else {
        for (int y = 0; y < height; ++y) removeRow(image, gray, width, seam[y], y);
    }
}

// ---------------------------------------------------------------------------------------------
// Whole algorithm (same structure as seq::carve; the outer seam loop is sequential).
// ---------------------------------------------------------------------------------------------
namespace {

void carveWidth(cv::Mat& image, int targetWidth, const ParallelConfig& config, StageTimings& t) {
    if (targetWidth == image.cols) return;

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Mat energy(image.rows, image.cols, CV_32SC1);
    cv::Mat M(image.rows, image.cols, CV_32SC1);
    std::vector<int> seam(image.rows);

    int width = image.cols;
    while (width > targetWidth) {   // seam i+1 needs the image after seam i -> sequential
        auto t0 = SeamClock::now();
        computeEnergy(gray, width, energy, config);
        t.energy += secondsSince(t0);

        t0 = SeamClock::now();
        computeCumulative(energy, width, M, config);
        t.dp += secondsSince(t0);

        t0 = SeamClock::now();
        const int endCol = findMinEndpoint(M, width, config);
        t.minEndpoint += secondsSince(t0);

        t0 = SeamClock::now();
        backtrackSeam(M, width, endCol, seam);
        t.backtrack += secondsSince(t0);

        t0 = SeamClock::now();
        removeSeam(image, gray, width, seam, config);
        t.removal += secondsSince(t0);

        --width;
    }
    image = image.colRange(0, targetWidth).clone();
}

}  // namespace

cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight,
              const ParallelConfig& config, StageTimings* timings) {
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
    if (config.threads < 1) {
        throw std::invalid_argument("carve: thread count must be >= 1");
    }

    // Sets the team size for all following parallel regions of this program.
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

int defaultThreadCount() { return omp_get_max_threads(); }

void useStaticScheduleUnlessSet() {
    const char* env = std::getenv("OMP_SCHEDULE");
    if (env == nullptr || env[0] == '\0') omp_set_schedule(omp_sched_static, 0);
}

std::string scheduleDescription() {
    omp_sched_t kind;
    int chunk = 0;
    omp_get_schedule(&kind, &chunk);
    // Remove the optional "monotonic" modifier bit (OpenMP 4.5+) before comparing.
    const int base = static_cast<int>(static_cast<unsigned>(kind) &
                                      ~static_cast<unsigned>(omp_sched_monotonic));
    std::string name;
    switch (base) {
        case omp_sched_static:  name = "static";  break;
        case omp_sched_dynamic: name = "dynamic"; break;
        case omp_sched_guided:  name = "guided";  break;
        case omp_sched_auto:    name = "auto";    break;
        default:                name = "other";   break;
    }
    if (chunk > 0) name += ":" + std::to_string(chunk);  // ':' instead of ',' keeps the CSV valid
    return name;
}

}  // namespace omp_impl
