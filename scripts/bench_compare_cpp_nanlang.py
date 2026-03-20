#!/usr/bin/env python3
"""Benchmark C++ vs NanLang core toolchain (without Go wrapper) with stats and charts.

Outputs:
  - benchmark_results/raw_samples.csv
  - benchmark_results/summary.json
  - benchmark_results/compile_means.(png|svg)
  - benchmark_results/runtime_(boxplot.png|means.svg)
"""

from __future__ import annotations

import argparse
import csv
import html
import json
import math
import os
import shutil
import statistics
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Dict, List, Sequence

HAS_MATPLOTLIB = False
try:
    # Headless rendering for CI/servers.
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except Exception:
    HAS_MATPLOTLIB = False


@dataclass
class Stats:
    n: int
    mean_ms: float
    median_ms: float
    stdev_ms: float
    min_ms: float
    max_ms: float
    p95_ms: float
    ci95_low_ms: float
    ci95_high_ms: float


def percentile(values: Sequence[float], q: float) -> float:
    if not values:
        return 0.0
    if len(values) == 1:
        return values[0]
    ordered = sorted(values)
    pos = (len(ordered) - 1) * q
    lo = math.floor(pos)
    hi = math.ceil(pos)
    if lo == hi:
        return ordered[lo]
    frac = pos - lo
    return ordered[lo] + (ordered[hi] - ordered[lo]) * frac


def compute_stats(values_sec: Sequence[float]) -> Stats:
    ms = [v * 1000.0 for v in values_sec]
    n = len(ms)
    if n == 0:
        return Stats(0, 0, 0, 0, 0, 0, 0, 0, 0)

    mean = statistics.mean(ms)
    median = statistics.median(ms)
    stdev = statistics.stdev(ms) if n > 1 else 0.0
    min_v = min(ms)
    max_v = max(ms)
    p95 = percentile(ms, 0.95)

    # Normal approximation CI (good enough for quick engineering comparisons).
    margin = 1.96 * (stdev / math.sqrt(n)) if n > 1 else 0.0
    return Stats(
        n=n,
        mean_ms=mean,
        median_ms=median,
        stdev_ms=stdev,
        min_ms=min_v,
        max_ms=max_v,
        p95_ms=p95,
        ci95_low_ms=mean - margin,
        ci95_high_ms=mean + margin,
    )


def run_timed(cmd: Sequence[str], cwd: Path, suppress_io: bool = True, env: dict | None = None) -> float:
    stdout = subprocess.DEVNULL if suppress_io else None
    stderr = subprocess.DEVNULL if suppress_io else None
    t0 = time.perf_counter()
    subprocess.run(
        cmd,
        cwd=str(cwd),
        check=True,
        stdout=stdout,
        stderr=stderr,
        env=env,
        close_fds=False,
    )
    t1 = time.perf_counter()
    return t1 - t0


def ensure_tool(name: str) -> None:
    if shutil.which(name) is None:
        raise RuntimeError(f"Required tool not found in PATH: {name}")


def resolve_local_or_path(repo_root: Path, local_rel: str, binary_name: str) -> Path:
    local = repo_root / local_rel
    if local.exists():
        return local
    system_path = shutil.which(binary_name)
    if system_path:
        return Path(system_path)
    raise RuntimeError(f"{binary_name} not found. Build first: make unified")


def write_nanlang_source(path: Path, workload_lines: int) -> None:
    lines = ["NAN3;"]
    for i in range(workload_lines):
        lines.append(f"let v{i} = {i % 256};")
    lines.append("halt;")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_cpp_source(path: Path, workload_lines: int) -> None:
    body: List[str] = [
        "#include <cstdint>",
        "int main() {",
        "    volatile uint64_t sink = 0;",
    ]
    for i in range(workload_lines):
        body.append(f"    sink += static_cast<uint64_t>({i}u) ^ 0x9E3779B97F4A7C15ull;")
    body.extend(
        [
            "    (void)sink;",
            "    return 0;",
            "}",
        ]
    )
    path.write_text("\n".join(body) + "\n", encoding="utf-8")


