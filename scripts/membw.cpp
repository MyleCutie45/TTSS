// membw.cpp - memory bandwidth reference for docs/benchmark_report.md (M4, section 7).
//
// Not part of the seam carving program. It answers one question for the analysis: how many bytes per
// second can this machine move between the cores and the main memory (RAM)? Every stage of seam carving
// streams through arrays that are much larger than the L3 cache, so this number is the upper limit for
// the memory-bound stages.
//
// Method (similar to the well-known STREAM benchmark):
//   * two arrays of 8-byte integers, each 512 MB (much larger than the 8 MB L3 cache);
//   * read  : sum = a[0] + a[1] + ...        bytes counted: 1 array
//   * write : b[i] = i                        bytes counted: 1 array (the hardware may also read the cache
//                                             lines before writing them; that traffic is NOT counted)
//   * copy  : b[i] = a[i]                     bytes counted: 2 arrays (read + write, STREAM convention)
//   * every test with 1, 2, 4, 8 threads (#pragma omp parallel for, static schedule: each thread streams
//     one contiguous block), 5 repetitions, the BEST repetition is reported (STREAM convention: the best
//     run shows what the hardware can do; slower runs were disturbed by something else).
//
// Build and run (MSYS2 UCRT64):
//   g++ -std=c++17 -O2 -fopenmp scripts/membw.cpp -o build/membw.exe
//   build/membw.exe            (prints CSV: threads,test,GB_per_s)
#include <omp.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

double secondsSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace

int main(int argc, char** argv) {
    const long long mb = argc > 1 ? std::atoll(argv[1]) : 512;
    const long long n = mb * 1024 * 1024 / static_cast<long long>(sizeof(long long));
    const double bytes = static_cast<double>(n) * sizeof(long long);
    std::vector<long long> a(n), b(n);

    // Touch both arrays once in parallel, so the operating system has really allocated the pages
    // before the first measurement.
    #pragma omp parallel for schedule(static)
    for (long long i = 0; i < n; ++i) { a[i] = i & 7; b[i] = 0; }

    long long sink = 0;   // printed at the end, so the compiler cannot remove the read loop
    std::printf("threads,test,GB_per_s\n");
    for (int threads : {1, 2, 4, 8}) {
        omp_set_num_threads(threads);
        double best[3] = {0.0, 0.0, 0.0};
        for (int rep = 0; rep < 5; ++rep) {
            auto t0 = std::chrono::steady_clock::now();
            long long sum = 0;
            #pragma omp parallel for schedule(static) reduction(+ : sum)
            for (long long i = 0; i < n; ++i) sum += a[i];
            best[0] = std::max(best[0], bytes / secondsSince(t0) / 1e9);
            sink += sum;

            t0 = std::chrono::steady_clock::now();
            #pragma omp parallel for schedule(static)
            for (long long i = 0; i < n; ++i) b[i] = i;
            best[1] = std::max(best[1], bytes / secondsSince(t0) / 1e9);

            t0 = std::chrono::steady_clock::now();
            #pragma omp parallel for schedule(static)
            for (long long i = 0; i < n; ++i) b[i] = a[i];
            best[2] = std::max(best[2], 2 * bytes / secondsSince(t0) / 1e9);
            sink += b[n / 2];
        }
        std::printf("%d,read,%.1f\n%d,write,%.1f\n%d,copy,%.1f\n", threads, best[0], threads, best[1],
                    threads, best[2]);
    }
    std::fprintf(stderr, "array size %lld MB each, checksum %lld\n", mb, sink);
    return 0;
}
