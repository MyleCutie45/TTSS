// test_equivalence.cpp - the OpenMP version must give EXACTLY the same results as the sequential one
// (M2; extended by M3). All data are integers, so every comparison is exact (no tolerance).
//
// Race conditions do not show up on every run, so every configuration is repeated several times,
// with several thread counts and schedules. (ThreadSanitizer is not available with MinGW.)
#include <cstdio>
#include <initializer_list>
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

// Records one check; prints at most 20 failure messages to keep the output readable.
static void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        if (g_failures <= 20) std::printf("  FAILED: %s\n", what.c_str());
    }
}

// ---------------------------------------------------------------------------------------------
// Test configurations
// ---------------------------------------------------------------------------------------------
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
    {"none",            makeCfg(false, false, false, false, false)},
    {"energy",          makeCfg(true,  false, false, false, false)},
    {"dp(A)",           makeCfg(false, true,  false, false, false)},
    {"dp(B)",           makeCfg(false, true,  true,  false, false)},
    {"min",             makeCfg(false, false, false, true,  false)},
    {"removal",         makeCfg(false, false, false, false, true)},
    {"all(A)",          makeCfg(true,  true,  false, true,  true)},
    {"all(B)",          makeCfg(true,  true,  true,  true,  true)},
};

static const std::vector<int> kThreads = {1, 2, 4, 8, 20};

struct Schedule {
    const char* name;
    omp_sched_t kind;
    int chunk;  // 0 = default chunk
};
static const std::vector<Schedule> kSchedules = {
    {"static",   omp_sched_static,  0},
    {"static:1", omp_sched_static,  1},  // columns dealt out one by one (round robin)
    {"dynamic",  omp_sched_dynamic, 0},
    {"guided",   omp_sched_guided,  0},
};

static std::string label(const Variant& v, int threads, const Schedule& s) {
    return std::string(v.name) + ", " + std::to_string(threads) + " threads, " + s.name;
}

// ---------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------
static bool sameCols(const cv::Mat& a, const cv::Mat& b, int width) {
    if (a.type() != b.type() || a.rows != b.rows) return false;
    cv::Mat diff;
    cv::compare(a.colRange(0, width), b.colRange(0, width), diff, cv::CMP_NE);
    return cv::countNonZero(diff.reshape(1)) == 0;
}

static bool identical(const cv::Mat& a, const cv::Mat& b) {
    return a.size() == b.size() && sameCols(a, b, a.cols);
}

static cv::Mat randomColor(int rows, int cols, unsigned seed) {
    std::mt19937 gen(seed);
    cv::Mat m(rows, cols, CV_8UC3);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
            m.at<cv::Vec3b>(y, x) = cv::Vec3b((uchar)(gen() % 256), (uchar)(gen() % 256),
                                              (uchar)(gen() % 256));
    return m;
}

static cv::Mat loadTestImage(const char* name) {
    return cv::imread(std::string(TEST_DATA_DIR) + "/" + name, cv::IMREAD_COLOR);
}

