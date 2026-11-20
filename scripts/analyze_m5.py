"""Analysis of the M5 benchmark results (docs/optimization_report.md).

Reads results/m5_*.csv (raw rows from scripts/run_bench.py), drops warm-up rounds, checks that every run
succeeded with the sequential output (hash_ok), and writes
  results/m5_summary.md    tables (medians; every number in the report comes from here)
  docs/figures/m5_*.png    plots (each one also has a table in m5_summary.md)

Usage (inside the Linux container or any Python with matplotlib):
  python3 scripts/analyze_m5.py
"""
import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
RES = ROOT / "results"
FIG = ROOT / "docs" / "figures"
STAGES = ["t_energy", "t_dp", "t_min", "t_backtrack", "t_removal"]
STAGE_NAMES = {"t_energy": "energy", "t_dp": "DP", "t_min": "min", "t_backtrack": "backtrack",
               "t_removal": "removal (+ energy update)"}

# Default categorical palette, fixed order (validated: scripts of the dataviz guide, light surface).
C = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
INK, INK2, GRID, SURFACE = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": INK2, "axes.labelcolor": INK, "xtick.color": INK2, "ytick.color": INK2,
    "text.color": INK, "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8,
    "axes.spines.top": False, "axes.spines.right": False, "font.size": 10, "lines.linewidth": 2,
    "legend.frameon": False,
})


def load(name):
    path = RES / f"{name}.csv"
    if not path.exists():
        return None
    with path.open(newline="") as f:
        rows = list(csv.DictReader(f))
    return rows


def check(rows, name, out):
    measured = [r for r in rows if r["warmup"] == "0"]
    bad_exit = sum(r["exit"] != "0" for r in rows)
    bad_hash = sum(r["mode"] != "seq" and r["hash_ok"] != "1" for r in rows)
    out.append(f"- `{name}`: {len(rows)} runs ({len(measured)} measured, {len(rows) - len(measured)} warm-up), "
               f"exit != 0: {bad_exit}, output different from seq: {bad_hash}")
    return measured


def medians(rows):
    """config -> dict(median of every timing column, n, first row)"""
    groups = defaultdict(list)
    for r in rows:
        groups[r["config"]].append(r)
    res = {}
    for cid, rs in groups.items():
        d = {k: statistics.median(float(r[k]) for r in rs) for k in STAGES + ["t_total"]}
        d["n"] = len(rs)
        d["min"] = min(float(r["t_total"]) for r in rs)
        d["max"] = max(float(r["t_total"]) for r in rs)
        d["row"] = rs[0]
        res[cid] = d
    return res


def size_of(row):
    return row["image"].rsplit("_", 1)[1].removesuffix(".png")


def find(med, **want):
    """First configuration whose row matches all fields (mode, threads, stages-substring...)."""
    for cid, d in med.items():
        r = d["row"]
        ok = True
        for k, v in want.items():
            if k == "cid_has":
                ok &= all(part in cid for part in v)
            elif k == "size":
                ok &= size_of(r) == v
            else:
                ok &= r[k] == str(v)
        if ok:
            return d
    return None


def label(r):
    if r["mode"] == "seq":
        return "seq (original)"
    if r["mode"] == "omp":
        return f"basic omp: {r['stages']}"
    return f"opt: {r['stages']}"


def fmt(x):
    return f"{x:.3f}"


