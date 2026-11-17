// test_seq.cpp - unit tests for the sequential implementation (M1). Extended by M3.
//
// We use our own CHECK macro instead of assert(): Release builds define NDEBUG, which turns
// assert() into nothing, so the tests would silently check nothing.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "seam_seq.h"

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failures;                                                             \
            std::printf("  FAILED: %s  (%s:%d)\n", #cond, __FILE__, __LINE__);        \
        }                                                                             \
    } while (0)

// ---------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------
static cv::Mat makeGray(const std::vector<std::vector<int>>& rows) {
    cv::Mat m((int)rows.size(), (int)rows[0].size(), CV_8UC1);
    for (int y = 0; y < m.rows; ++y)
        for (int x = 0; x < m.cols; ++x) m.at<uchar>(y, x) = (uchar)rows[y][x];
    return m;
}

static cv::Mat makeInt(const std::vector<std::vector<int>>& rows) {
    cv::Mat m((int)rows.size(), (int)rows[0].size(), CV_32SC1);
    for (int y = 0; y < m.rows; ++y)
        for (int x = 0; x < m.cols; ++x) m.at<int>(y, x) = rows[y][x];
    return m;
}

// True if the first 'width' columns of a and b are identical (same type, same rows).
static bool sameCols(const cv::Mat& a, const cv::Mat& b, int width) {
    if (a.type() != b.type() || a.rows != b.rows) return false;
    cv::Mat diff;
    cv::compare(a.colRange(0, width), b.colRange(0, width), diff, cv::CMP_NE);
    return cv::countNonZero(diff.reshape(1)) == 0;
}

static bool identical(const cv::Mat& a, const cv::Mat& b) {
    return a.size() == b.size() && sameCols(a, b, a.cols);
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

static int seamCost(const cv::Mat& energy, const std::vector<int>& seam) {
    int sum = 0;
    for (int y = 0; y < energy.rows; ++y) sum += energy.at<int>(y, seam[y]);
    return sum;
}

// Deterministic pseudo-random gray image (std::mt19937 output is fixed by the C++ standard).
static cv::Mat randomGray(int rows, int cols, unsigned seed) {
    std::mt19937 gen(seed);
    cv::Mat m(rows, cols, CV_8UC1);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x) m.at<uchar>(y, x) = (uchar)(gen() % 256);
    return m;
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

static bool throwsInvalidArgument(const std::function<void()>& f) {
    try { f(); } catch (const std::invalid_argument&) { return true; }
    return false;
}

// The 4x4 worked example of docs/algorithm.md, section 5.
static const std::vector<std::vector<int>> kExampleGray = {
    {10, 20, 30, 40}, {50, 10, 60, 20}, {30, 70, 10, 50}, {20, 40, 60, 10}};

// ---------------------------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------------------------
static void testEnergy4x4() {
    cv::Mat gray = makeGray(kExampleGray);
    cv::Mat energy(4, 4, CV_32SC1);
    seq::computeEnergy(gray, 4, energy);
    cv::Mat expected = makeInt({{50, 30, 50, 30}, {60, 60, 30, 50}, {70, 50, 20, 50}, {30, 70, 80, 90}});
    CHECK(identical(energy, expected));
}

static void testCumulative4x4() {
    cv::Mat energy = makeInt({{50, 30, 50, 30}, {60, 60, 30, 50}, {70, 50, 20, 50}, {30, 70, 80, 90}});
    cv::Mat M(4, 4, CV_32SC1);
    seq::computeCumulative(energy, 4, M);
    cv::Mat expected = makeInt({{50, 30, 50, 30}, {90, 90, 60, 80}, {160, 110, 80, 110}, {140, 150, 160, 170}});
    CHECK(identical(M, expected));
}

static void testSeam4x4() {
    cv::Mat M = makeInt({{50, 30, 50, 30}, {90, 90, 60, 80}, {160, 110, 80, 110}, {140, 150, 160, 170}});
    const int end = seq::findMinEndpoint(M, 4);
    CHECK(end == 0);
    std::vector<int> seam;
    seq::backtrackSeam(M, 4, end, seam);
    // Row 0 has a tie (columns 1 and 3, both 30): the smallest column must win.
    CHECK((seam == std::vector<int>{1, 2, 1, 0}));
}

static void testRemove4x4() {
    cv::Mat gray = makeGray(kExampleGray);
    cv::Mat color;
    cv::Mat channels[] = {gray, gray + 1, gray + 2};  // B = I, G = I+1, R = I+2 (distinct channels)
    cv::merge(channels, 3, color);
    seq::removeSeam(color, gray, 4, {1, 2, 1, 0});

    cv::Mat expected = makeGray({{10, 30, 40}, {50, 10, 20}, {30, 10, 50}, {40, 60, 10}});
    CHECK(sameCols(gray, expected, 3));
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 3; ++x) {
            const int v = expected.at<uchar>(y, x);
            CHECK((color.at<cv::Vec3b>(y, x) == cv::Vec3b((uchar)v, (uchar)(v + 1), (uchar)(v + 2))));
        }
}

