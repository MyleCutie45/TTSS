"""M4 analysis: medians, variability, speedup, efficiency and plots from results/*.csv.

Input : raw CSV files written by scripts/run_bench.py (one row per run).
Output: results/summary.md (Markdown tables used in docs/benchmark_report.md)
        results/summary.csv (one row per configuration)
        docs/figures/*.png (plots)

Rules:
  * warm-up rows (warmup=1), failed runs (exit != 0) and runs with a wrong output (hash_ok != 1) are
    excluded from every statistic; their number is reported.
  * T_seq is the median of --mode seq (the real sequential program), never omp with 1 thread.
    Speedups always compare runs of the SAME experiment (same session, same platform).
  * Speedup S(p) = T_seq / T_omp(p) (medians), efficiency E(p) = S(p) / p.
  * Variability: min, max and the interquartile range (IQR = Q3 - Q1) of the measured runs.

Usage: python scripts/analyze_bench.py [--out DIR] [results/a.csv results/b.csv ...]
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
import pandas as pd  # noqa: E402
from matplotlib.lines import Line2D  # noqa: E402
from matplotlib.ticker import FuncFormatter, NullFormatter, NullLocator  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"
FIG = ROOT / "docs" / "figures"

STAGES = ["t_energy", "t_dp", "t_min", "t_backtrack", "t_removal"]
STAGE_NAMES = {"t_energy": "energy", "t_dp": "DP", "t_min": "min endpoint",
               "t_backtrack": "backtrack", "t_removal": "removal", "t_other": "other"}
NODP = "energy+min+removal"
PLATFORM = {"stages": "Windows", "wsl": "Linux (WSL2)"}

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
    "figure.constrained_layout.use": True,
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
    return df, stats


def summarize(df):
    keys = ["experiment", "config", "image", "size", "mpix", "in_w", "in_h", "reduction", "seams", "mode",
            "threads", "stages", "schedule", "omp_schedule_env", "oversubscribed"]
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


def seq_work(w, h, seams):
    """Pixel visits of a whole run. Seam k (k = 0, 1, ...) is computed on an image of width w - k,
    so the work is P = h * sum_{k<s} (w - k) = h * (s*w - s*(s-1)/2)."""
    return h * (seams * w - seams * (seams - 1) / 2)


def fmt(x, nd=3):
    return "–" if pd.isna(x) else f"{x:.{nd}f}"


def md_table(rows, cols, headers):
    out = ["| " + " | ".join(headers) + " |", "| " + " | ".join("---" for _ in headers) + " |"]
    for _, r in rows.iterrows():
        out.append("| " + " | ".join(c(r) if callable(c) else str(r[c]) for c in cols) + " |")
    return "\n".join(out)


def thread_label(r):
    return "seq" if r["mode"] == "seq" else f"{int(r['threads'])}{' (over)' if r['oversubscribed'] == 1 else ''}"


def pick(d, **eq):
    """Rows of d whose columns equal the given values."""
    m = np.ones(len(d), dtype=bool)
    for k, v in eq.items():
        m &= (d[k] == v).values
    return d[m]


# ---------------------------------------------------------------------------------------------
# Derived tables (also used by the plots)
# ---------------------------------------------------------------------------------------------
def dp_rows(s):
    """DP time per image row: sequential and variants A / B (only the DP parallel), per platform."""
    out = []
    for exp, plat in PLATFORM.items():
        d = s[s["experiment"] == exp]
        for _, sq in d[d["mode"] == "seq"].iterrows():
            same = pick(d, image=sq["image"], reduction=sq["reduction"], mode="omp")
            dp = same[same["stages"].isin(["dp", "dp_persistent"])]
            if dp.empty:
                continue
            rows = sq["seams"] * (sq["in_h"] - 1)       # row 0 is a copy; rows 1..H-1 per seam
            out.append({"experiment": exp, "platform": plat, "size": sq["size"], "threads": 0, "variant": "seq",
                        "t_dp": sq["t_dp"], "us_per_row": 1e6 * sq["t_dp"] / rows, "extra_us": 0.0, "over": 0})
            for _, r in dp.sort_values("threads").iterrows():
                out.append({"experiment": exp, "platform": plat, "size": sq["size"], "threads": int(r["threads"]),
                            "variant": "A" if r["stages"] == "dp" else "B", "t_dp": r["t_dp"],
                            "us_per_row": 1e6 * r["t_dp"] / rows,
                            "extra_us": 1e6 * (r["t_dp"] - sq["t_dp"]) / rows, "over": r["oversubscribed"]})
    return pd.DataFrame(out)


def stage_speedups(s, exp="stages"):
    """t_stage(seq) / t_stage(omp) when only that stage is parallel, and inside energy+min+removal."""
    d = s[s["experiment"] == exp]
    seq = d[d["mode"] == "seq"]
    if seq.empty:
        return [], []
    sq = seq.iloc[0]
    d = pick(d, image=sq["image"], reduction=sq["reduction"], mode="omp")
    single, combined = [], []
    for t in sorted(d["threads"].unique()):
        def sp(stages, col):
            r = pick(d, threads=t, stages=stages)
            return sq[col] / r.iloc[0][col] if not r.empty else np.nan
        lab = f"{int(t)}{' (over)' if pick(d, threads=t)['oversubscribed'].max() == 1 else ''}"
        single.append([lab, sp("energy", "t_energy"), sp("dp", "t_dp"), sp("dp_persistent", "t_dp"),
                       sp("min", "t_min"), sp("removal", "t_removal")])
        r = pick(d, threads=t, stages=NODP)
        if not r.empty:
            r = r.iloc[0]
            combined.append([lab, sq["t_energy"] / r["t_energy"], sq["t_min"] / r["t_min"],
                             sq["t_removal"] / r["t_removal"], r["speedup"]])
    return single, combined


def stage_speedups_by_size(s, exp="main_nodp", reduction=0.2):
    """Stage speedups t_stage(seq) / t_stage(omp) of one combined version, per image size."""
    d = s[(s["experiment"] == exp) & (s["reduction"] == reduction)]
    out = []
    for z in sorted(d["size"].unique(), key=lambda z: d[d["size"] == z]["mpix"].iloc[0]):
        g = d[d["size"] == z]
        sq = g[g["mode"] == "seq"]
        if sq.empty:
            continue
        sq = sq.iloc[0]
        for _, r in g[g["mode"] == "omp"].sort_values("threads").iterrows():
            out.append([z, thread_label(r)] + [sq[c] / r[c] for c in ("t_energy", "t_dp", "t_min", "t_removal")]
                       + [r["speedup"]])
    return out


def amdahl(s, exp):
    """Amdahl's law for energy+min+removal: f = parallel fraction of the sequential run time."""
    d = s[s["experiment"] == exp]
    out = []
    for _, sq in d[d["mode"] == "seq"].iterrows():
        r = pick(d, image=sq["image"], reduction=sq["reduction"], mode="omp", stages=NODP).sort_values("threads")
        if r.empty:
            continue
        f = (sq["t_energy"] + sq["t_min"] + sq["t_removal"]) / sq["median"]
        for _, x in r.iterrows():
            p = x["threads"]
            out.append([sq["size"], f, 1 / (1 - f), int(p), 1 / ((1 - f) + f / p), x["speedup"]])
    return out


