#!/usr/bin/env bash
# run_sanitizers.sh - M3 tool-based checks for memory errors and data races (Linux / WSL only;
# MinGW GCC on Windows supports neither AddressSanitizer nor ThreadSanitizer).
#
#   asan   : g++ + libgomp, -fsanitize=address,undefined (out-of-bounds, use-after-free, leaks, UB)
#   tsan   : clang++ + LLVM libomp + Archer, -fsanitize=thread (data races). Archer is the OpenMP
#            tool that tells ThreadSanitizer about OpenMP barriers; without it TSan reports false races.
#   memcheck: g++ build without sanitizers, run under valgrind --tool=memcheck
#
# Usage (from the repository root inside WSL):
#   bash scripts/run_sanitizers.sh asan|tsan|tsan-quick|memcheck [source_dir] [build_root]
#   (tsan-quick runs only "test_m3 --quick"; used for the positive control on mutant M1:
#    bash scripts/run_sanitizers.sh tsan-quick build/mutants/M1)
# test_equivalence is not run under TSan: it is very slow there (> 40 min) and test_m3 covers the
# same stage variants, thread counts and schedules.
# Requires: g++, clang, libomp-dev, cmake, ninja-build, libopencv-dev, valgrind.
set -u
MODE=${1:?usage: run_sanitizers.sh asan|tsan|memcheck [source_dir] [build_root]}
SRC=$(realpath "${2:-.}")
ROOT=${3:-$HOME/ttss-sanitizers}
BUILD="$ROOT/$MODE-$(basename "$SRC")"
ARCHER=$(ls /usr/lib/llvm-*/lib/libarcher.so 2>/dev/null | head -1)

configure() {  # $1 = compiler, $2 = extra flags
    cmake -S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
          -DCMAKE_CXX_COMPILER="$1" -DCMAKE_CXX_FLAGS="$2" >/dev/null &&
    cmake --build "$BUILD" --target test_seq test_equivalence test_m3 >/dev/null
}

# $1 = label, rest = command; prints the summary line and the first sanitizer report line.
runtest() {
    local label=$1; shift
    local log="$BUILD/$label.log"
    local t0=$(date +%s)
    "$@" >"$log" 2>&1
    local rc=$?
    local summary=$(grep -E '[0-9]+ checks, [0-9]+ failed' "$log" | tail -1)
    local report=$(grep -m1 -E 'ERROR: (Address|Thread|Leak)Sanitizer|WARNING: ThreadSanitizer|runtime error:|ERROR SUMMARY' "$log")
    echo "$label | exit $rc | $(( $(date +%s) - t0 )) s | ${summary:-no summary} | ${report:-no sanitizer report}"
}

case "$MODE" in
  asan)
    configure g++ "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all" || exit 1
    export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=print_stacktrace=1
    runtest test_seq          "$BUILD/test_seq"
    runtest test_m3_full      "$BUILD/test_m3"
    runtest test_equivalence  "$BUILD/test_equivalence"
    ;;
  tsan|tsan-quick)
    [ -n "$ARCHER" ] || { echo "libarcher.so not found (install libomp-dev)"; exit 1; }
    configure clang++ "-fsanitize=thread -fno-omit-frame-pointer" || exit 1
    SYMB=$(ls /usr/lib/llvm-*/bin/llvm-symbolizer 2>/dev/null | head -1)
    export OMP_TOOL_LIBRARIES="$ARCHER" ARCHER_OPTIONS="verbose=1"
    # tests/tsan.supp: ignore reports that start inside uninstrumented libomp / libtbb (see file).
    # OPENCV_FOR_THREADS_NUM=1: OpenCV's own worker threads (TBB) are not started at all.
    export OPENCV_FOR_THREADS_NUM=1
    export TSAN_OPTIONS="halt_on_error=0:history_size=4:suppressions=$(realpath "$(dirname "$0")/../tests/tsan.supp"):external_symbolizer_path=$SYMB"
    runtest test_m3_quick     "$BUILD/test_m3" --quick
    if [ "$MODE" = tsan ]; then
        runtest test_m3_full  "$BUILD/test_m3"
    fi
    ;;
  memcheck)
    configure g++ "" || exit 1
    VG="valgrind --tool=memcheck --error-exitcode=99 --leak-check=full --errors-for-leak-kinds=definite"
    runtest test_seq          $VG "$BUILD/test_seq"
    runtest test_m3_quick     $VG "$BUILD/test_m3" --quick
    ;;
  *) echo "unknown mode $MODE"; exit 1 ;;
esac
echo "build: $BUILD  compiler: $(grep CMAKE_CXX_COMPILER: "$BUILD/CMakeCache.txt" | cut -d= -f2)"