static void testUniformAllTies() {
    cv::Mat gray(4, 4, CV_8UC1, cv::Scalar(7));
    cv::Mat energy(4, 4, CV_32SC1), M(4, 4, CV_32SC1);
    seq::computeEnergy(gray, 4, energy);
    seq::computeCumulative(energy, 4, M);
    CHECK(cv::countNonZero(energy) == 0);
    CHECK(cv::countNonZero(M) == 0);
    const int end = seq::findMinEndpoint(M, 4);
    CHECK(end == 0);
    std::vector<int> seam;
    seq::backtrackSeam(M, 4, end, seam);
    CHECK((seam == std::vector<int>{0, 0, 0, 0}));
}

static void testEndpointTie() {
    // Minimum 3 appears at columns 1 and 2 -> column 1.
    cv::Mat M = makeInt({{0, 0, 0, 0}, {5, 3, 3, 7}});
    CHECK(seq::findMinEndpoint(M, 4) == 1);
    // Only the first 'width' columns count: with width 1 the answer must be 0.
    CHECK(seq::findMinEndpoint(M, 1) == 0);
}

// Columns >= width must be ignored (catches the "mat.cols instead of width" bug).
static void testCurrentWidthIsRespected() {
    cv::Mat gray = randomGray(6, 9, 11);
    const int width = 6;
    cv::Mat garbage = gray.clone();
    garbage.colRange(width, 9).setTo(255);           // junk right of the valid area
    cv::Mat compact = gray.colRange(0, width).clone();

    cv::Mat e1(6, 9, CV_32SC1), e2(6, width, CV_32SC1);
    seq::computeEnergy(garbage, width, e1);
    seq::computeEnergy(compact, width, e2);
    CHECK(sameCols(e1, e2, width));

    cv::Mat m1(6, 9, CV_32SC1), m2(6, width, CV_32SC1);
    seq::computeCumulative(e1, width, m1);
    seq::computeCumulative(e2, width, m2);
    CHECK(sameCols(m1, m2, width));
}

// Brute force: try every possible seam of a small random image and compare with the DP result.
static void testDpIsOptimal() {
    for (unsigned seed = 1; seed <= 5; ++seed) {
        const int H = 5, W = 5;
        cv::Mat gray = randomGray(H, W, seed);
        cv::Mat energy(H, W, CV_32SC1), M(H, W, CV_32SC1);
        seq::computeEnergy(gray, W, energy);
        seq::computeCumulative(energy, W, M);
        std::vector<int> seam;
        seq::backtrackSeam(M, W, seq::findMinEndpoint(M, W), seam);

        int best = INT32_MAX;
        std::vector<int> path(H);
        std::function<void(int, int)> walk = [&](int y, int cost) {
            if (y == H) { best = std::min(best, cost); return; }
            for (int x = 0; x < W; ++x) {
                if (y > 0 && std::abs(x - path[y - 1]) > 1) continue;
                path[y] = x;
                walk(y + 1, cost + energy.at<int>(y, x));
            }
        };
        walk(0, 0);

        CHECK(validSeam(seam, H, W));
        CHECK(seamCost(energy, seam) == best);
        CHECK(M.at<int>(H - 1, seam[H - 1]) == best);
    }
}

// On a real image: the seam is connected and removal deletes exactly the seam pixel in each row.
static void testRealImageOneSeam() {
    cv::Mat color = cv::imread(std::string(TEST_DATA_DIR) + "/boat_sea.png", cv::IMREAD_COLOR);
    CHECK(!color.empty());
    if (color.empty()) return;
    cv::Mat gray;
    cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY);
    const int W = color.cols, H = color.rows;

    cv::Mat energy(H, W, CV_32SC1), M(H, W, CV_32SC1);
    seq::computeEnergy(gray, W, energy);
    seq::computeCumulative(energy, W, M);
    std::vector<int> seam;
    seq::backtrackSeam(M, W, seq::findMinEndpoint(M, W), seam);
    CHECK(validSeam(seam, H, W));
    CHECK(seamCost(energy, seam) == M.at<int>(H - 1, seam[H - 1]));

    cv::Mat before = color.clone();
    seq::removeSeam(color, gray, W, seam);
    bool rowsOk = true;
    for (int y = 0; y < H && rowsOk; ++y)
        for (int x = 0; x < W - 1; ++x) {
            const int src = (x < seam[y]) ? x : x + 1;  // pixels after the seam moved one left
            if (color.at<cv::Vec3b>(y, x) != before.at<cv::Vec3b>(y, src)) { rowsOk = false; break; }
        }
    CHECK(rowsOk);
}

