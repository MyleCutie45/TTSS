// test_m3.cpp - additional correctness tests for Milestone 3 (see docs/testing_report.md).
//
// The existing tests (test_seq.cpp, test_equivalence.cpp) are kept unchanged. This file closes the
// gaps listed in docs/testing_report.md, section 3:
//   * an INDEPENDENT reference implementation (written differently from seam_seq.cpp), so that
//     seq and omp are not only compared with each other but also with a third implementation;
//   * edge-case images for the OpenMP version (1 column, 1 row, 1x1, narrower than the thread count);
//   * seams forced into the first / last column;
//   * maximum-contrast images (energy 510);
//   * brute force on non-square shapes;
//   * connectivity of EVERY seam of a long run;
//   * large images (1920x1080, 3840x2160);
//   * odd thread counts and more threads than rows/columns.
//
// Usage:  test_m3            full test set
//         test_m3 --quick    reduced parameter sets (for frequent runs)
//
// All data are integers, so every comparison is exact (no floating-point tolerance is needed).
#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include <omp.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "seam_omp.h"
#include "seam_seq.h"

static int g_checks = 0;
static int g_failures = 0;
static bool g_quick = false;

// Records one check; prints at most 30 failure messages.
static void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        if (g_failures <= 30) std::printf("  FAILED: %s\n", what.c_str());
    }
}

// =============================================================================================
// Configurations
// =============================================================================================
struct Variant {
    const char* name;
    ParallelConfig cfg;
};

static ParallelConfig makeCfg(bool e, bool dp, bool persistent, bool mn, bool rm) {
    ParallelConfig c;
    c.parEnergy = e; c.parDP = dp; c.persistentDP = persistent; c.parMin = mn; c.parRemoval = rm;
    return c;
}

static const std::vector<Variant> kVariants = {
    {"none",    makeCfg(false, false, false, false, false)},
    {"energy",  makeCfg(true,  false, false, false, false)},
    {"dp(A)",   makeCfg(false, true,  false, false, false)},
    {"dp(B)",   makeCfg(false, true,  true,  false, false)},
    {"min",     makeCfg(false, false, false, true,  false)},
    {"removal", makeCfg(false, false, false, false, true)},
    {"all(A)",  makeCfg(true,  true,  false, true,  true)},
    {"all(B)",  makeCfg(true,  true,  true,  true,  true)},
};
static const Variant& kAllA = kVariants[6];
static const Variant& kAllB = kVariants[7];

struct Schedule {
    const char* name;
    omp_sched_t kind;
    int chunk;  // 0 = default chunk
};
static const Schedule kStatic  = {"static",   omp_sched_static,  0};
static const Schedule kStatic1 = {"static:1", omp_sched_static,  1};
static const Schedule kDynamic = {"dynamic",  omp_sched_dynamic, 0};
static const Schedule kGuided  = {"guided",   omp_sched_guided,  0};

static void useSchedule(const Schedule& s) { omp_set_schedule(s.kind, s.chunk); }

// =============================================================================================
// Small helpers
// =============================================================================================
static bool sameCols(const cv::Mat& a, const cv::Mat& b, int width) {
    if (a.type() != b.type() || a.rows != b.rows) return false;
    if (width == 0) return true;
    cv::Mat diff;
    cv::compare(a.colRange(0, width), b.colRange(0, width), diff, cv::CMP_NE);
    return cv::countNonZero(diff.reshape(1)) == 0;
}

static bool identical(const cv::Mat& a, const cv::Mat& b) {
    return a.size() == b.size() && a.type() == b.type() && sameCols(a, b, a.cols);
}

// Seam validity: one entry per row, inside [0, width), neighbours differ by at most 1.
static bool validSeam(const std::vector<int>& seam, int height, int width) {
    if ((int)seam.size() != height) return false;
    for (int y = 0; y < height; ++y) {
        if (seam[y] < 0 || seam[y] >= width) return false;
        if (y > 0 && std::abs(seam[y] - seam[y - 1]) > 1) return false;
    }
    return true;
}

static std::string sz(const cv::Mat& m) {
    return std::to_string(m.cols) + "x" + std::to_string(m.rows);
}

