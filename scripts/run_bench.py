"""M4 benchmark runner (docs/benchmark_plan.md).

Runs build/seam_carving --csv for every configuration of an experiment and stores ONE CSV row per run
(raw data; medians are computed later by scripts/analyze_bench.py).

Methodology (docs/benchmark_plan.md, section 5):
  * Only carve() is timed (the program's t_* columns); image loading and saving are excluded.
  * Rounds: round 0 is a warm-up round (every configuration once, flagged warmup=1, not used for
    medians); rounds 1..N are measured. Inside every round the configurations run in a shuffled order
    (fixed seed), so slow drifts (heat, background load) spread over all configurations instead of
    hitting one of them.
  * Every output image is hashed (SHA-256). For each (image, target) the seq output is the reference;
    an omp run whose output differs is flagged hash_ok=0 (a fast but wrong result is worthless).
  * The machine description is stored next to the CSV (<name>.meta.json).
  * Resumable: rows already present in the CSV (same experiment/config/round) are skipped.

Usage:
  python scripts/run_bench.py <experiment> [--rounds N] [--warmup K] [--timeout S] [--out results/x.csv]
                              [--exe path/to/seam_carving]
  python scripts/run_bench.py --list
"""
import argparse
import csv
import hashlib
import json
import os
import platform
import random
import re
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "build" / ("seam_carving.exe" if sys.platform == "win32" else "seam_carving")
IMAGES = ROOT / "bench_images"
TMP = ROOT / "out" / "bench"          # output images (ignored by git)

CLI_COLUMNS = ["mode", "threads", "schedule", "stages", "in_w", "in_h", "out_w", "out_h", "seams",
               "t_energy", "t_dp", "t_min", "t_backtrack", "t_removal", "t_total"]
COLUMNS = (["experiment", "config", "image", "reduction", "round", "warmup", "omp_schedule_env",
            "oversubscribed"] + CLI_COLUMNS + ["output_sha256", "hash_ok", "exit", "wall_s", "timestamp"])

ALL = "energy,dp,min,removal"
MAIN_SIZES = ["512x512", "1920x1080", "3840x2160", "7680x4320"]
LOGICAL = os.cpu_count() or 1


def main_image(size):
    return f"person_desert_{size}.png"


def cfg(image, reduction, mode, threads=None, stages=ALL, persistent=False, schedule=None):
    """One benchmark configuration. reduction = fraction of the width removed (0.1 / 0.2 / 0.3)."""
    return {"image": image, "reduction": reduction, "mode": mode, "threads": threads,
            "stages": stages, "persistent": persistent, "schedule": schedule}


def config_id(c):
    if c["mode"] == "seq":
        return f"{c['image']}|{c['reduction']}|seq"
    return (f"{c['image']}|{c['reduction']}|omp|t{c['threads']}|{c['stages']}"
            f"{'|persistent' if c['persistent'] else ''}|{c['schedule'] or 'static(default)'}")


# ---------------------------------------------------------------------------------------------
# Experiments (docs/benchmark_plan.md, section 3). Sizes / thread counts are fixed here so that every
# result file states exactly what was run.
# ---------------------------------------------------------------------------------------------
def exp_pilot():
    """One run per size, smallest reduction (10 %), to estimate how long the real experiments take
    (run time grows roughly linearly with the number of seams, so 20 % / 30 % can be extrapolated
    for PLANNING only - such extrapolations are never reported as results)."""
    out = []
    for s in MAIN_SIZES:
        out += [cfg(main_image(s), 0.1, "seq"),
                cfg(main_image(s), 0.1, "omp", 4, ALL),
                cfg(main_image(s), 0.1, "omp", 4, "energy,min,removal")]
    return out


def exp_main(sizes=MAIN_SIZES, threads=(1, 2, 4, 8, 16), reductions=(0.1, 0.2, 0.3), stages=ALL):
    out = []
    for s in sizes:
        for r in reductions:
            out.append(cfg(main_image(s), r, "seq"))
            out += [cfg(main_image(s), r, "omp", t, stages) for t in threads]
    return out