static void testCarveDimensionsAndDeterminism() {
    cv::Mat img = cv::imread(std::string(TEST_DATA_DIR) + "/boat_sea.png", cv::IMREAD_COLOR);
    CHECK(!img.empty());
    if (img.empty()) return;
    StageTimings t;
    cv::Mat a = seq::carve(img, img.cols - 40, img.rows - 20, &t);
    CHECK(a.cols == img.cols - 40);
    CHECK(a.rows == img.rows - 20);
    CHECK(a.type() == CV_8UC3);
    CHECK(t.total > 0.0);
    CHECK(t.total >= t.energy + t.dp + t.minEndpoint + t.backtrack + t.removal);

    cv::Mat b = seq::carve(img, img.cols - 40, img.rows - 20, nullptr);
    CHECK(identical(a, b));  // two runs give exactly the same pixels
}

static void testEdgeCases() {
    cv::Mat img = randomColor(7, 10, 3);

    // Target == input: unchanged copy.
    cv::Mat same = seq::carve(img, 10, 7, nullptr);
    CHECK(identical(same, img));

    // Down to one column / one row.
    cv::Mat col = seq::carve(img, 1, 7, nullptr);
    CHECK(col.cols == 1 && col.rows == 7);
    cv::Mat row = seq::carve(img, 10, 1, nullptr);
    CHECK(row.cols == 10 && row.rows == 1);
    cv::Mat px = seq::carve(img, 1, 1, nullptr);
    CHECK(px.cols == 1 && px.rows == 1);

    // Image only 2 columns wide.
    cv::Mat narrow = randomColor(5, 2, 4);
    CHECK(seq::carve(narrow, 1, 5, nullptr).cols == 1);

    // Single-row image (energy has no vertical part) and 1x1 image.
    cv::Mat oneRow = randomColor(1, 8, 5);
    cv::Mat r1 = seq::carve(oneRow, 3, 1, nullptr);
    CHECK(r1.cols == 3 && r1.rows == 1);
    cv::Mat onePixel = randomColor(1, 1, 6);
    CHECK(identical(seq::carve(onePixel, 1, 1, nullptr), onePixel));

    // Invalid targets.
    CHECK(throwsInvalidArgument([&] { seq::carve(img, 0, 7, nullptr); }));
    CHECK(throwsInvalidArgument([&] { seq::carve(img, 11, 7, nullptr); }));
    CHECK(throwsInvalidArgument([&] { seq::carve(img, 10, 0, nullptr); }));
    CHECK(throwsInvalidArgument([&] { seq::carve(img, 10, 8, nullptr); }));
    CHECK(throwsInvalidArgument([&] { seq::carve(cv::Mat(), 1, 1, nullptr); }));
    CHECK(throwsInvalidArgument([&] { seq::carve(cv::Mat(4, 4, CV_8UC1), 2, 2, nullptr); }));
}

// Height reduction must equal: transpose, reduce width, transpose back.
static void testHeightUsesTranspose() {
    cv::Mat img = randomColor(12, 9, 8);
    cv::Mat viaHeight = seq::carve(img, 9, 8, nullptr);
    cv::Mat t, tCarved, expected;
    cv::transpose(img, t);
    tCarved = seq::carve(t, 8, 9, nullptr);
    cv::transpose(tCarved, expected);
    CHECK(identical(viaHeight, expected));
}

// ---------------------------------------------------------------------------------------------
int main() {
    struct Test { const char* name; void (*fn)(); };
    const Test tests[] = {
        {"energy 4x4 (docs example)", testEnergy4x4},
        {"cumulative M 4x4", testCumulative4x4},
        {"endpoint + backtrack 4x4 with tie", testSeam4x4},
        {"seam removal 4x4 (color + gray)", testRemove4x4},
        {"uniform image (all ties)", testUniformAllTies},
        {"endpoint tie -> smallest column", testEndpointTie},
        {"current width respected", testCurrentWidthIsRespected},
        {"DP optimal vs brute force", testDpIsOptimal},
        {"real image: seam + removal", testRealImageOneSeam},
        {"carve dimensions + determinism", testCarveDimensionsAndDeterminism},
        {"edge cases", testEdgeCases},
        {"height via transpose", testHeightUsesTranspose},
    };
    for (const Test& t : tests) {
        const int before = g_failures;
        t.fn();
        std::printf("[%s] %s\n", g_failures == before ? "PASS" : "FAIL", t.name);
    }
    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