def format_row(name: str, st: Stats) -> str:
    return (
        f"{name:<18} n={st.n:<3} "
        f"mean={st.mean_ms:8.3f} ms  median={st.median_ms:8.3f} ms  "
        f"p95={st.p95_ms:8.3f} ms  stdev={st.stdev_ms:8.3f} ms"
    )


def plot_compile_means(output: Path, stats_map: Dict[str, Stats]) -> None:
    labels = list(stats_map.keys())
    means = [stats_map[k].mean_ms for k in labels]
    errs = [stats_map[k].stdev_ms for k in labels]

    fig, ax = plt.subplots(figsize=(10, 5.5))
    bars = ax.bar(labels, means, yerr=errs, capsize=6)
    ax.set_title("Compile Time Comparison (mean ± stdev)")
    ax.set_ylabel("Milliseconds")
    ax.grid(axis="y", linestyle="--", alpha=0.35)

    for bar, mean in zip(bars, means):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height(), f"{mean:.1f}",
                ha="center", va="bottom", fontsize=9)

    fig.tight_layout()
    fig.savefig(output, dpi=160)
    plt.close(fig)


def plot_runtime_boxplot(output: Path, samples_map: Dict[str, Sequence[float]]) -> None:
    labels = list(samples_map.keys())
    ms_data = [[x * 1000.0 for x in samples_map[k]] for k in labels]

    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.boxplot(ms_data, tick_labels=labels, showmeans=True)
    ax.set_title("Runtime Distribution Comparison")
    ax.set_ylabel("Milliseconds")
    ax.grid(axis="y", linestyle="--", alpha=0.35)

    fig.tight_layout()
    fig.savefig(output, dpi=160)
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Benchmark C++ vs NanLang with charts and stats")
    parser.add_argument("--iterations", type=int, default=20, help="Measured repetitions per command")
    parser.add_argument("--warmup", type=int, default=3, help="Warmup repetitions per command")
    parser.add_argument("--workload-lines", type=int, default=4000, help="Number of synthetic source lines")
    parser.add_argument("--output-dir", default="benchmark_results", help="Result directory")
    parser.add_argument("--keep-workdir", action="store_true", help="Keep generated benchmark sources")
    parser.add_argument("--verbose", action="store_true", help="Print command outputs")
    return parser.parse_args()


def write_simple_bar_svg(output: Path, title: str, stats_map: Dict[str, Stats]) -> None:
    labels = list(stats_map.keys())
    means = [stats_map[k].mean_ms for k in labels]
    max_mean = max(means) if means else 1.0
    max_mean = max(max_mean, 1.0)

    width = 1000
    height = 520
    left = 90
    top = 70
    chart_w = 860
    chart_h = 360
    bar_w = int(chart_w / max(len(labels), 1) * 0.55)
    gap = int(chart_w / max(len(labels), 1))

    svg_lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{width // 2}" y="36" text-anchor="middle" font-size="24" font-family="Arial">{title}</text>',
        f'<line x1="{left}" y1="{top + chart_h}" x2="{left + chart_w}" y2="{top + chart_h}" stroke="black"/>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + chart_h}" stroke="black"/>',
    ]

    for i in range(6):
        y = top + chart_h - int(chart_h * i / 5)
        val = max_mean * i / 5
        svg_lines.append(
            f'<line x1="{left - 6}" y1="{y}" x2="{left + chart_w}" y2="{y}" stroke="#dddddd"/>'
        )
        svg_lines.append(
            f'<text x="{left - 12}" y="{y + 4}" text-anchor="end" font-size="12" font-family="Arial">{val:.1f}</text>'
        )

    for idx, (label, mean) in enumerate(zip(labels, means)):
        x = left + int(gap * idx + (gap - bar_w) / 2)
        bar_h = int((mean / max_mean) * chart_h)
        y = top + chart_h - bar_h
        svg_lines.append(f'<rect x="{x}" y="{y}" width="{bar_w}" height="{bar_h}" fill="#4C78A8"/>')
        svg_lines.append(
            f'<text x="{x + bar_w / 2:.1f}" y="{top + chart_h + 20}" text-anchor="middle" font-size="12" font-family="Arial">{label}</text>'
        )
        svg_lines.append(
            f'<text x="{x + bar_w / 2:.1f}" y="{y - 6}" text-anchor="middle" font-size="12" font-family="Arial">{mean:.2f} ms</text>'
        )

    svg_lines.append("</svg>")
    output.write_text("\n".join(svg_lines), encoding="utf-8")