// Deterministic pseudo-random color image (std::mt19937 output is fixed by the C++ standard,
// so every compiler and platform produces exactly the same pixels).
static cv::Mat randomColor(int rows, int cols, unsigned seed) {
    std::mt19937 gen(seed);
    cv::Mat m(rows, cols, CV_8UC3);
    for (int y = 0; y < rows; ++y) {
        cv::Vec3b* p = m.ptr<cv::Vec3b>(y);
        for (int x = 0; x < cols; ++x)
            p[x] = cv::Vec3b((uchar)(gen() % 256), (uchar)(gen() % 256), (uchar)(gen() % 256));
    }
    return m;
}

// Gray image -> 3-channel image with B = G = R = gray (cvtColor back to gray gives the same values).
static cv::Mat grayToColor(const cv::Mat& gray) {
    cv::Mat color;
    cv::cvtColor(gray, color, cv::COLOR_GRAY2BGR);
    return color;
}

static cv::Mat loadTestImage(const char* name) {
    return cv::imread(std::string(TEST_DATA_DIR) + "/" + name, cv::IMREAD_COLOR);
}

// =============================================================================================
// Independent reference implementation.
// Deliberately written in a different style than seam_seq.cpp: element access with at<>(),
// explicit clamp() function, 64-bit sums, and tie-breaking by comparing (value, column) pairs.
// If seq and this reference agree, a mistake would have to be made twice in two different ways.
// =============================================================================================
namespace ref {

static int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// e(x, y) = |I(x+1, y) - I(x-1, y)| + |I(x, y+1) - I(x, y-1)|, coordinates clamped to the
// valid area [0, width-1] x [0, rows-1].
static cv::Mat energy(const cv::Mat& gray, int width) {
    cv::Mat e(gray.rows, width, CV_32SC1);
    for (int y = 0; y < gray.rows; ++y)
        for (int x = 0; x < width; ++x) {
            const int l = gray.at<uchar>(y, clampInt(x - 1, 0, width - 1));
            const int r = gray.at<uchar>(y, clampInt(x + 1, 0, width - 1));
            const int u = gray.at<uchar>(clampInt(y - 1, 0, gray.rows - 1), x);
            const int d = gray.at<uchar>(clampInt(y + 1, 0, gray.rows - 1), x);
            e.at<int>(y, x) = std::abs(r - l) + std::abs(d - u);
        }
    return e;
}

// M(x, y) = e(x, y) + min(M(x-1, y-1), M(x, y-1), M(x+1, y-1)), neighbours outside [0, width) skipped.
// Computed with int64 so an overflow in the int32 version would show up as a mismatch.
static std::vector<std::vector<int64_t>> cumulative(const cv::Mat& e, int width) {
    std::vector<std::vector<int64_t>> M(e.rows, std::vector<int64_t>(width));
    for (int x = 0; x < width; ++x) M[0][x] = e.at<int>(0, x);
    for (int y = 1; y < e.rows; ++y)
        for (int x = 0; x < width; ++x) {
            int64_t best = INT64_MAX;
            for (int dx = -1; dx <= 1; ++dx) {
                const int c = x + dx;
                if (c >= 0 && c < width) best = std::min(best, M[y - 1][c]);
            }
            M[y][x] = e.at<int>(y, x) + best;
        }
    return M;
}

// Smallest (value, column) pair among the given columns of one row.
static int argminPair(const std::vector<int64_t>& row, int from, int to) {
    int best = from;
    for (int c = from + 1; c <= to; ++c)
        if (std::make_pair(row[c], c) < std::make_pair(row[best], best)) best = c;
    return best;
}

static std::vector<int> seam(const std::vector<std::vector<int64_t>>& M, int width) {
    const int H = (int)M.size();
    std::vector<int> s(H);
    s[H - 1] = argminPair(M[H - 1], 0, width - 1);
    for (int y = H - 1; y > 0; --y)
        s[y - 1] = argminPair(M[y - 1], std::max(s[y] - 1, 0), std::min(s[y] + 1, width - 1));
    return s;
}

// New image with the seam pixel of every row left out (built by copying, not by shifting).
static cv::Mat removeSeam(const cv::Mat& img, int width, const std::vector<int>& s) {
    cv::Mat out(img.rows, width - 1, img.type());
    const size_t px = img.elemSize();
    for (int y = 0; y < img.rows; ++y)
        for (int x = 0, ox = 0; x < width; ++x) {
            if (x == s[y]) continue;
            std::memcpy(out.ptr(y, ox), img.ptr(y, x), px);
            ++ox;
        }
    return out;
}

// True if the int32 matrix m (first 'width' columns) equals the int64 reference.
static bool equalsM(const cv::Mat& m, const std::vector<std::vector<int64_t>>& R, int width) {
    for (int y = 0; y < m.rows; ++y)
        for (int x = 0; x < width; ++x)
            if ((int64_t)m.at<int>(y, x) != R[y][x]) return false;
    return true;
}

}  // namespace ref