def platform_table(s):
    """Same configuration on Windows (main experiments) and on Linux (wsl experiment)."""
    w = s[s["experiment"] == "wsl"]
    if w.empty:
        return None
    win = s[s["experiment"].isin(["stages", "main_nodp", "main"]) & s["omp_schedule_env"].isna()]
    prio = {"stages": 0, "main_nodp": 1, "main": 2}
    rows = []
    for _, r in w.sort_values(["mpix", "mode", "stages", "threads"], ascending=[True, False, True, True]).iterrows():
        m = pick(win, image=r["image"], reduction=r["reduction"], mode=r["mode"], threads=r["threads"],
                 stages=r["stages"])
        if m.empty:
            continue
        m = m.assign(p=m["experiment"].map(prio)).sort_values("p").iloc[0]
        rows.append([r["size"], int(r["seams"]), thread_label(r), r["stages"], m["median"], m["speedup"],
                     r["median"], r["speedup"], m["experiment"]])
    return rows


# ---------------------------------------------------------------------------------------------
# Plot helpers
# ---------------------------------------------------------------------------------------------
def save(fig, name):
    FIG.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIG / name, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {FIG / name}")


def plain_log(ax, axis="y"):
    """Log axis labelled with plain numbers (0.01, 0.1, 1, 10) and no minor labels."""
    a = ax.yaxis if axis == "y" else ax.xaxis
    a.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    a.set_minor_formatter(NullFormatter())


