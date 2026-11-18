"""Run the complete M4 benchmark plan (overnight variant, chosen 2026-11-19) and then the analysis.

Order and repetitions (time estimates from the pilot - planning only):
  1. content     1 warm-up + 5 measured rounds   (~10 min)  content check, 1920x1080
  2. stages      1 warm-up + 5 measured rounds   (~1 h)     one stage parallel at a time, 1920x1080, -20 %
  3. main_nodp   1 warm-up + 5 measured rounds   (~3 h)     main grid, DP sequential (energy+min+removal)
  4. main        0 warm-up + 3 measured rounds   (~3 h)     main grid, all stages parallel;
                                                            7680x4320 only at -10 % (see below)
  5. schedule    0 warm-up + 3 measured rounds   (~1.5-2 h) OMP_SCHEDULE study, 1920x1080, -20 %

Reductions compared with the full plan (reasons in docs/benchmark_report.md): the parallel DP costs about
50-120 us of synchronization per image row (pilot), so "main" with all stages would take ~16 h with
1+5 rounds. Every run is a fresh process and steps 1-3 keep the machine warm, so steps 4-5 skip the
warm-up round.

The runner is resumable: if the machine stops, start this script again - finished runs are skipped.
Log: results/run_all.log

Usage: python scripts/run_all_bench.py
"""
import os
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PY = sys.executable
LOG = ROOT / "results" / "run_all.log"

STEPS = [
    ["content", "--warmup", "1", "--rounds", "5"],
    ["stages", "--warmup", "1", "--rounds", "5"],
    ["main_nodp", "--warmup", "1", "--rounds", "5"],
    # all configurations except 7680x4320 at -20 % and -30 %
    ["main", "--warmup", "0", "--rounds", "3", "--only", r"^(?!person_desert_7680x4320\.png\|0\.[23]\|)"],
    ["schedule", "--warmup", "0", "--rounds", "3"],
]


def log(msg):
    LOG.parent.mkdir(exist_ok=True)
    line = f"{datetime.now().isoformat(timespec='seconds')} {msg}"
    with LOG.open("a", encoding="utf-8") as f:
        f.write(line + "\n")
    print(line, flush=True)


def main():
    env = os.environ.copy()
    msys = r"E:\msys64\ucrt64\bin"
    if sys.platform == "win32" and os.path.isdir(msys) and msys.lower() not in env["PATH"].lower():
        env["PATH"] = msys + os.pathsep + env["PATH"]   # OpenCV / libgomp DLLs for seam_carving.exe
    log("run_all_bench start")
    for step in STEPS:
        t0 = time.time()
        log(f"step {step[0]} start: {' '.join(step)}")
        with LOG.open("a", encoding="utf-8") as f:
            rc = subprocess.run([PY, str(ROOT / "scripts" / "run_bench.py"), *step, "--timeout", "3600"],
                                cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT).returncode
        log(f"step {step[0]} end: exit {rc}, {(time.time() - t0) / 60:.1f} min")
    rc = subprocess.run([PY, str(ROOT / "scripts" / "analyze_bench.py")], cwd=ROOT, env=env).returncode
    log(f"analysis exit {rc}")
    log("run_all_bench done")


if __name__ == "__main__":
    main()