// =============================================================================================
// Lockstep run: seq, omp and the reference process the same image seam by seam; after every stage
// all three must agree exactly. Also checks seam validity (connectivity) for every seam.
// withRef = false skips the (slow) reference for long runs.
// =============================================================================================
static void lockstep(const cv::Mat& color, int seams, const ParallelConfig& cfg,
                     const std::string& tag, bool withRef) {
    cv::Mat imgS = color.clone(), imgO = color.clone();
    cv::Mat grayS, grayO;
    cv::cvtColor(imgS, grayS, cv::COLOR_BGR2GRAY);
    grayO = grayS.clone();
    cv::Mat grayR = grayS.clone(), imgR = color.clone();  // reference state (compact matrices)
    const int H = color.rows, W = color.cols;
    cv::Mat eS(H, W, CV_32SC1), eO(H, W, CV_32SC1), mS(H, W, CV_32SC1), mO(H, W, CV_32SC1);
    std::vector<int> seamS, seamO;

    for (int width = W, i = 0; i < seams && width > 1; ++i, --width) {
        const std::string where = tag + ", seam " + std::to_string(i);

        seq::computeEnergy(grayS, width, eS);
        omp_impl::computeEnergy(grayO, width, eO, cfg);
        bool ok = sameCols(eS, eO, width);
        check(ok, "energy seq!=omp: " + where);

        seq::computeCumulative(eS, width, mS);
        omp_impl::computeCumulative(eS, width, mO, cfg);
        const bool okM = sameCols(mS, mO, width);
        check(okM, "M seq!=omp: " + where);
        ok = ok && okM;

        const int endS = seq::findMinEndpoint(mS, width);
        const int endO = omp_impl::findMinEndpoint(mS, width, cfg);
        check(endS == endO, "endpoint seq!=omp: " + where);
        ok = ok && endS == endO;

        seq::backtrackSeam(mS, width, endS, seamS);
        omp_impl::backtrackSeam(mS, width, endS, seamO);
        check(seamS == seamO, "seam seq!=omp: " + where);
        check(validSeam(seamS, H, width), "seam not connected / out of range: " + where);
        ok = ok && seamS == seamO;

        if (withRef) {
            const cv::Mat eR = ref::energy(grayR, width);
            const bool okRE = sameCols(eS, eR, width);
            check(okRE, "energy seq!=reference: " + where);
            const auto MR = ref::cumulative(eR, width);
            const bool okRM = ref::equalsM(mS, MR, width);
            check(okRM, "M seq!=reference: " + where);
            const std::vector<int> seamR = ref::seam(MR, width);
            check(seamS == seamR, "seam seq!=reference: " + where);
            ok = ok && okRE && okRM && seamS == seamR;
        }

        seq::removeSeam(imgS, grayS, width, seamS);
        omp_impl::removeSeam(imgO, grayO, width, seamS, cfg);
        bool okR = sameCols(imgS, imgO, width - 1) && sameCols(grayS, grayO, width - 1);
        check(okR, "image after removal seq!=omp: " + where);
        if (withRef) {
            imgR = ref::removeSeam(imgR, width, seamS);
            grayR = ref::removeSeam(grayR, width, seamS);
            const bool okRR = sameCols(imgS, imgR, width - 1) && sameCols(grayS, grayR, width - 1);
            check(okRR, "image after removal seq!=reference: " + where);
            okR = okR && okRR;
        }
        if (!(ok && okR)) return;  // states diverged; later seams would only repeat the error
    }
}

