// test_opt.cpp - correctness tests for the optimized OpenMP version (Milestone 5, seam_opt.h).
//
// Every optimization must leave the result bit-identical to the sequential baseline (seq::).
// The tests compare stage by stage, so that a wrong result points at the optimization that broke it:
//   1. energy kernels   (scalar / SIMD, sequential / parallel)            vs seq::computeEnergy
//   2. DP kernels       (scalar / SIMD, sequential / blocked with many K)  vs seq::computeCumulative,
//      and the O5 direction DP: every direction and the last row of M vs the sequential M
//   3. incremental energy: after EVERY seam removal the updated energy map equals a full
//      recomputation; random 8-connected seams reach every column (incl. the first and last)
//   4. whole carve()    for all optimization combinations, thread counts and thresholds vs seq::carve
//   5. invalid arguments throw like seq::carve
//
// Usage:  test_opt            full test set
//         test_opt --quick    reduced parameter sets
#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include <omp.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "seam_opt.h"
#include "seam_seq.h"

static int g_checks = 0;
static int g_failures = 0;
static bool g_quick = false;

static void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        if (g_failures <= 30) std::printf("  FAILED: %s\n", what.c_str());
    }
}

// True if columns 0..width-1 of a and b are identical (both CV_32SC1 or both CV_8U*).
static bool sameCols(const cv::Mat& a, const cv::Mat& b, int width) {
    if (a.rows != b.rows || a.type() != b.type()) return false;
    const size_t bytes = static_cast<size_t>(width) * a.elemSize();
    for (int y = 0; y < a.rows; ++y)
        if (std::memcmp(a.ptr(y), b.ptr(y), bytes) != 0) return false;
    return true;
}

static bool sameImage(const cv::Mat& a, const cv::Mat& b) {
    return a.size() == b.size() && a.type() == b.type() && sameCols(a, b, a.cols);
}

static cv::Mat randomGray(int w, int h, std::mt19937& rng, int levels = 256) {
    cv::Mat g(h, w, CV_8UC1);
    std::uniform_int_distribution<int> d(0, levels - 1);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) g.at<uchar>(y, x) = static_cast<uchar>(d(rng) * 255 / std::max(1, levels - 1));
    return g;
}

static cv::Mat randomColor(int w, int h, std::mt19937& rng) {
    cv::Mat c(h, w, CV_8UC3);
    cv::randu(c, 0, 256);
    (void)rng;
    return c;
}

// Random 8-connected vertical seam in [0, width): s[y+1] - s[y] in {-1, 0, 1}.
static std::vector<int> randomSeam(int width, int height, std::mt19937& rng) {
    std::vector<int> s(height);
    s[0] = std::uniform_int_distribution<int>(0, width - 1)(rng);
    // Sometimes force the seam into the first or last column (border cases of the update).
    const int mode = std::uniform_int_distribution<int>(0, 5)(rng);
    if (mode == 0) s[0] = 0;
    if (mode == 1) s[0] = width - 1;
    for (int y = 1; y < height; ++y) {
        int step = std::uniform_int_distribution<int>(-1, 1)(rng);
        if (mode == 0 && step != 0) step = -1;       // stick to the left border
        if (mode == 1 && step != 0) step = 1;        // stick to the right border
        s[y] = std::min(width - 1, std::max(0, s[y - 1] + step));
    }
    return s;
}

static const std::vector<std::pair<int, int>> kShapes = {
    {1, 1}, {1, 5}, {2, 1}, {2, 2}, {3, 3}, {4, 7}, {5, 2}, {7, 13}, {16, 16}, {33, 9},
    {64, 40}, {100, 3}, {129, 65}};