def thread_axis(ax, threads, over, label="OpenMP threads"):
    """log2 x axis, one tick per thread count; oversubscribed counts are shaded and marked '(over)'."""
    threads = [int(t) for t in threads]
    ax.set_xscale("log", base=2)
    ax.set_xticks(threads, [f"{t}\n(over)" if t in over else str(t) for t in threads])
    ax.xaxis.set_minor_locator(NullLocator())
    ax.set_xlim(min(threads) / 1.3, max(threads) * 1.3)
    if over:
        below = [t for t in threads if t not in over]
        start = np.sqrt(max(below) * min(over)) if below else min(over) / 1.3
        ax.axvspan(start, max(threads) * 1.3, color=GRID, alpha=0.5, lw=0)
        label += "  (shaded: more threads than logical CPUs)"
    ax.set_xlabel(label)


def legend_right(ax, handles=None, labels=None):
    if handles is None:
        handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc="upper left", bbox_to_anchor=(1.02, 1), fontsize=8)


# ---------------------------------------------------------------------------------------------
# Plots
# ---------------------------------------------------------------------------------------------
def plot_main(s, exp, tag, reduction=0.2):
    d = s[(s["experiment"] == exp) & (s["reduction"] == reduction)]
    if d.empty:
        return
    sizes = sorted(d["size"].unique(), key=lambda z: d[d["size"] == z]["mpix"].iloc[0])
    omp = d[d["mode"] == "omp"]
    threads = sorted(omp["threads"].unique())
    over = {int(t) for t in omp[omp["oversubscribed"] == 1]["threads"]}
    what = f"({tag}, −{int(reduction * 100)} % width)"

    # 1. time vs threads (log y: sizes differ by orders of magnitude)
    fig, ax = plt.subplots(figsize=(7.4, 4.2))
    for i, z in enumerate(sizes):
        o = omp[omp["size"] == z].sort_values("threads")
        ax.errorbar(o["threads"], o["median"], yerr=[o["median"] - o["tmin"], o["tmax"] - o["median"]],
                    color=SERIES[i], marker="o", capsize=3, label=z)
        t_seq = d[(d["size"] == z) & (d["mode"] == "seq")]["median"]
        if not t_seq.empty:
            ax.axhline(t_seq.iloc[0], color=SERIES[i], lw=1, ls="--")
    thread_axis(ax, threads, over)
    ax.set_yscale("log"); plain_log(ax)
    ax.set_ylabel("t_total (s): median, bars = min–max")
    ax.set_title(f"Execution time vs threads {what}", fontsize=10)
    h, lab = ax.get_legend_handles_labels()
    legend_right(ax, h + [Line2D([], [], color=MUTED, lw=1, ls="--")],
                 lab + ["sequential program\n(dashed, same colour)"])
    save(fig, f"time_vs_threads_{exp}.png")

    # 2. speedup and 3. efficiency
    for metric, ylab, fname in [("speedup", "speedup S(p) = T_seq / T_omp(p)", "speedup"),
                                ("efficiency", "efficiency E(p) = S(p) / p", "efficiency")]:
        fig, ax = plt.subplots(figsize=(7.4, 4.2))
        for i, z in enumerate(sizes):
            o = omp[omp["size"] == z].sort_values("threads")
            ax.plot(o["threads"], o[metric], color=SERIES[i], marker="o", label=z)
        if metric == "speedup":
            ax.plot(threads, threads, color=MUTED, ls="--", lw=1, label="ideal S = p")
            ax.axhline(1, color=AXIS, lw=1)
            if omp["speedup"].min() < 0.3:
                ax.set_yscale("log"); plain_log(ax)
            else:   # linear: the ideal line leaves the plot at the top, the measured lines stay readable
                ax.set_ylim(0, max(4.5, 1.15 * omp["speedup"].max()))
        else:
            ax.axhline(1, color=MUTED, ls="--", lw=1, label="ideal E = 1")
            ax.set_ylim(0, 1.1)
        thread_axis(ax, threads, over)
        ax.set_ylabel(ylab)
        ax.set_title(f"{fname.capitalize()} vs threads {what}", fontsize=10)
        legend_right(ax)
        save(fig, f"{fname}_vs_threads_{exp}.png")