def exp_stages(size="1920x1080", reduction=0.2, threads=(1, 2, 4, 8, 16)):
    """Which stage gains from parallelization? One stage at a time, plus none / all."""
    img = main_image(size)
    out = [cfg(img, reduction, "seq")]
    for t in threads:
        out += [cfg(img, reduction, "omp", t, "none"),
                cfg(img, reduction, "omp", t, "energy"),
                cfg(img, reduction, "omp", t, "dp"),
                cfg(img, reduction, "omp", t, "dp", persistent=True),
                cfg(img, reduction, "omp", t, "min"),
                cfg(img, reduction, "omp", t, "removal"),
                cfg(img, reduction, "omp", t, "energy,min,removal"),
                cfg(img, reduction, "omp", t, ALL),
                cfg(img, reduction, "omp", t, ALL, persistent=True)]
    return out


def exp_schedule(size="1920x1080", reduction=0.2, threads=(2, 4, 8)):
    img = main_image(size)
    out = [cfg(img, reduction, "seq")]
    for t in threads:
        for sch in ("static", "static,1", "dynamic", "dynamic,64", "guided"):
            out += [cfg(img, reduction, "omp", t, "energy,min,removal", schedule=sch),
                    cfg(img, reduction, "omp", t, ALL, schedule=sch)]
    return out


def exp_content(size="1920x1080", reduction=0.2):
    names = ["person_desert", "boat_sea", "tree_field", "building_windows", "face_portrait", "noise"]
    out = []
    for n in names:
        img = f"{n}_{size}.png"
        out += [cfg(img, reduction, "seq"), cfg(img, reduction, "omp", 4, "energy,min,removal")]
    return out


def exp_pilot_threads():
    """Planning only: cost of the parallel DP for other thread counts (incl. oversubscription)."""
    img = main_image("1920x1080")
    return ([cfg(img, 0.1, "omp", t, ALL) for t in (1, 2, 8, 16)] +
            [cfg(img, 0.1, "omp", t, ALL, persistent=True) for t in (4, 16)])


def exp_wsl():
    """Supplementary check, run inside WSL2 (Linux) on the same laptop with a Linux build (--exe).
    Same code, but a Linux OpenMP runtime (GCC libgomp for Linux). Question: is the high cost of the
    parallel DP measured on Windows a property of the algorithm or of the platform's OpenMP runtime?"""
    img = main_image("1920x1080")
    out = [cfg(img, 0.2, "seq")]
    for t in (1, 2, 4, 8):
        out += [cfg(img, 0.2, "omp", t, "dp"),
                cfg(img, 0.2, "omp", t, "dp", persistent=True),
                cfg(img, 0.2, "omp", t, "energy,min,removal"),
                cfg(img, 0.2, "omp", t, ALL)]
    for s in ("3840x2160", "7680x4320"):
        out.append(cfg(main_image(s), 0.1, "seq"))
        out += [cfg(main_image(s), 0.1, "omp", 4, st) for st in ("energy,min,removal", ALL)]
    return out


EXPERIMENTS = {
    "pilot": exp_pilot,
    "pilot_threads": exp_pilot_threads,
    "main": exp_main,
    "main_nodp": lambda: exp_main(stages="energy,min,removal"),
    "stages": exp_stages,
    "schedule": exp_schedule,
    "content": exp_content,
    "wsl": exp_wsl,
}