# ---------------------------------------------------------------------------------------------
def ablation(out):
    rows = load("m5_ablation")
    if rows is None:
        return
    out.append("\n## Ablation (`m5_ablation`): each optimization alone and step by step\n")
    rows = check(rows, "m5_ablation", out)
    med = medians(rows)
    for size in sorted({size_of(d["row"]) for d in med.values()}, key=lambda s: int(s.split("x")[0])):
        seq = find(med, size=size, mode="seq")
        out.append(f"\n### {size}, -20 % width, median of {seq['n']} runs (seconds)\n")
        out.append("| version | threads | t_total | speedup vs seq | energy | DP | min | backtrack | removal |")
        out.append("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
        items = sorted((d for d in med.values() if size_of(d["row"]) == size),
                       key=lambda d: (d["row"]["mode"] != "seq", d["row"]["mode"] != "omp",
                                      int(d["row"]["threads"]), -d["t_total"]))
        for d in items:
            r = d["row"]
            out.append(f"| {label(r)} | {r['threads']} | {fmt(d['t_total'])} | {seq['t_total'] / d['t_total']:.2f} | "
                       + " | ".join(fmt(d[k]) for k in STAGES) + " |")

        # Stacked stage bars for the step-by-step chain.
        chain = [("seq", find(med, size=size, mode="seq")),
                 ("basic omp\nDP seq, 8t", find(med, size=size, mode="omp", stages="energy+min+removal")),
                 ("opt none\n8t", find(med, size=size, mode="opt", threads=8, cid_has=["|none|K0"])),
                 ("+ simd\n8t", find(med, size=size, mode="opt", threads=8, cid_has=["|simd|K0"])),
                 ("+ incr\n8t", find(med, size=size, mode="opt", threads=8, cid_has=["|incr,simd|K0"])),
                 ("+ compact\n8t", find(med, size=size, mode="opt", threads=8, cid_has=["|incr,simd,compact|K0"])),
                 ("+ blocked DP\nK=64, 8t", find(med, size=size, mode="opt", threads=8,
                                                 cid_has=["|incr,simd,compact|K64"]))]
        chain = [(n, d) for n, d in chain if d]
        fig, ax = plt.subplots(figsize=(8.5, 4.2))
        bottom = [0.0] * len(chain)
        for i, k in enumerate(STAGES):
            vals = [d[k] for _, d in chain]
            ax.bar(range(len(chain)), vals, bottom=bottom, color=C[i], width=0.62,
                   edgecolor=SURFACE, linewidth=2, label=STAGE_NAMES[k])
            bottom = [b + v for b, v in zip(bottom, vals)]
        for i, (_, d) in enumerate(chain):
            ax.text(i, bottom[i] * 1.01 + 0.01 * max(bottom), f"{d['t_total']:.2f} s", ha="center",
                    va="bottom", fontsize=9, color=INK)
        ax.set_xticks(range(len(chain)), [n for n, _ in chain], fontsize=8.5)
        ax.set_ylabel("median time (s), stages stacked")
        ax.set_title(f"Optimizations added one by one, {size}, -20 % width", loc="left", fontsize=11)
        ax.legend(loc="upper right", fontsize=8.5)
        ax.grid(axis="x", visible=False)
        fig.tight_layout()
        fig.savefig(FIG / f"m5_ablation_{size}.png", dpi=150)
        plt.close(fig)


def block(out):
    rows = load("m5_block")
    if rows is None:
        return
    out.append("\n## DP block height K (`m5_block`, opt incr+simd+compact, 8 threads)\n")
    rows = check(rows, "m5_block", out)
    med = medians(rows)
    fig, ax = plt.subplots(figsize=(7, 4))
    for i, size in enumerate(sorted({size_of(d["row"]) for d in med.values()}, key=lambda s: int(s.split("x")[0]))):
        seq = find(med, size=size, mode="seq")
        out.append(f"\n### {size} (seq {fmt(seq['t_total'])} s)\n")
        out.append("| K | t_total | t_dp | speedup vs seq | t_dp min..max of runs (t_total) |")
        out.append("| ---: | ---: | ---: | ---: | --- |")
        pts = []
        for d in med.values():
            r = d["row"]
            if r["mode"] != "opt" or size_of(r) != size:
                continue
            K = int(d["row"]["config"].split("|K")[1].split("|")[0])
            pts.append((K, d))
        pts.sort(key=lambda p: p[0])
        for K, d in pts:
            out.append(f"| {K} | {fmt(d['t_total'])} | {fmt(d['t_dp'])} | {seq['t_total'] / d['t_total']:.2f} | "
                       f"{fmt(d['min'])}..{fmt(d['max'])} |")
        seqdp = [d for K, d in pts if K == 0]
        ks = [(K, d) for K, d in pts if K > 0]
        if seqdp:
            # Relative to the sequential DP of the same image, so both sizes share one linear axis.
            ax.plot([K for K, _ in ks], [d["t_dp"] / seqdp[0]["t_dp"] for _, d in ks], marker="o", markersize=6,
                    color=C[i], label=size)
    ax.set_xscale("log", base=2)
    ax.set_xticks([1, 2, 4, 8, 16, 32, 64, 128, 256], ["1", "2", "4", "8", "16", "32", "64", "128", "256"])
    ax.set_xlabel("block height K (rows per barrier)")
    ax.axhline(1, color=INK2, linewidth=1, linestyle=":")
    ax.text(256, 1, "sequential DP (K = 0) ", va="bottom", ha="right", fontsize=8, color=INK2)
    ax.set_ylim(0, 1.1)
    ax.set_ylabel("DP time / sequential DP time")
    ax.set_title("Blocked parallel DP: fewer barriers vs extra halo work (8 threads)", loc="left", fontsize=11)
    ax.legend()
    fig.tight_layout()
    fig.savefig(FIG / "m5_dp_block.png", dpi=150)
    plt.close(fig)


def threads(out):
    rows = []
    for name in ("m5_threads", "m5_threads_8k"):
        r = load(name)
        if r is not None:
            rows += r
    if not rows:
        return
    out.append("\n## Thread scaling (`m5_threads`, `m5_threads_8k`)\n")
    for name in ("m5_threads", "m5_threads_8k"):
        r = load(name)
        if r is not None:
            check(r, name, out)
    rows = [r for r in rows if r["warmup"] == "0"]
    med = medians(rows)
    sizes = sorted({size_of(d["row"]) for d in med.values()}, key=lambda s: int(s.split("x")[0]))
    fig, axes = plt.subplots(1, len(sizes), figsize=(3.2 * len(sizes), 3.6), sharey=True)
    if len(sizes) == 1:
        axes = [axes]
    for ax, size in zip(axes, sizes):
        seq = find(med, size=size, mode="seq")
        red = seq["row"]["reduction"]
        out.append(f"\n### {size}, -{round(float(red) * 100)} % width (seq {fmt(seq['t_total'])} s, "
                   f"median of {seq['n']})\n")
        out.append("| version | threads | t_total | speedup | efficiency | DP | removal |")
        out.append("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
        series = {"opt": [], "omp": []}
        for d in sorted(med.values(), key=lambda d: (d["row"]["mode"], int(d["row"]["threads"]))):
            r = d["row"]
            if size_of(r) != size or r["mode"] == "seq":
                continue
            t = int(r["threads"])
            sp = seq["t_total"] / d["t_total"]
            series[r["mode"]].append((t, sp))
            out.append(f"| {label(r)} | {t} | {fmt(d['t_total'])} | {sp:.2f} | {sp / t:.2f} | "
                       f"{fmt(d['t_dp'])} | {fmt(d['t_removal'])} |")
        for i, (mode, name) in enumerate((("opt", "optimized (opt, K=64)"), ("omp", "basic omp, DP seq"))):
            pts = sorted(series[mode])
            if pts:
                ax.plot([p[0] for p in pts], [p[1] for p in pts], marker="o", markersize=6, color=C[i], label=name)
        ax.axhline(1, color=INK2, linewidth=1, linestyle=":")
        ax.set_title(size, loc="left", fontsize=10)
        ax.set_xticks([1, 2, 4, 8, 12, 16])
        ax.set_xlabel("threads")
    axes[0].set_ylabel("speedup vs original sequential")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper right", bbox_to_anchor=(0.985, 1.0), ncol=2, fontsize=9)
    fig.suptitle("Speedup vs threads (Linux container, Ryzen 7 4800H, 8 cores / 16 threads)", x=0.01, ha="left",
                 fontsize=11)
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    fig.savefig(FIG / "m5_speedup_threads.png", dpi=150)
    plt.close(fig)


def misc(out):
    rows = load("m5_misc")
    if rows is None:
        return
    out.append("\n## Threshold, schedules, image content (`m5_misc`, opt incr+simd+compact, K=64)\n")
    rows = check(rows, "m5_misc", out)
    med = medians(rows)
    out.append("\n| image | version | threads | schedule (OMP_SCHEDULE) | par-min-pixels | t_total | speedup vs seq |")
    out.append("| --- | --- | ---: | --- | ---: | ---: | ---: |")
    for d in sorted(med.values(), key=lambda d: (d["row"]["image"], d["row"]["mode"] != "seq", d["row"]["config"])):
        r = d["row"]
        seq = find(med, mode="seq", image=r["image"])
        thr = r["config"].split("|thr")[1].split("|")[0] if r["mode"] == "opt" else "-"
        out.append(f"| {r['image']} | {label(r)} | {r['threads']} | {r['omp_schedule_env'] or '(static)'} | {thr} | "
                   f"{fmt(d['t_total'])} | {seq['t_total'] / d['t_total']:.2f} |")


def main():
    FIG.mkdir(parents=True, exist_ok=True)
    out = ["# M5 benchmark summary (generated by scripts/analyze_m5.py)", "",
           "All times are medians of the measured rounds (warm-up excluded), `t_total` = carve() only.", "",
           "Each section starts with a data-quality line: number of runs, runs with exit code != 0, and runs",
           "whose output image differs from the sequential output (SHA-256 `hash_ok`)."]
    ablation(out)
    block(out)
    threads(out)
    misc(out)
    (RES / "m5_summary.md").write_text("\n".join(out) + "\n", encoding="utf-8")
    print("\n".join(out))


if __name__ == "__main__":
    main()