// =============================================================================================
// Test 1: edge-case images for the OpenMP carve() (requirements 6, 8, 9).
// Shapes include images narrower / shorter than the thread count, so some threads get no work.
// =============================================================================================
static void testEdgeCaseCarve() {
    struct Shape { int rows, cols; };
    const std::vector<Shape> shapes = {{1, 1}, {1, 8}, {8, 1}, {9, 2}, {9, 3}, {2, 9},
                                       {3, 9}, {5, 5}, {17, 13}};
    const std::vector<int> threads = g_quick ? std::vector<int>{3, 16} : std::vector<int>{1, 2, 3, 5, 8, 16};
    const std::vector<Schedule> schedules =
        g_quick ? std::vector<Schedule>{kStatic1} : std::vector<Schedule>{kStatic, kStatic1, kDynamic};

    unsigned seed = 100;
    for (const Shape& sh : shapes) {
        const cv::Mat img = randomColor(sh.rows, sh.cols, ++seed);
        // Targets: unchanged, one column, one row, one pixel, about half.
        const std::vector<std::pair<int, int>> targets = {
            {sh.cols, sh.rows}, {1, sh.rows}, {sh.cols, 1}, {1, 1},
            {std::max(1, sh.cols / 2), std::max(1, sh.rows / 2)}};
        for (const auto& tg : targets) {
            const cv::Mat expected = seq::carve(img, tg.first, tg.second, nullptr);
            check(expected.cols == tg.first && expected.rows == tg.second,
                  "seq dimensions: " + sz(img) + " -> " + sz(expected));
            for (const Variant& v : kVariants)
                for (int t : threads)
                    for (const Schedule& s : schedules) {
                        useSchedule(s);
                        ParallelConfig cfg = v.cfg;
                        cfg.threads = t;
                        const cv::Mat got = omp_impl::carve(img, tg.first, tg.second, cfg, nullptr);
                        const std::string where = sz(img) + " -> " + std::to_string(tg.first) + "x" +
                                                  std::to_string(tg.second) + ", " + v.name + ", " +
                                                  std::to_string(t) + " threads, " + s.name;
                        check(got.cols == tg.first && got.rows == tg.second && got.type() == CV_8UC3,
                              "omp dimensions/type: " + where);
                        check(identical(got, expected), "omp carve != seq carve: " + where);
                    }
        }
    }
}

// =============================================================================================
// Test 2: seams forced into the first and the last column (requirements 4, 5, 9).
// Gray columns 0,0,100,200,... make column 0 the only zero-energy column (energy of column 0 is
// |I(1)-I(0)| = 0, every other column has energy >= 100). Mirrored for the last column.
// =============================================================================================
static cv::Mat rampGray(int rows, int cols, bool zeroAtRight) {
    cv::Mat g(rows, cols, CV_8UC1);
    for (int x = 0; x < cols; ++x) {
        const int k = zeroAtRight ? (cols - 1 - x) : x;  // distance from the "flat" border
        const int v = (k <= 1) ? 0 : std::min(255, (k - 1) * 100);  // 0, 0, 100, 200, 255, ...
        g.col(x).setTo(v);
    }
    return g;
}

static void testForcedBorderSeams() {
    for (bool right : {false, true}) {
        const int H = 6, W = 4;  // values 0,0,100,200 (or mirrored); 4 columns keep values <= 255
        const cv::Mat gray = rampGray(H, W, right);
        const cv::Mat color = grayToColor(gray);
        const std::string side = right ? "last column" : "first column";

        cv::Mat e(H, W, CV_32SC1), M(H, W, CV_32SC1);
        seq::computeEnergy(gray, W, e);
        seq::computeCumulative(e, W, M);
        std::vector<int> seam;
        seq::backtrackSeam(M, W, seq::findMinEndpoint(M, W), seam);
        const int expectedCol = right ? W - 1 : 0;
        check(seam == std::vector<int>(H, expectedCol), "forced seam in " + side);

        // Removal of that seam: seq, omp and reference must agree; with the seam in the last
        // column nothing has to be shifted (count = 0 branch of removeSeam).
        for (int t : {1, 3, 8}) {
            omp_set_num_threads(t);
            useSchedule(kStatic1);
            cv::Mat cS = color.clone(), gS = gray.clone(), cO = color.clone(), gO = gray.clone();
            seq::removeSeam(cS, gS, W, seam);
            omp_impl::removeSeam(cO, gO, W, seam, kAllA.cfg);
            const cv::Mat cR = ref::removeSeam(color, W, seam);
            check(sameCols(cS, cR, W - 1) && sameCols(cO, cR, W - 1) && sameCols(gO, gS, W - 1),
                  "removal of " + side + " seam, " + std::to_string(t) + " threads");
        }
        // Whole lockstep on the same image (all seams down to width 1).
        for (const Variant& v : {kAllA, kAllB}) {
            omp_set_num_threads(8);
            lockstep(color, W, v.cfg, "forced " + side + ", " + v.name, true);
        }
    }
    // Zig-zag seam touching both borders: 0,1,2,3,2,1 on a 6x4 image, removal only.
    const std::vector<int> zig = {0, 1, 2, 3, 2, 1};
    const cv::Mat color = randomColor(6, 4, 77);
    cv::Mat gray;
    cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY);
    cv::Mat cS = color.clone(), gS = gray.clone(), cO = color.clone(), gO = gray.clone();
    seq::removeSeam(cS, gS, 4, zig);
    omp_set_num_threads(4);
    omp_impl::removeSeam(cO, gO, 4, zig, kAllA.cfg);
    const cv::Mat cR = ref::removeSeam(color, 4, zig);
    check(sameCols(cS, cR, 3) && sameCols(cO, cR, 3), "removal of zig-zag seam");
}