# ---------------------------------------------------------------------------------------------
def machine_info():
    def sh(cmd):
        try:
            return subprocess.run(cmd, capture_output=True, text=True, timeout=60, shell=isinstance(cmd, str)
                                  ).stdout.strip()
        except Exception as e:  # noqa: BLE001 - metadata only
            return f"unknown ({e})"

    def git(*args):  # None if git fails (e.g. "dubious ownership" inside WSL): never guess "clean"
        try:
            p = subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True, timeout=60)
            return p.stdout.strip() if p.returncode == 0 else None
        except Exception:  # noqa: BLE001 - metadata only
            return None
    commit = git("rev-parse", "--short", "HEAD")
    status = git("status", "--porcelain", "--untracked-files=no")
    info = {
        "date": datetime.now().isoformat(timespec="seconds"),
        "platform": platform.platform(),
        "logical_processors": LOGICAL,
        "python": sys.version.split()[0],
        "executable": str(EXE),
        "git_commit": commit or "unknown",
        "git_dirty": bool(status) if status is not None else "unknown",
        "env": {k: os.environ.get(k) for k in ("OMP_SCHEDULE", "OMP_PLACES", "OMP_PROC_BIND",
                                               "OMP_WAIT_POLICY", "OMP_DYNAMIC")},
    }
    build_dir = EXE.parent
    cache = build_dir / "CMakeCache.txt"
    if cache.exists():
        for line in cache.read_text(errors="replace").splitlines():
            if line.startswith(("CMAKE_BUILD_TYPE:", "OpenMP_CXX_FLAGS:", "CMAKE_CXX_COMPILER:")):
                k, v = line.split("=", 1)
                info[k.split(":")[0]] = v
    # The compiler that built the program (from the CMake cache), not whatever g++ is first on PATH.
    version = sh([info.get("CMAKE_CXX_COMPILER", "g++"), "--version"])
    info["compiler"] = version.splitlines()[0] if version else "unknown"
    # Effective compile flags. CMakeCache.txt is NOT used for them: CMakeLists.txt overrides the cached
    # CMAKE_CXX_FLAGS_RELEASE (CMake default "-O3 -DNDEBUG") with "-O2 -DNDEBUG" as a normal variable,
    # so only the generated build file shows what the compiler really received.
    ninja = build_dir / "build.ninja"
    if ninja.exists():
        info["compile_flags"] = sorted({ln.split("=", 1)[1].strip()
                                        for ln in ninja.read_text(errors="replace").splitlines()
                                        if ln.strip().startswith("FLAGS = ") and "-std=" in ln})
    if sys.platform.startswith("linux"):
        try:
            cpuinfo = Path("/proc/cpuinfo").read_text()
            info["cpu"] = re.search(r"model name\s*:\s*(.*)", cpuinfo).group(1).strip()
            kb = int(re.search(r"MemTotal:\s*(\d+)", Path("/proc/meminfo").read_text()).group(1))
            info["ram_gb"] = str(round(kb / 1024 ** 2, 1))
        except (OSError, AttributeError):
            pass
        info["kernel"] = platform.release()
        info["wsl"] = "microsoft" in platform.release().lower()
    if sys.platform == "win32":
        ps = ("$p=Get-CimInstance Win32_Processor; $c=Get-CimInstance Win32_ComputerSystem; "
              "$b=Get-CimInstance Win32_Battery; "
              "'{0}|{1}|{2}|{3}|{4}' -f $p.Name,$p.NumberOfCores,$p.MaxClockSpeed,"
              "[math]::Round($c.TotalPhysicalMemory/1GB,1),$b.BatteryStatus")
        parts = sh(["powershell", "-NoProfile", "-Command", ps]).split("|")
        if len(parts) == 5:
            info.update({"cpu": parts[0].strip(), "cores": parts[1], "base_mhz": parts[2], "ram_gb": parts[3],
                         "on_ac_power": parts[4].strip() == "2"})
        info["power_scheme"] = sh(["powercfg", "/getactivescheme"])
        # Windows 11 "Power mode" (Settings > Power & battery) is an overlay on top of the scheme.
        overlay = sh(["powershell", "-NoProfile", "-Command",
                      "(Get-ItemProperty 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes')"
                      ".ActiveOverlayAcPowerScheme"]).lower()
        info["power_mode_ac"] = {"ded574b5-45a0-4f42-8737-46345c09c238": "Best performance",
                                 "961cc777-2547-4f9d-8174-7d86181b8a7a": "Best power efficiency",
                                 "00000000-0000-0000-0000-000000000000": "Balanced",
                                 "": "Balanced"}.get(overlay, f"unknown ({overlay})")
    return info


def build_cmd(c, out_png):
    w, h = map(int, re.search(r"(\d+)x(\d+)", c["image"]).groups())
    target = w - round(w * c["reduction"])
    cmd = [str(EXE), "--input", str(IMAGES / c["image"]), "--output", str(out_png),
           "--width", str(target), "--csv"]
    if c["mode"] == "omp":
        cmd += ["--mode", "omp", "--threads", str(c["threads"]), "--stages", c["stages"]]
        if c["persistent"]:
            cmd.append("--persistent-dp")
    return cmd