def plot_sizes(s, reduction=0.2):
    """5. Performance across image sizes: speedup at each thread count, both combined versions."""
    exps = [e for e in ("main", "main_nodp") if e in set(s["experiment"])]
    if not exps:
        return
    fig, axes = plt.subplots(1, len(exps), figsize=(5.4 * len(exps) + 1.4, 4.0), squeeze=False)
    for ax, exp in zip(axes[0], exps):
        d = s[(s["experiment"] == exp) & (s["reduction"] == reduction) & (s["mode"] == "omp")]
        for i, t in enumerate(sorted(d["threads"].unique())):
            o = d[d["threads"] == t].sort_values("mpix")
            over = (o["oversubscribed"] == 1).any()
            ax.plot(o["mpix"], o["speedup"], color=SERIES[i], marker="o",
                    label=f"{int(t)} thread{'s' if t > 1 else ''}{' (over)' if over else ''}")
        ax.axhline(1, color=MUTED, ls="--", lw=1)
        sz = d.drop_duplicates("size").sort_values("mpix")
        ax.set_xscale("log")
        ax.set_xticks(sz["mpix"], [f"{z}\n{m:.2g} MP" for z, m in zip(sz["size"], sz["mpix"])], fontsize=8)
        ax.xaxis.set_minor_locator(NullLocator())
        ax.set_xlabel("image size (log scale)")
        ax.set_ylabel("speedup vs sequential (dashed: 1 = same speed)")
        if not d.empty and d["speedup"].min() < 0.3:
            ax.set_yscale("log"); plain_log(ax)
        else:
            ax.set_ylim(0, 1.15 * d["speedup"].max())
        ax.set_title({"main": "all stages parallel (energy+DP+min+removal)",
                      "main_nodp": "DP sequential (energy+min+removal)"}[exp], fontsize=10)
    legend_right(axes[0][-1])
    fig.suptitle(f"Speedup across image sizes (−{int(reduction * 100)} % width)", fontsize=11)
    save(fig, "speedup_vs_size.png")


def plot_breakdown(s):
    """4. Run-time breakdown: stacked stage times (seq vs omp versions) + per-stage times."""
    d = s[s["experiment"] == "stages"]
    if d.empty:
        return
    seq = d[d["mode"] == "seq"].iloc[0]
    parts = STAGES + ["t_other"]

    # 4a stacked bars: seq and energy+min+removal (DP sequential, same scale) for every thread count
    rows = [("seq", seq)]
    for t in sorted(d[d["mode"] == "omp"]["threads"].unique()):
        r = d[(d["threads"] == t) & (d["stages"] == NODP)]
        if not r.empty:
            over = " (over)" if r.iloc[0]["oversubscribed"] == 1 else ""
            rows.append((f"omp {int(t)} thread{'s' if t > 1 else ''}{over}\n{NODP}", r.iloc[0]))
    fig, ax = plt.subplots(figsize=(8.4, 4.2))
    y = np.arange(len(rows))
    left = np.zeros(len(rows))
    for i, p in enumerate(parts):
        vals = np.array([r[p] for _, r in rows])
        ax.barh(y, vals, left=left, color=SERIES[i], edgecolor=SURFACE, linewidth=2, height=0.6,
                label=STAGE_NAMES[p])
        left += vals
    ax.set_yticks(y, [n for n, _ in rows], fontsize=8); ax.invert_yaxis()
    ax.set_xlabel("time (s), median"); ax.grid(axis="y", visible=False)
    ax.set_title(f"Run-time breakdown, {seq['size']}, −{int(seq['reduction'] * 100)} % width "
                 "(DP sequential in all bars)", fontsize=10)
    legend_right(ax)
    save(fig, "breakdown_stages.png")

    # 4b per-stage time when ONLY that stage is parallel, vs threads; seq as dashed line
    single = {"t_energy": "energy", "t_dp": "dp", "t_min": "min", "t_removal": "removal"}
    fig, axes = plt.subplots(1, 4, figsize=(14, 3.9))
    for ax, (col, st) in zip(axes, single.items()):
        o = d[(d["mode"] == "omp") & (d["stages"] == st)].sort_values("threads")
        ax.plot(o["threads"], o[col], color=SERIES[0], marker="o",
                label="parallel" if st != "dp" else "DP variant A")
        vals = list(o[col]) + [seq[col]]
        if st == "dp":
            ob = d[(d["mode"] == "omp") & (d["stages"] == "dp_persistent")].sort_values("threads")
            ax.plot(ob["threads"], ob[col], color=SERIES[1], marker="s", label="DP variant B")
            vals += list(ob[col])
        ax.axhline(seq[col], color=MUTED, ls="--", lw=1, label="sequential")
        thread_axis(ax, sorted(o["threads"].unique()), {int(t) for t in o[o["oversubscribed"] == 1]["threads"]},
                    label="threads")
        ax.set_xlabel("threads")
        if max(vals) / min(vals) > 20:
            ax.set_yscale("log"); plain_log(ax)
            ax.set_ylim(bottom=10 ** np.floor(np.log10(min(vals))))   # a labelled decade below the data
        else:
            ax.set_ylim(0, 1.15 * max(vals))
        ax.set_title(f"{STAGE_NAMES[col]} (only this stage parallel)", fontsize=9)
        ax.legend(fontsize=7)
    axes[0].set_ylabel("stage time (s), median")
    fig.suptitle(f"Stage time vs threads ({seq['size']}, −{int(seq['reduction'] * 100)} % width)", fontsize=11)
    save(fig, "stage_times_vs_threads.png")


