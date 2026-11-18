"""M4 analysis: medians, variability, speedup, efficiency and plots from results/*.csv.

Input : raw CSV files written by scripts/run_bench.py (one row per run).
Output: results/summary.md (Markdown tables used in docs/benchmark_report.md)
        results/summary.csv (one row per configuration)
        docs/figures/*.png (plots)

Rules:
  * warm-up rows (warmup=1), failed runs (exit != 0) and runs with a wrong output (hash_ok != 1) are
    excluded from every statistic; their number is reported.
  * T_seq is the median of --mode seq (the real sequential program), never omp with 1 thread.
  * Speedup S(p) = T_seq / T_omp(p) (medians), efficiency E(p) = S(p) / p.
  * Variability: min, max and the interquartile range (IQR = Q3 - Q1) of the measured runs.

Usage: python scripts/analyze_bench.py [results/a.csv results/b.csv ...]
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"
FIG = ROOT / "docs" / "figures"

STAGES = ["t_energy", "t_dp", "t_min", "t_backtrack", "t_removal"]
STAGE_NAMES = {"t_energy": "energy", "t_dp": "DP", "t_min": "min endpoint",
               "t_backtrack": "backtrack", "t_removal": "removal", "t_other": "other"}

# Reference palette (dataviz skill, references/palette.md), light mode, fixed slot order.
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
INK, INK2, MUTED = "#0b0b0b", "#52514e", "#898781"
GRID, AXIS, SURFACE = "#e1e0d9", "#c3c2b7", "#fcfcfb"

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": AXIS, "axes.labelcolor": INK2, "axes.titlecolor": INK,
    "xtick.color": MUTED, "ytick.color": MUTED, "text.color": INK,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.6,
    "axes.spines.top": False, "axes.spines.right": False,
    "lines.linewidth": 2, "lines.markersize": 8, "font.size": 10,
    "legend.frameon": False, "figure.dpi": 110,
})


# ---------------------------------------------------------------------------------------------
def load(paths):
    frames = [pd.read_csv(p) for p in paths]
    df = pd.concat(frames, ignore_index=True)
    total = len(df)
    df["exit"] = df["exit"].astype(str)
    bad_exit = (df["exit"] != "0")
    bad_hash = (~bad_exit) & (df["hash_ok"].astype(str).str.replace(".0", "", regex=False) != "1")
    stats = {"rows": total, "warmup": int((df["warmup"] == 1).sum()),
             "failed_or_timeout": int(bad_exit.sum()), "wrong_output": int(bad_hash.sum())}
    df = df[(df["warmup"] == 0) & ~bad_exit & ~bad_hash].copy()
    for c in STAGES + ["t_total", "threads", "seams", "in_w", "in_h"]:
        df[c] = pd.to_numeric(df[c])
    df["t_other"] = df["t_total"] - df[STAGES].sum(axis=1)
    df["size"] = df["in_w"].astype(int).astype(str) + "x" + df["in_h"].astype(int).astype(str)
    df["mpix"] = df["in_w"] * df["in_h"] / 1e6
    df["dp_variant"] = np.where(df["stages"].astype(str).str.contains("dp_persistent"), "B",
                                np.where(df["stages"].astype(str).str.contains("dp"), "A", "-"))
    return df, stats


def summarize(df):
    keys = ["experiment", "config", "image", "size", "mpix", "reduction", "seams", "mode", "threads",
            "stages", "schedule", "omp_schedule_env", "oversubscribed"]
    g = df.groupby(keys, dropna=False)
    s = g["t_total"].agg(n="count", median="median", tmin="min", tmax="max",
                         q1=lambda x: x.quantile(0.25), q3=lambda x: x.quantile(0.75)).reset_index()
    for c in STAGES + ["t_other"]:
        s[c] = g[c].median().values
    s["iqr_pct"] = 100 * (s["q3"] - s["q1"]) / s["median"]
    seq = (s[s["mode"] == "seq"].groupby(["experiment", "image", "reduction"])
           .agg(t_seq=("median", "first"), **{f"{c}_seq": (c, "first") for c in STAGES}).reset_index())
    s = s.merge(seq, on=["experiment", "image", "reduction"], how="left")
    s["speedup"] = s["t_seq"] / s["median"]
    s["efficiency"] = s["speedup"] / s["threads"]
    return s


def fmt(x, nd=3):
    return "–" if pd.isna(x) else f"{x:.{nd}f}"


def md_table(rows, cols, headers):
    out = ["| " + " | ".join(headers) + " |", "| " + " | ".join("---" for _ in headers) + " |"]
    for _, r in rows.iterrows():
        out.append("| " + " | ".join(c(r) if callable(c) else str(r[c]) for c in cols) + " |")
    return "\n".join(out)


def thread_label(r):
    return "seq" if r["mode"] == "seq" else f"{int(r['threads'])}{' (over)' if r['oversubscribed'] == 1 else ''}"


# ---------------------------------------------------------------------------------------------
def save(fig, name):
    FIG.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(FIG / name)
    plt.close(fig)
    print(f"wrote {FIG / name}")


def mark_oversubscription(ax, threads):
    if threads and max(threads) > 8:
        ax.axvspan(8.5, max(threads) * 1.15, color=GRID, alpha=0.5, lw=0)
        ax.text(max(threads), ax.get_ylim()[0], "oversubscribed\n(> 8 logical CPUs)", ha="right",
                va="bottom", fontsize=8, color=MUTED)


def plot_main(s, exp, tag, reduction=0.2):
    d = s[(s["experiment"] == exp) & (s["reduction"] == reduction)]
    if d.empty:
        return
    sizes = sorted(d["size"].unique(), key=lambda z: d[d["size"] == z]["mpix"].iloc[0])
    omp = d[d["mode"] == "omp"]
    threads = sorted(omp["threads"].unique())

    # 1. time vs threads (log y: sizes differ by orders of magnitude)
    fig, ax = plt.subplots(figsize=(6.4, 4.2))
    for i, z in enumerate(sizes):
        o = omp[omp["size"] == z].sort_values("threads")
        ax.errorbar(o["threads"], o["median"], yerr=[o["median"] - o["tmin"], o["tmax"] - o["median"]],
                    color=SERIES[i], marker="o", capsize=3, label=f"{z} omp")
        t_seq = d[(d["size"] == z) & (d["mode"] == "seq")]["median"]
        if not t_seq.empty:
            ax.axhline(t_seq.iloc[0], color=SERIES[i], lw=1, ls="--")
    ax.set_xscale("log", base=2); ax.set_xticks(threads, [str(int(t)) for t in threads])
    ax.set_yscale("log"); ax.set_xlabel("OpenMP threads"); ax.set_ylabel("t_total (s), median, min–max")
    ax.set_title(f"Execution time vs threads ({tag}, −{int(reduction*100)} % width)\n"
                 "dashed = sequential program", fontsize=10)
    ax.legend(fontsize=8); mark_oversubscription(ax, threads)
    save(fig, f"time_vs_threads_{exp}.png")

    # 2. speedup and 3. efficiency
    for metric, ylab, fname in [("speedup", "speedup S(p) = T_seq / T_omp(p)", "speedup"),
                                ("efficiency", "efficiency E(p) = S(p) / p", "efficiency")]:
        fig, ax = plt.subplots(figsize=(6.4, 4.2))
        for i, z in enumerate(sizes):
            o = omp[omp["size"] == z].sort_values("threads")
            ax.plot(o["threads"], o[metric], color=SERIES[i], marker="o", label=z)
        if metric == "speedup":
            ax.plot(threads, threads, color=MUTED, ls="--", lw=1, label="ideal S = p")
            ax.axhline(1, color=AXIS, lw=1)
            if omp["speedup"].min() < 0.3:
                ax.set_yscale("log")
        else:
            ax.axhline(1, color=MUTED, ls="--", lw=1, label="ideal E = 1")
        ax.set_xscale("log", base=2); ax.set_xticks(threads, [str(int(t)) for t in threads])
        ax.set_xlabel("OpenMP threads"); ax.set_ylabel(ylab)
        ax.set_title(f"{fname.capitalize()} vs threads ({tag}, −{int(reduction*100)} % width)", fontsize=10)
        ax.legend(fontsize=8); mark_oversubscription(ax, threads)
        save(fig, f"{fname}_vs_threads_{exp}.png")


def plot_sizes(s, reduction=0.2):
    """5. Performance across image sizes: speedup at each thread count, both combined versions."""
    exps = [e for e in ("main", "main_nodp") if e in set(s["experiment"])]
    if not exps:
        return
    fig, axes = plt.subplots(1, len(exps), figsize=(5.2 * len(exps), 4.0), squeeze=False)
    for ax, exp in zip(axes[0], exps):
        d = s[(s["experiment"] == exp) & (s["reduction"] == reduction) & (s["mode"] == "omp")]
        for i, t in enumerate(sorted(d["threads"].unique())):
            o = d[d["threads"] == t].sort_values("mpix")
            ax.plot(o["mpix"], o["speedup"], color=SERIES[i], marker="o",
                    label=f"{int(t)} threads{' (over)' if t > 8 else ''}")
        ax.axhline(1, color=MUTED, ls="--", lw=1)
        ax.set_xscale("log"); ax.set_xlabel("image size (megapixels, log)")
        ax.set_ylabel("speedup vs sequential")
        if not d.empty and d["speedup"].min() < 0.3:
            ax.set_yscale("log")
        ax.set_title({"main": "all stages parallel (energy+DP+min+removal)",
                      "main_nodp": "DP sequential (energy+min+removal)"}[exp], fontsize=10)
        ax.legend(fontsize=8)
    fig.suptitle(f"Speedup across image sizes (−{int(reduction*100)} % width)", fontsize=11)
    save(fig, "speedup_vs_size.png")


def plot_breakdown(s):
    """4. Run-time breakdown: stacked stage times (seq vs omp versions) + per-stage speedup."""
    d = s[s["experiment"] == "stages"]
    if d.empty:
        return
    seq = d[d["mode"] == "seq"].iloc[0]
    parts = STAGES + ["t_other"]

    # 4a stacked bars, versions WITHOUT the parallel DP (same scale) + one with DP per thread count
    rows = [("seq", seq)]
    for t in sorted(d[d["mode"] == "omp"]["threads"].unique()):
        r = d[(d["threads"] == t) & (d["stages"] == "energy+min+removal")]
        if not r.empty:
            rows.append((f"omp {int(t)}t\nenergy+min+removal", r.iloc[0]))
    fig, ax = plt.subplots(figsize=(7.2, 4.2))
    y = np.arange(len(rows))
    left = np.zeros(len(rows))
    for i, p in enumerate(parts):
        vals = np.array([r[p] for _, r in rows])
        ax.barh(y, vals, left=left, color=SERIES[i], edgecolor=SURFACE, linewidth=2, height=0.6,
                label=STAGE_NAMES[p])
        left += vals
    ax.set_yticks(y, [n for n, _ in rows], fontsize=8); ax.invert_yaxis()
    ax.set_xlabel("time (s), median"); ax.grid(axis="y", visible=False)
    ax.set_title(f"Run-time breakdown, {seq['size']}, −{int(seq['reduction']*100)} % width "
                 "(DP sequential in all bars)", fontsize=10)
    ax.legend(fontsize=8, ncol=3, loc="lower right")
    save(fig, "breakdown_stages.png")

    # 4b per-stage time when ONLY that stage is parallel, vs threads (log y), seq as dashed line
    single = {"t_energy": "energy", "t_dp": "dp", "t_min": "min", "t_removal": "removal"}
    fig, axes = plt.subplots(1, 4, figsize=(13, 3.6))
    for ax, (col, st) in zip(axes, single.items()):
        o = d[(d["mode"] == "omp") & (d["stages"] == st)].sort_values("threads")
        ax.plot(o["threads"], o[col], color=SERIES[0], marker="o", label="parallel" if st != "dp" else "DP variant A")
        if st == "dp":
            ob = d[(d["mode"] == "omp") & (d["stages"] == "dp_persistent")].sort_values("threads")
            ax.plot(ob["threads"], ob[col], color=SERIES[1], marker="s", label="DP variant B")
        ax.axhline(seq[col], color=MUTED, ls="--", lw=1, label="sequential")
        ax.set_xscale("log", base=2)
        ax.set_xticks(o["threads"], [str(int(t)) for t in o["threads"]])
        ax.set_yscale("log"); ax.set_title(STAGE_NAMES[col], fontsize=10); ax.set_xlabel("threads")
        ax.legend(fontsize=7)
    axes[0].set_ylabel("stage time (s), median")
    fig.suptitle(f"Stage time when only this stage is parallel ({seq['size']})", fontsize=11)
    save(fig, "stage_times_vs_threads.png")


def plot_schedule(s):
    d = s[(s["experiment"] == "schedule") & (s["mode"] == "omp")]
    if d.empty:
        return
    fig, axes = plt.subplots(1, 2, figsize=(11, 3.8))
    for ax, st in zip(axes, ["energy+min+removal", "energy+dp+min+removal"]):
        o = d[d["stages"] == st]
        if o.empty:
            ax.set_visible(False)
            continue
        scheds = list(dict.fromkeys(o["omp_schedule_env"]))
        threads = sorted(o["threads"].unique())
        w = 0.8 / len(threads)
        for i, t in enumerate(threads):
            vals = [o[(o["threads"] == t) & (o["omp_schedule_env"] == sc)]["median"].mean() for sc in scheds]
            ax.bar(np.arange(len(scheds)) + i * w, vals, width=w - 0.02, color=SERIES[i],
                   label=f"{int(t)} threads")
        ax.set_xticks(np.arange(len(scheds)) + w * (len(threads) - 1) / 2, scheds, fontsize=8)
        seqt = s[(s["experiment"] == "schedule") & (s["mode"] == "seq")]["median"]
        if not seqt.empty:
            ax.axhline(seqt.iloc[0], color=MUTED, ls="--", lw=1, label="sequential")
        if st.count("dp"):
            ax.set_yscale("log")
        ax.set_title(st, fontsize=10); ax.set_ylabel("t_total (s), median"); ax.grid(axis="x", visible=False)
        ax.legend(fontsize=7)
    fig.suptitle("Effect of the OpenMP schedule (OMP_SCHEDULE)", fontsize=11)
    save(fig, "schedule.png")


# ---------------------------------------------------------------------------------------------
def report(s, stats, df):
    L = ["# Benchmark summary (generated by scripts/analyze_bench.py)", "",
         f"Rows read: {stats['rows']}; warm-up rows excluded: {stats['warmup']}; failed/timeout: "
         f"{stats['failed_or_timeout']}; wrong output (hash): {stats['wrong_output']}.", ""]
    common = [("size", "size"), ("seams", "seams"), (thread_label, "threads"), ("stages", "parallel stages"),
              (lambda r: str(int(r["n"])), "n"), (lambda r: fmt(r["median"]), "median t_total (s)"),
              (lambda r: f"{fmt(r['tmin'])} – {fmt(r['tmax'])}", "min – max"),
              (lambda r: fmt(r["iqr_pct"], 1), "IQR %"), (lambda r: fmt(r["speedup"], 2), "speedup"),
              (lambda r: fmt(r["efficiency"], 2), "efficiency")]
    for exp in s["experiment"].unique():
        d = s[s["experiment"] == exp].sort_values(["mpix", "reduction", "mode", "stages", "threads"],
                                                  ascending=[True, True, False, True, True])
        L += [f"## {exp}", ""]
        if exp == "schedule":
            cols = common[:4] + [("omp_schedule_env", "OMP_SCHEDULE")] + common[4:]
        elif exp == "content":
            cols = [("image", "image")] + common
        else:
            cols = common
        if exp in ("stages",):
            cols = cols + [(lambda r: fmt(r["t_energy"]), "t_energy"), (lambda r: fmt(r["t_dp"]), "t_dp"),
                           (lambda r: fmt(r["t_min"], 4), "t_min"), (lambda r: fmt(r["t_backtrack"], 4), "t_backtrack"),
                           (lambda r: fmt(r["t_removal"], 4), "t_removal")]
        if exp.startswith("main") or exp == "pilot":
            L.append(md_table(d, [c for c, _ in cols[:1]] + [lambda r: fmt(r["reduction"], 1)] +
                              [c for c, _ in cols[1:]],
                              [h for _, h in cols[:1]] + ["reduction"] + [h for _, h in cols[1:]]))
        else:
            L.append(md_table(d, [c for c, _ in cols], [h for _, h in cols]))
        L.append("")

    # Fixed cost vs per-seam cost (sequential): t_total = a + b * seams, per size (least squares).
    seq = df[(df["mode"] == "seq") & df["experiment"].isin(["main", "main_nodp"])]
    if not seq.empty and seq["reduction"].nunique() >= 2:
        L += ["## Fixed cost vs per-seam cost (sequential, least squares t_total = a + b·seams)", "",
              "| size | a = fixed part (s) | b = per seam (ms) | runs |", "| --- | --- | --- | --- |"]
        for z, g in seq.groupby("size"):
            b, a = np.polyfit(g["seams"], g["t_total"], 1)
            L.append(f"| {z} | {a:.4f} | {1000*b:.3f} | {len(g)} |")
        L.append("")
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / "summary.md").write_text("\n".join(L), encoding="utf-8")
    s.to_csv(RESULTS / "summary.csv", index=False)
    print(f"wrote {RESULTS / 'summary.md'}, {RESULTS / 'summary.csv'}")


def main():
    global RESULTS, FIG
    args = sys.argv[1:]
    if "--out" in args:                     # write summary + figures elsewhere (script testing)
        i = args.index("--out")
        RESULTS = FIG = Path(args[i + 1])
        args = args[:i] + args[i + 2:]
    sys.argv = [sys.argv[0]] + args
    # Default: all result files except the summary and the pilot runs (planning data, 1 run each).
    paths = [Path(p) for p in sys.argv[1:]] or sorted(
        p for p in RESULTS.glob("*.csv") if p.name != "summary.csv" and not p.name.startswith("pilot"))
    df, stats = load(paths)
    s = summarize(df)
    report(s, stats, df)
    plot_main(s, "main", "all stages parallel")
    plot_main(s, "main_nodp", "DP sequential")
    plot_sizes(s)
    plot_breakdown(s)
    plot_schedule(s)


if __name__ == "__main__":
    main()
