cmake_minimum_required(VERSION 3.16)
project(SeamCarving LANGUAGES CXX)

# ---- Language standard -------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++17, not -std=gnu++17

# ---- Build type and flags ----------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
# CMake's default Release flags for GCC are "-O3 -DNDEBUG"; the project contract
# (docs/architecture.md, section 8) fixes -O2 so that all members and all
# benchmarks use the same optimization level.
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

# ---- Dependencies ------------------------------------------------------------
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
find_package(OpenMP REQUIRED)

# ---- Sequential library (NO OpenMP flags: proves it contains no OpenMP) -------
add_library(seam_seq STATIC src/seam_seq.cpp)
target_include_directories(seam_seq PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(seam_seq PUBLIC ${OpenCV_LIBS})

# ---- OpenMP library ----------------------------------------------------------
# PRIVATE: only seam_omp.cpp is compiled with -fopenmp; programs that use the
# library still get the OpenMP runtime (libgomp) at link time.
add_library(seam_omp STATIC src/seam_omp.cpp)
target_include_directories(seam_omp PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(seam_omp PUBLIC ${OpenCV_LIBS} PRIVATE OpenMP::OpenMP_CXX)

# ---- Optimized OpenMP library (M5) ---------------------------------------------
# Kept separate from seam_omp so that the basic M2 version stays unchanged for comparison.
add_library(seam_opt STATIC src/seam_opt.cpp)
target_include_directories(seam_opt PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(seam_opt PUBLIC ${OpenCV_LIBS} PRIVATE OpenMP::OpenMP_CXX)

# ---- Command-line program ----------------------------------------------------
add_executable(seam_carving src/main.cpp)
target_link_libraries(seam_carving PRIVATE seam_seq seam_omp seam_opt)

# ---- Tests -------------------------------------------------------------------
enable_testing()

add_executable(test_seq tests/test_seq.cpp)
target_link_libraries(test_seq PRIVATE seam_seq)
target_compile_definitions(test_seq PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
add_test(NAME test_seq COMMAND test_seq)

add_executable(test_equivalence tests/test_equivalence.cpp)
target_link_libraries(test_equivalence PRIVATE seam_seq seam_omp OpenMP::OpenMP_CXX)
target_compile_definitions(test_equivalence PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
add_test(NAME test_equivalence COMMAND test_equivalence)

# M3 tests (tests/test_m3.cpp). Two CTest entries: a reduced "--quick" run and the full run.
add_executable(test_m3 tests/test_m3.cpp)
target_link_libraries(test_m3 PRIVATE seam_seq seam_omp OpenMP::OpenMP_CXX)
target_compile_definitions(test_m3 PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
add_test(NAME test_m3_quick COMMAND test_m3 --quick)
add_test(NAME test_m3_full COMMAND test_m3)

# M5 tests (tests/test_opt.cpp): optimized version against the sequential baseline.
add_executable(test_opt tests/test_opt.cpp)
target_link_libraries(test_opt PRIVATE seam_seq seam_opt OpenMP::OpenMP_CXX)
target_compile_definitions(test_opt PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
add_test(NAME test_opt_quick COMMAND test_opt --quick)
add_test(NAME test_opt_full COMMAND test_opt)

# CLI argument checks: these commands must FAIL (non-zero exit code).
add_test(NAME cli_missing_args COMMAND seam_carving)
add_test(NAME cli_target_too_wide
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_fail.png --width 801)
add_test(NAME cli_target_zero
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_fail.png --width 0)
add_test(NAME cli_missing_file
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/does_not_exist.png
                              --output ${CMAKE_BINARY_DIR}/cli_fail.png --width 10)
add_test(NAME cli_bad_stage
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_fail.png --width 790 --mode omp --stages foo)
add_test(NAME cli_zero_threads
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_fail.png --width 790 --mode omp --threads 0)
set_tests_properties(cli_missing_args cli_target_too_wide cli_target_zero cli_missing_file
                     cli_bad_stage cli_zero_threads
                     PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_opt_run
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_ok_opt.png --width 780 --height 590
                              --mode opt --threads 4 --opt incr,simd --dp-block 8 --csv)
add_test(NAME cli_bad_opt
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_fail.png --width 790 --mode opt --opt foo)
set_tests_properties(cli_bad_opt PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_omp_run
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_ok_omp.png --width 780 --height 590
                              --mode omp --threads 4 --persistent-dp --csv)

# CLI success case: a small real run must succeed.
add_test(NAME cli_seq_run
         COMMAND seam_carving --input ${CMAKE_SOURCE_DIR}/tests/data/boat_sea.png
                              --output ${CMAKE_BINARY_DIR}/cli_ok.png --width 780 --height 590 --csv)

# Test sets (M3): "quick" for frequent runs (about a minute), "full" for the complete validation.
#   ctest --test-dir build -L quick
#   ctest --test-dir build -L full
set_tests_properties(test_seq test_m3_quick cli_missing_args cli_target_too_wide cli_target_zero
                     cli_missing_file cli_bad_stage cli_zero_threads cli_omp_run cli_seq_run
                     test_opt_quick cli_opt_run cli_bad_opt
                     PROPERTIES LABELS quick)
set_tests_properties(test_equivalence test_m3_full test_opt_full PROPERTIES LABELS full)