def plot_schedule(s):
    d = s[(s["experiment"] == "schedule") & (s["mode"] == "omp")]
    if d.empty:
        return
    fig, axes = plt.subplots(1, 2, figsize=(12, 4.0))
    for ax, st in zip(axes, [NODP, "energy+dp+min+removal"]):
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
            ax.axhline(seqt.iloc[0], color=INK2, ls="--", lw=1, label="sequential program")
        ax.set_title(f"parallel stages: {st}", fontsize=10)
        ax.set_ylabel("t_total (s), median"); ax.set_xlabel("OMP_SCHEDULE")
        ax.grid(axis="x", visible=False)
    legend_right(axes[-1] if axes[-1].get_visible() else axes[0])
    fig.suptitle("Effect of the OpenMP schedule (1920x1080, −20 % width)", fontsize=11)
    save(fig, "schedule.png")


def plot_dp_rows(dp):
    """DP time per image row on each platform: sequential vs variant A / B."""
    if dp.empty:
        return
    plats = [p for p in PLATFORM.values() if p in set(dp["platform"])]
    fig, axes = plt.subplots(1, len(plats), figsize=(5.2 * len(plats) + 1.4, 4.0), squeeze=False, sharey=True)
    for ax, plat in zip(axes[0], plats):
        d = dp[dp["platform"] == plat]
        threads = sorted(d[d["variant"] != "seq"]["threads"].unique())
        for i, v in enumerate(["A", "B"]):
            o = d[d["variant"] == v].sort_values("threads")
            ax.plot(o["threads"], o["us_per_row"], color=SERIES[i], marker="os"[i],
                    label=f"DP variant {v} ({'parallel for per row' if v == 'A' else 'one region per seam'})")
        sq = d[d["variant"] == "seq"]["us_per_row"]
        ax.axhline(sq.iloc[0], color=INK2, ls="--", lw=1, label="sequential DP")
        thread_axis(ax, threads, {int(t) for t in d[d["over"] == 1]["threads"]}, label="threads")
        ax.set_xlabel("threads")
        ax.set_yscale("log"); plain_log(ax)
        ax.set_ylim(bottom=10 ** np.floor(np.log10(dp["us_per_row"].min())))   # labelled decade below the data
        ax.set_title(f"{plat}, {d['size'].iloc[0]}", fontsize=10)
    axes[0][0].set_ylabel("DP time per image row (µs), median")
    legend_right(axes[0][-1])
    fig.suptitle("Cost of one DP row: same code, same laptop, two OpenMP runtimes", fontsize=11)
    save(fig, "dp_time_per_row.png")