// =============================================================================================
// Test 3: maximum contrast and simple patterns against the reference (requirements 1, 2).
// A checkerboard of 2x2 blocks (0 / 255) gives energy 255 + 255 = 510, the largest possible value.
// =============================================================================================
static void testPatternsAgainstReference() {
    const int H = 24, W = 30;
    struct Pattern { const char* name; std::function<int(int, int)> f; };
    const std::vector<Pattern> patterns = {
        {"checkerboard 2x2 blocks", [](int x, int y) { return ((x / 2 + y / 2) % 2) ? 255 : 0; }},
        {"checkerboard 1x1",        [](int x, int y) { return ((x + y) % 2) ? 255 : 0; }},
        {"vertical stripes",        [](int x, int)   { return (x / 3 % 2) ? 255 : 0; }},
        {"horizontal stripes",      [](int, int y)   { return (y / 2 % 2) ? 255 : 0; }},
        {"horizontal gradient",     [](int x, int)   { return x * 8; }},
        {"diagonal gradient",       [](int x, int y) { return (x + y) * 4; }},
        {"constant 255",            [](int, int)     { return 255; }},
    };
    for (const Pattern& p : patterns) {
        cv::Mat gray(H, W, CV_8UC1);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) gray.at<uchar>(y, x) = (uchar)std::min(255, p.f(x, y));

        cv::Mat e(H, W, CV_32SC1);
        seq::computeEnergy(gray, W, e);
        check(identical(e, ref::energy(gray, W)), std::string("energy seq!=reference: ") + p.name);
        double minE, maxE;
        cv::minMaxLoc(e, &minE, &maxE);
        check(minE >= 0 && maxE <= 510, std::string("energy outside [0, 510]: ") + p.name);
        if (std::string(p.name) == "checkerboard 2x2 blocks")
            check(maxE == 510, "checkerboard 2x2 reaches the maximum energy 510");

        omp_set_num_threads(g_quick ? 3 : 8);
        useSchedule(kStatic1);
        for (const Variant& v : {kAllA, kAllB})
            lockstep(grayToColor(gray), g_quick ? 5 : 15, v.cfg, std::string(p.name) + ", " + v.name, true);
    }
}