// ---------------------------------------------------------------------------------------------
static void testEnergyKernels() {
    std::printf("[1] energy kernels\n");
    std::mt19937 rng(11);
    for (auto [w, h] : kShapes) {
        for (int levels : {2, 256}) {
            cv::Mat gray = randomGray(w, h, rng, levels);
            for (int width = 1; width <= w; width += std::max(1, w / 4)) {
                cv::Mat ref(h, w, CV_32SC1, cv::Scalar(0));
                seq::computeEnergy(gray, width, ref);
                for (int type : {CV_32SC1, CV_16UC1})
                for (int simd = 0; simd <= 1; ++simd)
                    for (int threads : {1, 3, 8}) {
                        omp_set_num_threads(threads);
                        cv::Mat e(h, w, type, cv::Scalar(12345)), e32;
                        opt_impl::computeEnergy(gray, width, e, simd, threads > 1);
                        e.convertTo(e32, CV_32SC1);
                        check(sameCols(e32, ref, width), "energy " + std::to_string(w) + "x" +
                              std::to_string(h) + " width " + std::to_string(width) + " simd " +
                              std::to_string(simd) + " threads " + std::to_string(threads) +
                              (type == CV_16UC1 ? " 16-bit" : " 32-bit"));
                    }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
static void testDPKernels() {
    std::printf("[2] DP kernels (sequential and blocked)\n");
    std::mt19937 rng(22);
    const std::vector<int> blocks = g_quick ? std::vector<int>{0, 1, 2, 7, 64}
                                            : std::vector<int>{0, 1, 2, 3, 4, 5, 8, 16, 31, 64, 1000};
    const std::vector<int> threadList = g_quick ? std::vector<int>{1, 2, 5}
                                                : std::vector<int>{1, 2, 3, 4, 5, 7, 8, 16};
    std::vector<std::pair<int, int>> shapes = kShapes;
    shapes.push_back({300, 120});
    for (auto [w, h] : shapes) {
        cv::Mat energy(h, w, CV_32SC1);
        // Small value range -> many ties, large range -> no ties.
        const int maxV = std::uniform_int_distribution<int>(0, 1)(rng) ? 3 : 510;
        cv::randu(energy, 0, maxV + 1);
        for (int width : {w, std::max(1, w - 1), std::max(1, w / 2)}) {
            cv::Mat ref(h, w, CV_32SC1, cv::Scalar(0));
            seq::computeCumulative(energy, width, ref);
            for (int simd = 0; simd <= 1; ++simd)
                for (int K : blocks)
                    for (int threads : threadList) {
                        omp_set_num_threads(threads);
                        cv::Mat M(h, w, CV_32SC1, cv::Scalar(-7));
                        opt_impl::computeCumulative(energy, width, M, simd, K, threads > 1);
                        const std::string tag = std::to_string(w) + "x" + std::to_string(h) +
                              " width " + std::to_string(width) + " simd " + std::to_string(simd) +
                              " K " + std::to_string(K) + " threads " + std::to_string(threads);
                        check(sameCols(M, ref, width), "DP " + tag);

                        // O5: directions + last row, from the 16-bit copy of the energy.
                        cv::Mat e16, dir(h, w, CV_8SC1, cv::Scalar(99));
                        energy.convertTo(e16, CV_16UC1);
                        std::vector<int> lastRow;
                        opt_impl::computeDirections(e16, width, dir, lastRow, simd, K, threads > 1);
                        bool ok = std::memcmp(lastRow.data(), ref.ptr<int>(h - 1),
                                              sizeof(int) * width) == 0;
                        for (int y = 1; y < h && ok; ++y) {
                            const int* prev = ref.ptr<int>(y - 1);
                            for (int x = 0; x < width && ok; ++x) {
                                int best = -1;                     // rule of seq::backtrackSeam
                                for (int c = x - 1; c <= x + 1; ++c) {
                                    if (c < 0 || c >= width) continue;
                                    if (best == -1 || prev[c] < prev[best]) best = c;
                                }
                                ok = dir.at<int8_t>(y, x) == best - x;
                            }
                        }
                        check(ok, "directions " + tag);
                    }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Removes seams one by one with opt_impl::removeSeam (energy update on) and compares image, gray and
// energy with seq::removeSeam + a full seq::computeEnergy after every single seam.
static void incrementalRun(cv::Mat color, int seams, bool realSeams, int threads, std::mt19937& rng,
                           const std::string& label, int energyType = CV_32SC1) {
    omp_set_num_threads(threads);
    cv::Mat gray;
    cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY);
    cv::Mat refColor = color.clone(), refGray = gray.clone();
    const int h = color.rows;
    cv::Mat energy(h, color.cols, energyType), M(h, color.cols, CV_32SC1), refE(h, color.cols, CV_32SC1);
    cv::Mat e32(h, color.cols, CV_32SC1);
    seq::computeEnergy(gray, color.cols, e32);
    e32.convertTo(energy, energyType);
    std::vector<int> seam;
    bool ok = true;
    int width = color.cols;
    for (int k = 0; k < seams && width > 1 && ok; ++k) {
        if (realSeams) {
            energy.convertTo(e32, CV_32SC1);
            seq::computeCumulative(e32, width, M);
            seq::backtrackSeam(M, width, seq::findMinEndpoint(M, width), seam);
        } else {
            seam = randomSeam(width, h, rng);
        }
        opt_impl::removeSeam(color, gray, &energy, width, seam, threads > 1);
        seq::removeSeam(refColor, refGray, width, seam);
        --width;
        seq::computeEnergy(refGray, width, refE);
        energy.convertTo(e32, CV_32SC1);
        ok = sameCols(color, refColor, width) && sameCols(gray, refGray, width) &&
             sameCols(e32, refE, width);
        if (!ok) std::printf("    first difference after seam %d\n", k + 1);
    }
    check(ok, "incremental energy " + label + " threads " + std::to_string(threads) +
              (energyType == CV_16UC1 ? " 16-bit" : " 32-bit"));
}

static void testIncrementalEnergy() {
    std::printf("[3] incremental energy update\n");
    std::mt19937 rng(33);
    for (auto [w, h] : kShapes) {
        if (w < 2) continue;
        for (int threads : {1, 4}) {
            for (int type : {CV_32SC1, CV_16UC1}) {
                for (int rep = 0; rep < (g_quick ? 2 : 6); ++rep) {
                    incrementalRun(randomColor(w, h, rng), w - 1, false, threads, rng,
                                   "random seams " + std::to_string(w) + "x" + std::to_string(h), type);
                }
                incrementalRun(randomColor(w, h, rng), w - 1, true, threads, rng,
                               "real seams " + std::to_string(w) + "x" + std::to_string(h), type);
            }
        }
    }
    // Real photos (natural content), many seams.
    for (const char* name : {"boat_sea.png", "face_portrait.png"}) {
        cv::Mat img = cv::imread(std::string(TEST_DATA_DIR) + "/" + name, cv::IMREAD_COLOR);
        check(!img.empty(), std::string("load ") + name);
        if (img.empty()) continue;
        cv::Mat small;
        cv::resize(img, small, cv::Size(200, 150), 0, 0, cv::INTER_AREA);
        incrementalRun(small, g_quick ? 40 : 150, true, 4, rng, name);
        incrementalRun(small, g_quick ? 40 : 150, false, 3, rng, std::string(name) + " random");
        incrementalRun(small, g_quick ? 40 : 150, true, 4, rng, name, CV_16UC1);
    }
}

// ---------------------------------------------------------------------------------------------
static void testCarve() {
    std::printf("[4] whole carve() vs seq::carve\n");
    std::mt19937 rng(44);
    struct Case { cv::Mat img; int tw, th; std::string name; };
    std::vector<Case> cases;
    for (auto [w, h] : std::vector<std::pair<int, int>>{{1, 1}, {1, 9}, {9, 1}, {3, 3}, {17, 11},
                                                         {64, 48}, {120, 31}}) {
        cases.push_back({randomColor(w, h, rng), std::max(1, w / 2), std::max(1, h - h / 3),
                         std::to_string(w) + "x" + std::to_string(h)});
    }
    for (const char* name : {"boat_sea.png", "tree_field.png", "building_windows.png"}) {
        cv::Mat img = cv::imread(std::string(TEST_DATA_DIR) + "/" + name, cv::IMREAD_COLOR);
        if (img.empty()) { check(false, std::string("load ") + name); continue; }
        if (g_quick) cv::resize(img, img, cv::Size(240, 160), 0, 0, cv::INTER_AREA);
        cases.push_back({img, img.cols - img.cols / 6, img.rows - img.rows / 10, name});
    }

    const std::vector<int> threadList = g_quick ? std::vector<int>{1, 4} : std::vector<int>{1, 2, 3, 4, 8};
    for (const Case& c : cases) {
        const cv::Mat ref = seq::carve(c.img, c.tw, c.th, nullptr);
        for (int compact = 0; compact <= 1; ++compact)
        for (int incr = 0; incr <= 1; ++incr)
            for (int simd = 0; simd <= 1; ++simd)
                for (int K : {0, 1, 16})
                    for (long long thr : {0LL, 1000000000LL})
                        for (int threads : threadList) {
                            if (g_quick && thr != 0 && threads == 1) continue;
                            OptConfig cfg;
                            cfg.threads = threads;
                            cfg.incrementalEnergy = incr;
                            cfg.simdKernels = simd;
                            cfg.compactMemory = compact;
                            cfg.dpBlockRows = K;
                            cfg.parallelMinPixels = thr;
                            StageTimings t;
                            const cv::Mat out = opt_impl::carve(c.img, c.tw, c.th, cfg, &t);
                            check(sameImage(out, ref) && t.total > 0.0,
                                  "carve " + c.name + " " + opt_impl::describe(cfg) + " threads " +
                                  std::to_string(threads));
                        }
    }
}

// ---------------------------------------------------------------------------------------------
static bool throws(const std::function<void()>& f) {
    try { f(); } catch (const std::invalid_argument&) { return true; }
    return false;
}

static void testErrors() {
    std::printf("[5] invalid arguments\n");
    cv::Mat img(10, 12, CV_8UC3, cv::Scalar(1, 2, 3));
    OptConfig cfg;
    cfg.threads = 2;
    check(throws([&] { opt_impl::carve(img, 0, 10, cfg, nullptr); }), "target width 0");
    check(throws([&] { opt_impl::carve(img, 13, 10, cfg, nullptr); }), "target width too big");
    check(throws([&] { opt_impl::carve(img, 12, 11, cfg, nullptr); }), "target height too big");
    check(throws([&] { opt_impl::carve(cv::Mat(), 1, 1, cfg, nullptr); }), "empty input");
    check(throws([&] { opt_impl::carve(cv::Mat(5, 5, CV_8UC1), 1, 1, cfg, nullptr); }), "gray input");
    OptConfig bad = cfg;
    bad.threads = 0;
    check(throws([&] { opt_impl::carve(img, 5, 5, bad, nullptr); }), "0 threads");
    bad = cfg;
    bad.dpBlockRows = -1;
    check(throws([&] { opt_impl::carve(img, 5, 5, bad, nullptr); }), "negative block rows");
    check(!throws([&] { opt_impl::carve(img, 12, 10, cfg, nullptr); }), "no-op carve");
}

int main(int argc, char** argv) {
    g_quick = argc > 1 && std::string(argv[1]) == "--quick";
    std::printf("test_opt (%s)\n", g_quick ? "quick" : "full");
    testEnergyKernels();
    testDPKernels();
    testIncrementalEnergy();
    testCarve();
    testErrors();
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
