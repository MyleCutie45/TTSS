#!/usr/bin/env bash
# run_m5_bench.sh - all M5 benchmark experiments (docs/optimization_report.md, section 3).
# Runs INSIDE the Linux container (docker/Dockerfile); the repository is mounted at /src.
#
#   docker build -t ttss-dev docker
#   docker run --rm -v "<repo>:/src" -v ttss-build:/build ttss-dev bash scripts/run_m5_bench.sh
#
# Builds a Release binary in /build/rel, runs the quick tests, then the experiments one after the
# other (never in parallel: they would disturb each other's timings). Resumable (run_bench.py skips
# rows that already exist).
set -eu
cd /src
# Git inside the container: the mounted repository belongs to another user, and the Windows working
# copy has CRLF line endings. Environment-only settings, no configuration file is changed.
export GIT_CONFIG_COUNT=2 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0=/src \
       GIT_CONFIG_KEY_1=core.autocrlf GIT_CONFIG_VALUE_1=true

cmake -S /src -B /build/rel -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build /build/rel >/dev/null
ctest --test-dir /build/rel -L quick 2>&1 | grep -E "tests passed|tests failed"

EXE=/build/rel/seam_carving
for exp in m5_ablation m5_block m5_threads m5_misc; do
    echo "=== $exp $(date -Is)"
    python3 scripts/run_bench.py "$exp" --exe "$EXE" --warmup 1 --rounds 5 >"results/$exp.stdout.txt"
done
echo "=== m5_threads_8k $(date -Is)"
python3 scripts/run_bench.py m5_threads_8k --exe "$EXE" --warmup 1 --rounds 3 >results/m5_threads_8k.stdout.txt
echo "=== done $(date -Is)"