def write_html_report(output: Path, summary: dict, rows: List[List[str]]) -> None:
    meta = summary.get("meta", {})
    compile_stats = summary.get("compile", {})
    runtime_stats = summary.get("runtime", {})

    def render_stats_cards(phase_name: str, stats_obj: Dict[str, dict]) -> str:
        cards = []
        for tool, st in stats_obj.items():
            cards.append(
                f"""
                <article class="card">
                  <h3>{html.escape(tool)}</h3>
                  <p><strong>Mean:</strong> {st.get('mean_ms', 0):.3f} ms</p>
                  <p><strong>Median:</strong> {st.get('median_ms', 0):.3f} ms</p>
                  <p><strong>P95:</strong> {st.get('p95_ms', 0):.3f} ms</p>
                  <p><strong>StdDev:</strong> {st.get('stdev_ms', 0):.3f} ms</p>
                </article>
                """
            )
        return f"""
        <section>
          <h2>{html.escape(phase_name)}</h2>
          <div class="cards">
            {''.join(cards)}
          </div>
        </section>
        """

    table_rows = []
    for phase, tool, iteration, seconds, milliseconds in rows:
        table_rows.append(
            "<tr>"
            f"<td>{html.escape(phase)}</td>"
            f"<td>{html.escape(tool)}</td>"
            f"<td class='num'>{html.escape(str(iteration))}</td>"
            f"<td class='num'>{html.escape(str(seconds))}</td>"
            f"<td class='num'>{html.escape(str(milliseconds))}</td>"
            "</tr>"
        )

    html_doc = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>NanLang Benchmark Report</title>
  <style>
    :root {{
      --bg: #f4f7fb;
      --panel: #ffffff;
      --text: #17212f;
      --muted: #52627a;
      --line: #dde5ef;
      --accent: #0b7285;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      font-family: "Segoe UI", Tahoma, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
    }}
    .wrap {{
      max-width: 1200px;
      margin: 0 auto;
      padding: 24px;
    }}
    .hero {{
      background: linear-gradient(120deg, #e6f4ff, #eefaf2);
      border: 1px solid var(--line);
      border-radius: 14px;
      padding: 18px 20px;
      margin-bottom: 18px;
    }}
    h1, h2, h3 {{ margin: 0 0 10px; }}
    h1 {{ font-size: 1.5rem; }}
    h2 {{ font-size: 1.1rem; margin-top: 12px; }}
    .meta {{
      color: var(--muted);
      font-size: 0.95rem;
      display: flex;
      flex-wrap: wrap;
      gap: 16px;
    }}
    .cards {{
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
      gap: 12px;
      margin-bottom: 18px;
    }}
    .card {{
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 12px;
      padding: 12px;
      box-shadow: 0 2px 8px rgba(9, 25, 46, 0.04);
    }}
    .card p {{
      margin: 6px 0;
      color: var(--muted);
      font-size: 0.92rem;
    }}
    .table-wrap {{
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 12px;
      overflow: auto;
    }}
    table {{
      width: 100%;
      border-collapse: collapse;
      min-width: 780px;
    }}
    thead th {{
      position: sticky;
      top: 0;
      background: #f8fbff;
      color: #224;
      text-align: left;
      font-weight: 600;
      border-bottom: 1px solid var(--line);
      padding: 10px;
    }}
    tbody td {{
      border-bottom: 1px solid var(--line);
      padding: 8px 10px;
      font-size: 0.92rem;
    }}
    tbody tr:nth-child(even) {{ background: #fbfdff; }}
    .num {{ text-align: right; font-variant-numeric: tabular-nums; }}
    .footer {{
      margin-top: 14px;
      color: var(--muted);
      font-size: 0.85rem;
    }}
    .accent {{ color: var(--accent); font-weight: 600; }}
  </style>
</head>
<body>
  <div class="wrap">
    <section class="hero">
      <h1>NanLang Benchmark Report</h1>
      <div class="meta">
        <span><span class="accent">Iterations:</span> {meta.get('iterations', '-')}</span>
        <span><span class="accent">Warmup:</span> {meta.get('warmup', '-')}</span>
        <span><span class="accent">Workload Lines:</span> {meta.get('workload_lines', '-')}</span>
      </div>
    </section>
    {render_stats_cards("Compile Stats", compile_stats)}
    {render_stats_cards("Runtime Stats", runtime_stats)}
    <section>
      <h2>Raw Samples (CSV -> HTML)</h2>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Phase</th>
              <th>Tool</th>
              <th class="num">Iteration</th>
              <th class="num">Seconds</th>
              <th class="num">Milliseconds</th>
            </tr>
          </thead>
          <tbody>
            {''.join(table_rows)}
          </tbody>
        </table>
      </div>
      <p class="footer">Generated from <code>benchmark_results/raw_samples.csv</code> by <code>bench_compare_cpp_nanlang.py</code>.</p>
    </section>
  </div>
</body>
</html>
"""
    output.write_text(html_doc, encoding="utf-8")


def main() -> int:
    args = parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    out_dir = Path(args.output_dir)
    if not out_dir.is_absolute():
        out_dir = repo_root / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    work_dir = out_dir / "generated"
    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)

    ensure_tool("g++")
    ensure_tool("raku")
    nanlang_cpp_bin = resolve_local_or_path(repo_root, "bin/nanlang_cpp", "nanlang_cpp")
    nanlang_cc_bin = resolve_local_or_path(repo_root, "bin/nanlang_cc", "nanlang_cc")

    preprocessor = repo_root / "scripts" / "nan_preprocessor.raku"
    if not preprocessor.exists():
        raise RuntimeError(f"Preprocessor not found: {preprocessor}")

    bench_env = dict(os.environ)
    bench_env["NANLANG_FAST_CACHE"] = "1"
    bench_env["NANLANG_CACHE_LOG"] = "0"

    cpp_src = work_dir / "bench_cpp.cpp"
    cpp_bin = work_dir / "bench_cpp"
    nl_src = work_dir / "bench_nanlang.nl"
    nl_pre = work_dir / "bench_nanlang.pre.nl"
    elf_out = work_dir / "bench_nanlang.elf"

    write_cpp_source(cpp_src, args.workload_lines)
    write_nanlang_source(nl_src, args.workload_lines)
    with nl_pre.open("w", encoding="utf-8") as pre_out:
        subprocess.run(
            ["raku", str(preprocessor), str(nl_src)],
            cwd=str(repo_root),
            check=True,
            stdout=pre_out,
            stderr=None if args.verbose else subprocess.DEVNULL,
        )

    compile_cmds = {
        "C++ (g++ -O3)": ["g++", "-std=c++17", "-O3", "-DNDEBUG", str(cpp_src), "-o", str(cpp_bin)],
        "NanLang Core -> .elf": [
            str(nanlang_cc_bin),
            "--compile",
            str(nl_pre),
            "--elf",
            "--output",
            str(elf_out),
        ],
    }

    runtime_prepare = [
        compile_cmds["C++ (g++ -O3)"],
        compile_cmds["NanLang Core -> .elf"],
    ]

    for cmd in runtime_prepare:
        subprocess.run(cmd, cwd=str(repo_root), check=True,
                       stdout=None if args.verbose else subprocess.DEVNULL,
                       stderr=None if args.verbose else subprocess.DEVNULL,
                       env=bench_env,
                       close_fds=False)

    runtime_cmds = {
        "C++ native run": [str(cpp_bin)],
        "NanLang ELF run": [str(elf_out)],
    }

    compile_samples: Dict[str, List[float]] = {k: [] for k in compile_cmds}
    runtime_samples: Dict[str, List[float]] = {k: [] for k in runtime_cmds}

    for _ in range(args.warmup):
        for name, cmd in compile_cmds.items():
            run_timed(cmd, repo_root, suppress_io=not args.verbose, env=bench_env)
        for name, cmd in runtime_cmds.items():
            run_timed(cmd, repo_root, suppress_io=not args.verbose, env=bench_env)

    for _ in range(args.iterations):
        for name, cmd in compile_cmds.items():
            compile_samples[name].append(run_timed(cmd, repo_root, suppress_io=not args.verbose, env=bench_env))
        for name, cmd in runtime_cmds.items():
            runtime_samples[name].append(run_timed(cmd, repo_root, suppress_io=not args.verbose, env=bench_env))

    compile_stats = {name: compute_stats(vals) for name, vals in compile_samples.items()}
    runtime_stats = {name: compute_stats(vals) for name, vals in runtime_samples.items()}

    raw_csv = out_dir / "raw_samples.csv"
    raw_rows: List[List[str]] = []
    with raw_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["phase", "tool", "iteration", "seconds", "milliseconds"])
        for tool, vals in compile_samples.items():
            for i, sec in enumerate(vals, start=1):
                row = ["compile", tool, i, f"{sec:.9f}", f"{sec * 1000.0:.6f}"]
                writer.writerow(row)
                raw_rows.append([str(x) for x in row])
        for tool, vals in runtime_samples.items():
            for i, sec in enumerate(vals, start=1):
                row = ["runtime", tool, i, f"{sec:.9f}", f"{sec * 1000.0:.6f}"]
                writer.writerow(row)
                raw_rows.append([str(x) for x in row])

    summary = {
        "meta": {
            "iterations": args.iterations,
            "warmup": args.warmup,
            "workload_lines": args.workload_lines,
            "repo_root": str(repo_root),
            "nanlang_cpp_bin": str(nanlang_cpp_bin),
            "nanlang_cc_bin": str(nanlang_cc_bin),
            "preprocessor": str(preprocessor),
            "nanlang_driver": "cpp-core",
            "generated_at_unix": time.time(),
        },
        "compile": {name: asdict(st) for name, st in compile_stats.items()},
        "runtime": {name: asdict(st) for name, st in runtime_stats.items()},
    }

    summary_json = out_dir / "summary.json"
    summary_json.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    raw_html = out_dir / "raw_samples.html"
    write_html_report(raw_html, summary, raw_rows)

    if HAS_MATPLOTLIB:
        compile_chart = out_dir / "compile_means.png"
        runtime_chart = out_dir / "runtime_boxplot.png"
        plot_compile_means(compile_chart, compile_stats)
        plot_runtime_boxplot(runtime_chart, runtime_samples)
    else:
        compile_chart = out_dir / "compile_means.svg"
        runtime_chart = out_dir / "runtime_means.svg"
        write_simple_bar_svg(compile_chart, "Compile Time Comparison (mean ms)", compile_stats)
        write_simple_bar_svg(runtime_chart, "Runtime Time Comparison (mean ms)", runtime_stats)

    print("\n=== Compile Benchmark ===")
    for name, st in compile_stats.items():
        print(format_row(name, st))

    print("\n=== Runtime Benchmark ===")
    for name, st in runtime_stats.items():
        print(format_row(name, st))

    print("\nOutputs:")
    print(f"  - {raw_csv}")
    print(f"  - {raw_html}")
    print(f"  - {summary_json}")
    print(f"  - {compile_chart}")
    print(f"  - {runtime_chart}")

    if not args.keep_workdir:
        shutil.rmtree(work_dir, ignore_errors=True)
    else:
        print(f"  - {work_dir} (kept)")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"Benchmark command failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)