def run_one(c, timeout):
    TMP.mkdir(parents=True, exist_ok=True)
    out_png = TMP / f"out_{os.getpid()}.png"
    if out_png.exists():
        out_png.unlink()
    env = os.environ.copy()
    env.pop("OMP_SCHEDULE", None)
    if c["schedule"]:
        env["OMP_SCHEDULE"] = c["schedule"]
    t0 = time.perf_counter()
    try:
        p = subprocess.run(build_cmd(c, out_png), capture_output=True, text=True, timeout=timeout, env=env)
        code, line = p.returncode, p.stdout.strip().splitlines()[-1] if p.stdout.strip() else ""
    except subprocess.TimeoutExpired:
        code, line = "timeout", ""
    wall = time.perf_counter() - t0
    values = dict(zip(CLI_COLUMNS, line.split(","))) if code == 0 else {}
    digest = hashlib.sha256(out_png.read_bytes()).hexdigest() if code == 0 and out_png.exists() else ""
    return code, values, digest, wall


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("experiment", nargs="?")
    ap.add_argument("--rounds", type=int, default=5, help="measured rounds (default 5)")
    ap.add_argument("--warmup", type=int, default=1, help="warm-up rounds (default 1)")
    ap.add_argument("--timeout", type=float, default=3600, help="seconds per run")
    ap.add_argument("--out", help="CSV path (default results/<experiment>.csv)")
    ap.add_argument("--seed", type=int, default=2026)
    ap.add_argument("--only", help="regex: run only configurations whose id matches")
    ap.add_argument("--exe", help="program to benchmark (default build/seam_carving[.exe])")
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()
    if a.exe:
        global EXE
        EXE = Path(a.exe).resolve()

    if a.list or not a.experiment:
        for k, f in EXPERIMENTS.items():
            print(f"{k:10s} {len(f())} configurations")
        return
    configs = EXPERIMENTS[a.experiment]()
    if a.only:
        configs = [c for c in configs if re.search(a.only, config_id(c))]
    out = Path(a.out) if a.out else ROOT / "results" / f"{a.experiment}.csv"
    out.parent.mkdir(parents=True, exist_ok=True)

    done, ref_hash = set(), {}
    if out.exists():
        with out.open(newline="") as f:
            for row in csv.DictReader(f):
                done.add((row["config"], row["round"]))
                if row["mode"] == "seq" and row["output_sha256"]:
                    ref_hash[(row["image"], row["reduction"])] = row["output_sha256"]
    meta_path = out.with_suffix(".meta.json")
    metas = json.loads(meta_path.read_text()) if meta_path.exists() else []
    metas.append({"experiment": a.experiment, "rounds": a.rounds, "warmup": a.warmup, "seed": a.seed,
                  "configurations": len(configs), **machine_info()})
    meta_path.write_text(json.dumps(metas, indent=2))

    rng = random.Random(a.seed)
    new_file = not out.exists()
    total = (a.warmup + a.rounds) * len(configs)
    n = 0
    with out.open("a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=COLUMNS)
        if new_file:
            w.writeheader()
        for rnd in range(a.warmup + a.rounds):
            order = configs[:]
            rng.shuffle(order)
            # seq first inside a round, so its reference hash exists before the omp runs are checked
            order.sort(key=lambda c: c["mode"] != "seq")
            for c in order:
                n += 1
                cid = config_id(c)
                if (cid, str(rnd)) in done:
                    continue
                code, v, digest, wall = run_one(c, a.timeout)
                key = (c["image"], str(c["reduction"]))
                if c["mode"] == "seq" and digest:
                    ref_hash.setdefault(key, digest)
                ok = "" if not digest or key not in ref_hash else int(digest == ref_hash[key])
                row = {"experiment": a.experiment, "config": cid, "image": c["image"],
                       "reduction": c["reduction"], "round": rnd, "warmup": int(rnd < a.warmup),
                       "omp_schedule_env": c["schedule"] or "",
                       "oversubscribed": int(c["mode"] == "omp" and c["threads"] > LOGICAL),
                       **v, "output_sha256": digest, "hash_ok": ok, "exit": code,
                       "wall_s": round(wall, 3), "timestamp": datetime.now().isoformat(timespec="seconds")}
                w.writerow(row)
                f.flush()
                print(f"[{n}/{total}] round {rnd} {cid}: exit {code} t_total {v.get('t_total', '-')} "
                      f"hash_ok {ok}", flush=True)


if __name__ == "__main__":
    main()