# ---------------------------------------------------------------------------------------------
def report(s, stats, df, dp):
    L = ["# Benchmark summary (generated by scripts/analyze_bench.py)", "",
         f"Rows read: {stats['rows']}; warm-up rows excluded: {stats['warmup']}; failed/timeout: "
         f"{stats['failed_or_timeout']}; wrong output (hash): {stats['wrong_output']}.", ""]
    common = [("size", "size"), ("seams", "seams"), (thread_label, "threads"), ("stages", "parallel stages"),
              (lambda r: str(int(r["n"])), "n"), (lambda r: fmt(r["median"]), "median t_total (s)"),
              (lambda r: f"{fmt(r['tmin'])} – {fmt(r['tmax'])}", "min – max"),
              (lambda r: fmt(r["iqr_pct"], 1), "IQR %"), (lambda r: fmt(r["speedup"], 2), "speedup"),
              (lambda r: fmt(r["efficiency"], 2), "efficiency")]
    stage_cols = [(lambda r: fmt(r["t_energy"]), "t_energy"), (lambda r: fmt(r["t_dp"]), "t_dp"),
                  (lambda r: fmt(r["t_min"], 4), "t_min"), (lambda r: fmt(r["t_backtrack"], 4), "t_backtrack"),
                  (lambda r: fmt(r["t_removal"], 4), "t_removal")]
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
        if exp in ("stages", "wsl"):
            cols = cols + stage_cols
        if exp.startswith("main") or exp in ("pilot", "wsl"):
            L.append(md_table(d, [c for c, _ in cols[:1]] + [lambda r: fmt(r["reduction"], 1)] +
                              [c for c, _ in cols[1:]],
                              [h for _, h in cols[:1]] + ["reduction"] + [h for _, h in cols[1:]]))
        else:
            L.append(md_table(d, [c for c, _ in cols], [h for _, h in cols]))
        L.append("")

    # ---- stage speedups and Amdahl's law (stages experiment) ----
    single, combined = stage_speedups(s)
    if single:
        L += ["## Stage speedup t_stage(seq) / t_stage(omp) (stages experiment)", "",
              "Only the named stage is parallel; all other stages run sequentially.", "",
              "| threads | energy | DP variant A | DP variant B | min endpoint | removal |",
              "| --- | --- | --- | --- | --- | --- |"]
        L += [f"| {r[0]} | " + " | ".join(fmt(x, 2) for x in r[1:]) + " |" for r in single]
        L.append("")
    if combined:
        L += [f"## Stage speedup inside the combined version {NODP} (stages experiment)", "",
              "| threads | energy | min endpoint | removal | whole run (t_total) |", "| --- | --- | --- | --- | --- |"]
        L += [f"| {r[0]} | " + " | ".join(fmt(x, 2) for x in r[1:]) + " |" for r in combined]
        L.append("")
    rows = stage_speedups_by_size(s)
    if rows:
        L += [f"## Stage speedup by image size ({NODP} parallel, DP sequential; main_nodp, −20 % width)", "",
              "t_stage(seq) / t_stage(omp). The DP column is the SEQUENTIAL DP inside the parallel program "
              "(1.00 = as fast as in the sequential program).", "",
              "| size | threads | energy | DP (sequential) | min endpoint | removal | whole run |",
              "| --- | --- | --- | --- | --- | --- | --- |"]
        L += [f"| {r[0]} | {r[1]} | " + " | ".join(fmt(x, 2) for x in r[2:]) + " |" for r in rows]
        L.append("")
    for exp in ("stages", "wsl"):
        rows = amdahl(s, exp)
        if rows:
            L += [f"## Amdahl's law for {NODP} ({exp} experiment)", "",
                  "f = (t_energy + t_min + t_removal) / t_total of the sequential run (the part that is "
                  "parallel in this version). Limit = 1 / ((1 − f) + f / p): the speedup if the parallel "
                  "part scaled perfectly and cost nothing extra.", "",
                  "| size | f | limit for p → ∞ | threads p | Amdahl limit S(p) | measured S(p) |",
                  "| --- | --- | --- | --- | --- | --- |"]
            L += [f"| {r[0]} | {r[1]:.3f} | {r[2]:.2f} | {r[3]} | {r[4]:.2f} | {r[5]:.2f} |" for r in rows]
            L.append("")

    # ---- DP cost per image row ----
    if not dp.empty:
        L += ["## DP time per image row (only the DP parallel)", "",
              "rows = seams × (H − 1). 'extra' = (t_dp(omp) − t_dp(seq)) / rows: the time every row costs "
              "in addition to the sequential DP (positive = slower).", "",
              "| platform | size | threads | DP version | t_dp (s) | time per row (µs) | extra per row (µs) |",
              "| --- | --- | --- | --- | --- | --- | --- |"]
        for _, r in dp.iterrows():
            th = "–" if r["variant"] == "seq" else f"{r['threads']}{' (over)' if r['over'] == 1 else ''}"
            name = {"seq": "sequential", "A": "variant A", "B": "variant B"}[r["variant"]]
            L.append(f"| {r['platform']} | {r['size']} | {th} | {name} | {r['t_dp']:.3f} | "
                     f"{r['us_per_row']:.2f} | {r['extra_us']:.2f} |")
        L.append("")

    # ---- Windows vs Linux ----
    rows = platform_table(s)
    if rows:
        L += ["## Windows vs Linux (WSL2): same code, same laptop", "",
              "Speedups are computed inside each platform (T_seq of the same platform).", "",
              "| size | seams | threads | parallel stages | Windows median (s) | Windows speedup | "
              "Linux median (s) | Linux speedup | Windows data from |",
              "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
        L += [f"| {r[0]} | {r[1]} | {r[2]} | {r[3]} | {r[4]:.3f} | {r[5]:.2f} | {r[6]:.3f} | {r[7]:.2f} | {r[8]} |"
              for r in rows]
        L.append("")

    # ---- fixed part vs per-seam work (sequential) ----
    seq = df[(df["mode"] == "seq") & df["experiment"].isin(["main", "main_nodp"])].copy()
    if not seq.empty and seq["reduction"].nunique() >= 2:
        seq["work"] = seq_work(seq["in_w"], seq["in_h"], seq["seams"])
        groups = sorted(seq.groupby("size"), key=lambda kv: kv[1]["mpix"].iloc[0])
        L += ["## Fixed part vs per-seam work (sequential runs of main + main_nodp)", "",
              "Seam k is computed on an image of width W − k, so the cost per seam shrinks during a run; the "
              "work of a run is P = H·(s·W − s(s−1)/2) pixel visits. Both models are fitted to the median "
              "t_total of each reduction (10/20/30 %; medians, so one slow run cannot move the line). "
              "Model 1: t_total = a + c·P (a = fixed part). Model 2 (naive): t_total = a′ + b·s; because the "
              "per-seam cost shrinks, its intercept a′ also contains per-seam work. Last column: the fixed "
              "part measured directly, t_total − Σ stage times (copy of the input, gray conversion, final "
              "crop). An intercept is only an extrapolation to zero seams: a 1 % change of the per-pixel "
              "cost between the reductions moves it by about 1 % of the run time.", "",
              "| size | a, model 1 (s) | c, model 1 (ns per pixel visit) | a′, model 2 (s) | b, model 2 (ms per seam)"
              " | t_total − Σ stages, median (s) | runs |",
              "| --- | --- | --- | --- | --- | --- | --- |"]
        for z, g in groups:
            m = g.groupby("reduction").agg(work=("work", "first"), seams=("seams", "first"),
                                           t=("t_total", "median"))
            c, a = np.polyfit(m["work"] / 1e9, m["t"], 1)   # x in 1e9 pixel visits -> c in ns/visit
            b, a2 = np.polyfit(m["seams"], m["t"], 1)
            L.append(f"| {z} | {a:.4f} | {c:.3f} | {a2:.4f} | {1e3 * b:.3f} | "
                     f"{g['t_other'].median():.4f} | {len(g)} |")
        L += ["", "## Sequential time per pixel visit (ns), median over the same runs", "",
              "Working set = color (3 B) + gray (1 B) + energy (4 B) + M (4 B) = 12 bytes per pixel.", "",
              "| size | working set (MB) | energy | DP | removal | whole run |", "| --- | --- | --- | --- | --- | --- |"]
        for z, g in groups:
            per = [(g[c] / g["work"] * 1e9).median() for c in ("t_energy", "t_dp", "t_removal", "t_total")]
            mb = g["in_w"].iloc[0] * g["in_h"].iloc[0] * 12 / 1e6
            L.append(f"| {z} | {mb:.1f} | " + " | ".join(f"{x:.3f}" for x in per) + " |")
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
    # Default: all result files except the summary and the pilot runs (planning data, 1 run each).
    paths = [Path(p) for p in args] or sorted(
        p for p in (ROOT / "results").glob("*.csv") if p.name != "summary.csv" and not p.name.startswith("pilot"))
    df, stats = load(paths)
    s = summarize(df)
    dp = dp_rows(s)
    report(s, stats, df, dp)
    plot_main(s, "main", "all stages parallel")
    plot_main(s, "main_nodp", "DP sequential")
    plot_sizes(s)
    plot_breakdown(s)
    plot_schedule(s)
    plot_dp_rows(dp)


if __name__ == "__main__":
    main()