// =============================================================================================
// Test 4: DP optimality by brute force on non-square shapes (requirement 2).
// Every possible connected seam is enumerated; the DP minimum must equal the best one.
// =============================================================================================
static void testBruteForceShapes() {
    struct Shape { int rows, cols; };
    const std::vector<Shape> shapes = {{7, 1}, {7, 2}, {6, 3}, {2, 6}, {1, 6}, {4, 7}, {8, 4}};
    const unsigned seeds = g_quick ? 3 : 10;
    for (const Shape& sh : shapes)
        for (unsigned seed = 1; seed <= seeds; ++seed) {
            const int H = sh.rows, W = sh.cols;
            std::mt19937 gen(seed * 31 + W * 7 + H);
            cv::Mat gray(H, W, CV_8UC1);
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x) gray.at<uchar>(y, x) = (uchar)(gen() % 256);
            cv::Mat e(H, W, CV_32SC1), M(H, W, CV_32SC1);
            seq::computeEnergy(gray, W, e);
            seq::computeCumulative(e, W, M);

            int best = INT_MAX;
            std::vector<int> path(H);
            std::function<void(int, int)> walk = [&](int y, int cost) {
                if (y == H) { best = std::min(best, cost); return; }
                for (int x = 0; x < W; ++x) {
                    if (y > 0 && std::abs(x - path[y - 1]) > 1) continue;
                    path[y] = x;
                    walk(y + 1, cost + e.at<int>(y, x));
                }
            };
            walk(0, 0);

            const std::string where = std::to_string(W) + "x" + std::to_string(H) + " seed " +
                                      std::to_string(seed);
            for (int t : {1, 3, 8}) {
                omp_set_num_threads(t);
                for (const Variant& v : {kAllA, kAllB}) {
                    cv::Mat MO(H, W, CV_32SC1);
                    omp_impl::computeCumulative(e, W, MO, v.cfg);
                    const int end = omp_impl::findMinEndpoint(MO, W, v.cfg);
                    std::vector<int> seam;
                    omp_impl::backtrackSeam(MO, W, end, seam);
                    int cost = 0;
                    for (int y = 0; y < H; ++y) cost += e.at<int>(y, seam[y]);
                    check(MO.at<int>(H - 1, end) == best && cost == best && validSeam(seam, H, W),
                          "brute force: " + where + ", " + std::to_string(t) + " threads, " + v.name);
                }
            }
        }
}

// =============================================================================================
// Test 5: connectivity of EVERY seam in a long run on a real image, seq vs omp (requirements 4, 7).
// =============================================================================================
static void testLongRunConnectivity() {
    const cv::Mat img = loadTestImage("building_windows.png");  // 533x800: tall, straight lines
    check(!img.empty(), "load building_windows.png");
    if (img.empty()) return;
    const int seams = g_quick ? 10 : 100;
    const std::vector<int> threads = g_quick ? std::vector<int>{4} : std::vector<int>{4, 8};
    useSchedule(kStatic);
    for (int t : threads)
        for (const Variant& v : {kAllA, kAllB}) {
            omp_set_num_threads(t);
            lockstep(img, seams, v.cfg,
                     "building_windows, " + std::string(v.name) + ", " + std::to_string(t) + " threads",
                     /*withRef=*/false);
        }
    // The first few seams also against the reference (slow: the reference uses at<>()).
    omp_set_num_threads(4);
    lockstep(img, 3, kAllB.cfg, "building_windows, reference", true);
}

// =============================================================================================
// Test 6: large images 1920x1080 and 3840x2160 (requirement 10).
// Deterministic random content; a few seams per configuration (the parallel DP is slow on this
// platform, see WORKLOG M2), the first seam also against the reference implementation.
// =============================================================================================
static void testLargeImages() {
    struct Size { int w, h; };
    const std::vector<Size> sizes = g_quick ? std::vector<Size>{{1920, 1080}}
                                            : std::vector<Size>{{1920, 1080}, {3840, 2160}};
    for (const Size& s : sizes) {
        const cv::Mat img = randomColor(s.h, s.w, 2024u + s.w);
        const std::string name = std::to_string(s.w) + "x" + std::to_string(s.h);
        useSchedule(kStatic);
        omp_set_num_threads(8);
        lockstep(img, 1, kAllB.cfg, name + ", all(B), 8 threads, with reference", true);
        for (const Variant& v : kVariants) {
            if (std::string(v.name) == "none") continue;
            lockstep(img, 2, v.cfg, name + ", " + v.name + ", 8 threads", false);
        }
        if (!g_quick) {
            omp_set_num_threads(3);
            lockstep(img, 2, kAllA.cfg, name + ", all(A), 3 threads", false);
        }
    }
}

// =============================================================================================
// Test 7: odd thread counts, oversubscription (16 threads on 8 logical processors) and
// more threads than columns or rows (requirement 8).
// =============================================================================================
static void testThreadCounts() {
    struct Input { const char* name; cv::Mat img; };
    const std::vector<Input> inputs = {
        {"random 61x47", randomColor(47, 61, 7)},
        {"tall 9x120 (fewer columns than threads)", randomColor(120, 9, 8)},
        {"wide 130x5 (fewer rows than threads)", randomColor(5, 130, 9)},
    };
    const std::vector<int> threads = g_quick ? std::vector<int>{3, 16} : std::vector<int>{3, 5, 7, 16};
    const std::vector<Schedule> schedules =
        g_quick ? std::vector<Schedule>{kDynamic} : std::vector<Schedule>{kStatic, kStatic1, kDynamic, kGuided};
    const int repeats = g_quick ? 1 : 2;
    for (const Input& in : inputs)
        for (const Variant& v : kVariants)
            for (int t : threads)
                for (const Schedule& s : schedules) {
                    omp_set_num_threads(t);
                    useSchedule(s);
                    for (int r = 0; r < repeats; ++r)
                        lockstep(in.img, 12, v.cfg,
                                 std::string(in.name) + ", " + v.name + ", " + std::to_string(t) +
                                     " threads, " + s.name,
                                 /*withRef=*/r == 0 && t == threads[0]);
                }
}

