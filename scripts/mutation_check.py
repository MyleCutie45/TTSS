"""Mutation check for M3: do the tests detect known bugs?

For every mutation, the script copies the project sources into build/mutants/<name>/ (the real
source files are never changed), inserts ONE known bug by exact text replacement, builds the test
programs there and runs them. A mutation is "detected" if at least one test program fails.

A test suite that passes on the correct code AND fails on every mutant is evidence that the tests
can find this kind of bug (for example a missing barrier = race condition).

Usage (from the repository root, with the MSYS2 UCRT64 tools on PATH):
    python scripts/mutation_check.py              # all mutations
    python scripts/mutation_check.py M1 M4        # selected mutations
Output: a Markdown table on stdout and build/mutants/results.json.
"""
import json
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "mutants"
EXE = ".exe" if sys.platform == "win32" else ""

# (id, description, [(file, old text, new text), ...], test programs to run)
# Every "old text" must occur exactly once in its file, otherwise the script stops.
MUTATIONS = [
    ("M1", "DP variant B: `nowait` removes the barrier between rows (race condition)",
     [("src/seam_omp.cpp",
       "#pragma omp for schedule(runtime)\n                for (int x = 0; x < width; ++x) {\n                    m[x]",
       "#pragma omp for schedule(runtime) nowait\n                for (int x = 0; x < width; ++x) {\n                    m[x]")],
     ["test_m3 --quick"]),
    ("M2", "Min endpoint: every thread writes slot 0 (lost update / race on shared slot)",
     [("src/seam_omp.cpp", "local[omp_get_thread_num()] = {bestValue, bestCol};",
       "local[0] = {bestValue, bestCol};")],
     ["test_m3 --quick"]),
    ("M3", "Min endpoint (omp): tie rule reversed (largest column wins)",
     [("src/seam_omp.cpp", "(value == bestValue && col < bestCol)", "(value == bestValue && col > bestCol)")],
     ["test_m3 --quick"]),
    ("M4", "Energy (omp): right neighbour clamped with gray.cols instead of the current width",
     [("src/seam_omp.cpp", "const int xr = std::min(x + 1, width - 1);",
       "const int xr = std::min(x + 1, gray.cols - 1);")],
     ["test_m3 --quick"]),
    ("M5", "Removal (omp): one pixel too few shifted per row (off-by-one)",
     [("src/seam_omp.cpp", "const int count = width - 1 - s;", "const int count = width - 2 - s;")],
     ["test_m3 --quick"]),
    ("M6", "DP (omp): reads the up-right neighbour also in the last column (stale column)",
     [("src/seam_omp.cpp", "if (x < width - 1) best = std::min(best, prev[x + 1]);",
       "if (x < width) best = std::min(best, prev[x + 1]);")],
     ["test_m3 --quick"]),
    ("M7", "Endpoint (seq): `<=` instead of `<` (largest column wins ties)",
     [("src/seam_seq.cpp", "if (last[x] < last[bestCol]) {", "if (last[x] <= last[bestCol]) {")],
     ["test_m3 --quick"]),
    ("M8", "SAME bug in seq AND omp: forward difference I(x+1)-I(x) instead of central difference",
     [("src/seam_seq.cpp", "const int xl = std::max(x - 1, 0);", "const int xl = x;"),
      ("src/seam_omp.cpp", "const int xl = std::max(x - 1, 0);", "const int xl = x;")],
     ["test_m3 --quick", "test_equivalence"]),
]

CHECKS_RE = re.compile(r"(\d+) checks, (\d+) failed")


def run(cmd, cwd, timeout):
    """Run a command; return (exit code or 'timeout', combined output)."""
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)
        return p.returncode, p.stdout + p.stderr
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or b"").decode(errors="replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
        return "timeout", out


def prepare(name):
    d = OUT / name
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    shutil.copy2(ROOT / "CMakeLists.txt", d / "CMakeLists.txt")
    for sub in ("src", "include", "tests"):
        shutil.copytree(ROOT / sub, d / sub)
    return d


def apply(d, edits):
    for rel, old, new in edits:
        f = d / rel
        text = f.read_text(encoding="utf-8")
        n = text.count(old)
        if n != 1:
            raise SystemExit(f"{rel}: expected exactly 1 occurrence, found {n}: {old!r}")
        f.write_text(text.replace(old, new), encoding="utf-8")


def main():
    selected = set(sys.argv[1:])
    results = []
    for mid, desc, edits, tests in MUTATIONS:
        if selected and mid not in selected:
            continue
        d = prepare(mid)
        apply(d, edits)
        b = d / "build"
        targets = sorted({t.split()[0] for t in tests})
        rc, out = run(["cmake", "-S", str(d), "-B", str(b), "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release"], d, 600)
        if rc == 0:
            rc, out = run(["cmake", "--build", str(b), "--target", *targets], d, 1200)
        entry = {"id": mid, "description": desc, "compiled": rc == 0, "tests": []}
        if rc != 0:
            entry["build_output_tail"] = out[-2000:]
        else:
            for t in tests:
                prog, *args = t.split()
                t0 = time.time()
                code, out = run([str(b / (prog + EXE)), *args], b, 1800)
                m = CHECKS_RE.findall(out)
                first_fail = next((ln.strip() for ln in out.splitlines() if "FAILED" in ln), "")
                entry["tests"].append({
                    "test": t, "exit": code, "seconds": round(time.time() - t0, 1),
                    "checks": int(m[-1][0]) if m else None, "failed": int(m[-1][1]) if m else None,
                    "first_failure": first_fail,
                })
        entry["detected"] = entry["compiled"] and any(x["exit"] != 0 for x in entry["tests"])
        results.append(entry)
        print(f"{mid}: detected={entry['detected']}", flush=True)

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")

    print("\n| ID | Mutation | Test | Exit | Failed / checks | Detected |")
    print("| -- | -------- | ---- | ---- | --------------- | -------- |")
    for r in results:
        if not r["compiled"]:
            print(f"| {r['id']} | {r['description']} | – | build failed | – | – |")
            continue
        for x in r["tests"]:
            fc = f"{x['failed']} / {x['checks']}" if x["checks"] is not None else "n/a"
            print(f"| {r['id']} | {r['description']} | `{x['test']}` | {x['exit']} | {fc} | "
                  f"{'yes' if x['exit'] != 0 else 'no'} |")


if __name__ == "__main__":
    main()