// ---------------------------------------------------------------------------------------------
// Test 1: lockstep. For every seam, give the same input to seq and omp and compare every stage.
// ---------------------------------------------------------------------------------------------
static void lockstep(const cv::Mat& color, int seams, const ParallelConfig& cfg, const std::string& tag) {
    cv::Mat imgS = color.clone(), imgO = color.clone();
    cv::Mat grayS, grayO;
    cv::cvtColor(imgS, grayS, cv::COLOR_BGR2GRAY);
    grayO = grayS.clone();
    const int H = color.rows, W = color.cols;
    cv::Mat eS(H, W, CV_32SC1), eO(H, W, CV_32SC1), mS(H, W, CV_32SC1), mO(H, W, CV_32SC1);
    std::vector<int> seamS, seamO;

    for (int width = W, i = 0; i < seams && width > 1; ++i, --width) {
        seq::computeEnergy(grayS, width, eS);
        omp_impl::computeEnergy(grayO, width, eO, cfg);
        const bool okE = sameCols(eS, eO, width);

        seq::computeCumulative(eS, width, mS);
        omp_impl::computeCumulative(eS, width, mO, cfg);   // same input -> isolates the DP stage
        const bool okM = sameCols(mS, mO, width);

        const int endS = seq::findMinEndpoint(mS, width);
        const int endO = omp_impl::findMinEndpoint(mS, width, cfg);

        seq::backtrackSeam(mS, width, endS, seamS);
        omp_impl::backtrackSeam(mS, width, endS, seamO);

        seq::removeSeam(imgS, grayS, width, seamS);
        omp_impl::removeSeam(imgO, grayO, width, seamS, cfg);
        const bool okR = sameCols(imgS, imgO, width - 1) && sameCols(grayS, grayO, width - 1);

        const std::string where = tag + ", seam " + std::to_string(i);
        check(okE, "energy differs: " + where);
        check(okM, "M differs: " + where);
        check(endS == endO, "endpoint differs: " + where);
        check(seamS == seamO, "seam differs: " + where);
        check(okR, "image after removal differs: " + where);
        if (!(okE && okM && endS == endO && seamS == seamO && okR)) return;  // states diverged
    }
}

static void testLockstep() {
    const cv::Mat boat = loadTestImage("boat_sea.png");
    check(!boat.empty(), "load boat_sea.png");
    if (boat.empty()) return;

    struct Input { const char* name; cv::Mat img; };
    const std::vector<Input> inputs = {
        {"random 47x61", randomColor(47, 61, 7)},
        {"uniform 30x40", cv::Mat(30, 40, CV_8UC3, cv::Scalar(9, 9, 9))},
        {"boat crop 160x200", boat(cv::Rect(300, 250, 200, 160)).clone()},
    };
    const int kSeams = 20, kRepeats = 3;
    for (const Input& in : inputs)
        for (const Variant& v : kVariants)
            for (int t : kThreads)
                for (const Schedule& s : kSchedules) {
                    omp_set_num_threads(t);
                    omp_set_schedule(s.kind, s.chunk);
                    for (int r = 0; r < kRepeats; ++r)
                        lockstep(in.img, kSeams, v.cfg, std::string(in.name) + ", " + label(v, t, s));
                }
}

// ---------------------------------------------------------------------------------------------
// Test 2: whole carve() on the real test images (width and height reduced).
// Only 12 + 6 seams per run: on this platform every OpenMP barrier costs 15-200 us (measured),
// so many seams x many configurations would take very long. Lockstep (test 1) covers more seams.
// ---------------------------------------------------------------------------------------------
static void testFullCarve() {
    const char* names[] = {"boat_sea.png", "tree_field.png", "person_desert.png",
                           "building_windows.png", "face_portrait.png"};
    const std::vector<Schedule> schedules = {kSchedules[0], kSchedules[2], kSchedules[3]};
    for (const char* name : names) {
        const cv::Mat img = loadTestImage(name);
        check(!img.empty(), std::string("load ") + name);
        if (img.empty()) continue;
        const int tw = img.cols - 12, th = img.rows - 6;
        const cv::Mat expected = seq::carve(img, tw, th, nullptr);

        for (int vi : {6, 7}) {  // all(A), all(B)
            for (int t : kThreads)
                for (const Schedule& s : schedules) {
                    omp_set_schedule(s.kind, s.chunk);
                    ParallelConfig cfg = kVariants[vi].cfg;
                    cfg.threads = t;
                    const cv::Mat got = omp_impl::carve(img, tw, th, cfg, nullptr);
                    check(identical(got, expected),
                          std::string("carve differs: ") + name + ", " + label(kVariants[vi], t, s));
                }
        }
    }
    // Single-stage variants on one image (8 threads, static).
    const cv::Mat img = loadTestImage("face_portrait.png");
    if (img.empty()) return;
    const cv::Mat expected = seq::carve(img, img.cols - 30, img.rows - 10, nullptr);
    omp_set_schedule(omp_sched_static, 0);
    for (const Variant& v : kVariants) {
        ParallelConfig cfg = v.cfg;
        cfg.threads = 8;
        check(identical(omp_impl::carve(img, img.cols - 30, img.rows - 10, cfg, nullptr), expected),
              std::string("carve differs: face_portrait, ") + v.name);
    }
}