// =============================================================================================
// Test 8: repeated whole runs (races show up only sometimes, so the same run is repeated).
// =============================================================================================
static void testRepeatedRuns() {
    const cv::Mat full = loadTestImage("person_desert.png");
    check(!full.empty(), "load person_desert.png");
    if (full.empty()) return;
    const cv::Mat img = full(cv::Rect(250, 150, 160, 120)).clone();
    const cv::Mat expected = seq::carve(img, 140, 110, nullptr);
    const int runs = g_quick ? 5 : 50;
    for (const Variant& v : {kAllA, kAllB})
        for (const Schedule& s : {kStatic, kDynamic}) {
            useSchedule(s);
            int bad = 0;
            for (int r = 0; r < runs; ++r) {
                ParallelConfig cfg = v.cfg;
                cfg.threads = 8;
                if (!identical(omp_impl::carve(img, 140, 110, cfg, nullptr), expected)) ++bad;
            }
            check(bad == 0, std::string("repeated runs ") + v.name + ", " + s.name + ": " +
                                std::to_string(bad) + "/" + std::to_string(runs) + " differ");
        }
}

// =============================================================================================
// Test 9 (full mode only): the real benchmark task "remove 20 % of the width" on every test image,
// plus 10 % of the height, omp vs seq.
// =============================================================================================
static void testRealImages20Percent() {
    if (g_quick) return;
    const char* names[] = {"boat_sea.png", "tree_field.png", "person_desert.png",
                           "building_windows.png", "face_portrait.png"};
    useSchedule(kStatic);
    for (const char* name : names) {
        const cv::Mat img = loadTestImage(name);
        check(!img.empty(), std::string("load ") + name);
        if (img.empty()) continue;
        const int tw = img.cols - img.cols / 5, th = img.rows - img.rows / 10;
        const cv::Mat expected = seq::carve(img, tw, th, nullptr);
        ParallelConfig cfg = kAllB.cfg;
        cfg.threads = 4;
        const cv::Mat got = omp_impl::carve(img, tw, th, cfg, nullptr);
        check(got.cols == tw && got.rows == th, std::string("dimensions: ") + name);
        check(identical(got, expected), std::string("20 % width + 10 % height, all(B), 4 threads: ") + name);
    }
}

// =============================================================================================
int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    g_quick = (argc > 1 && std::string(argv[1]) == "--quick");
    std::printf("test_m3 (%s mode), OpenMP %d, processors %d\n", g_quick ? "quick" : "full", _OPENMP,
                omp_get_num_procs());

    struct Test { const char* name; void (*fn)(); };
    const Test tests[] = {
        {"1 edge-case images, omp carve vs seq carve", testEdgeCaseCarve},
        {"2 seams forced into first / last column", testForcedBorderSeams},
        {"3 patterns + maximum contrast vs reference", testPatternsAgainstReference},
        {"4 brute force on non-square shapes", testBruteForceShapes},
        {"5 connectivity of every seam, long run", testLongRunConnectivity},
        {"6 large images 1920x1080 / 3840x2160", testLargeImages},
        {"7 odd thread counts, oversubscription, threads > columns/rows", testThreadCounts},
        {"8 repeated whole runs", testRepeatedRuns},
        {"9 real images, 20 % width + 10 % height", testRealImages20Percent},
    };
    for (const Test& t : tests) {
        const int beforeF = g_failures, beforeC = g_checks;
        const double t0 = omp_get_wtime();
        t.fn();
        std::printf("[%s] %s  (%d checks, %.1f s)\n", g_failures == beforeF ? "PASS" : "FAIL", t.name,
                    g_checks - beforeC, omp_get_wtime() - t0);
    }
    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