// ---------------------------------------------------------------------------------------------
// Test 3: minimum endpoint with MANY ties (values 0..2), various widths, threads and schedules.
// ---------------------------------------------------------------------------------------------
static void testMinEndpointTies() {
    const int widths[] = {1, 2, 3, 7, 20, 21, 100, 1001};
    ParallelConfig cfg = makeCfg(false, false, false, true, false);
    for (int W : widths)
        for (unsigned seed = 1; seed <= 20; ++seed) {
            std::mt19937 gen(seed * 1000 + W);
            cv::Mat M(2, W, CV_32SC1, cv::Scalar(0));
            for (int x = 0; x < W; ++x) M.at<int>(1, x) = (int)(gen() % 3);
            if (seed == 1) M.row(1).setTo(5);  // all equal -> must be column 0
            const int expected = seq::findMinEndpoint(M, W);
            for (int t : kThreads)
                for (const Schedule& s : kSchedules) {
                    omp_set_num_threads(t);
                    omp_set_schedule(s.kind, s.chunk);
                    const int got = omp_impl::findMinEndpoint(M, W, cfg);
                    check(got == expected, "min endpoint: W=" + std::to_string(W) + " seed=" +
                                               std::to_string(seed) + ", " + std::to_string(t) +
                                               " threads, " + s.name + ": got " + std::to_string(got) +
                                               " expected " + std::to_string(expected));
                }
        }
}

// ---------------------------------------------------------------------------------------------
// Test 4: DP stress – many repetitions on a full-size energy map with 20 threads.
// A missing barrier between rows would show up here as a different M.
// ---------------------------------------------------------------------------------------------
static void testDpStress() {
    const cv::Mat img = loadTestImage("boat_sea.png");
    if (img.empty()) return;
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::Mat e(img.rows, img.cols, CV_32SC1), mS(img.rows, img.cols, CV_32SC1),
        mO(img.rows, img.cols, CV_32SC1);
    seq::computeEnergy(gray, img.cols, e);
    seq::computeCumulative(e, img.cols, mS);

    for (bool persistent : {false, true})
        for (const Schedule& s : {kSchedules[0], kSchedules[2]}) {
            omp_set_num_threads(20);
            omp_set_schedule(s.kind, s.chunk);
            ParallelConfig cfg = makeCfg(false, true, persistent, false, false);
            int bad = 0;
            for (int r = 0; r < 30; ++r) {
                mO.setTo(-1);  // stale values must not survive
                omp_impl::computeCumulative(e, img.cols, mO, cfg);
                if (!identical(mS, mO)) ++bad;
            }
            check(bad == 0, std::string("DP stress ") + (persistent ? "B" : "A") + ", " + s.name +
                                ": " + std::to_string(bad) + "/30 runs differ");
        }
}

// ---------------------------------------------------------------------------------------------
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);  // print progress immediately, even into a pipe/file
    std::printf("OpenMP %d, max threads %d, processors %d\n", _OPENMP, omp_get_max_threads(),
                omp_get_num_procs());
    struct Test { const char* name; void (*fn)(); };
    const Test tests[] = {
        {"lockstep: every stage, every seam (8 variants x 5 thread counts x 4 schedules x 3 runs)", testLockstep},
        {"full carve on 5 real images (all(A)/all(B) x 5 thread counts x 3 schedules) + single stages", testFullCarve},
        {"min endpoint with many ties", testMinEndpointTies},
        {"DP stress, 20 threads, 30 runs", testDpStress},
    };
    for (const Test& t : tests) {
        const int before = g_failures;
        const double t0 = omp_get_wtime();
        t.fn();
        std::printf("[%s] %s  (%.1f s)\n", g_failures == before ? "PASS" : "FAIL", t.name,
                    omp_get_wtime() - t0);
    }
    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
