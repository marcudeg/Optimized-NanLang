#!/usr/bin/env python3
"""
Comprehensive benchmark: Go vs C++ vs NanLang vs Python
Compares compile time and runtime performance across all languages.

Outputs:
  - benchmark_results/all_langs_compile.png
  - benchmark_results/all_langs_runtime.png
  - benchmark_results/all_langs_summary.json
  - benchmark_results/all_langs_raw.csv
"""

import argparse
import csv
import json
import math
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Dict, List, Tuple

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except Exception:
    HAS_MATPLOTLIB = False


@dataclass
class BenchResult:
    language: str
    compile_times: List[float]  # seconds
    runtime_times: List[float]  # seconds
    
    @property
    def compile_mean(self) -> float:
        return statistics.mean(self.compile_times) if self.compile_times else 0.0
    
    @property
    def compile_stdev(self) -> float:
        return statistics.stdev(self.compile_times) if len(self.compile_times) > 1 else 0.0
    
    @property
    def runtime_mean(self) -> float:
        return statistics.mean(self.runtime_times) if self.runtime_times else 0.0
    
    @property
    def runtime_stdev(self) -> float:
        return statistics.stdev(self.runtime_times) if len(self.runtime_times) > 1 else 0.0


def run_timed(cmd: List[str], cwd: Path, suppress_io: bool = True, timeout: int = 30) -> Tuple[float, bool]:
    """Run command and return elapsed time and success status."""
    try:
        t0 = time.perf_counter()
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            stdout=subprocess.DEVNULL if suppress_io else None,
            stderr=subprocess.DEVNULL if suppress_io else None,
            timeout=timeout,
            check=False
        )
        t1 = time.perf_counter()
        return t1 - t0, result.returncode == 0
    except subprocess.TimeoutExpired:
        return timeout, False
    except Exception as e:
        print(f"Error running {cmd}: {e}")
        return -1, False


def generate_go_source(path: Path, workload_lines: int) -> None:
    """Generate a simple Go benchmark program."""
    code = f"""package main

import (
    "fmt"
)

func main() {{
    var sum uint64 = 0
"""
    for i in range(workload_lines):
        code += f"    sum += uint64({i % 256})\n"
    code += """    fmt.Println(sum)
}
"""
    path.write_text(code)


def generate_cpp_source(path: Path, workload_lines: int) -> None:
    """Generate a simple C++ benchmark program."""
    code = """\
#include <cstdint>
#include <iostream>

int main() {
    volatile uint64_t sum = 0;
"""
    for i in range(workload_lines):
        code += f"    sum += {i % 256};\n"
    code += """    (void)sum;
    return 0;
}
"""
    path.write_text(code)


def generate_nanlang_source(path: Path, workload_lines: int) -> None:
    """Generate a NanLang benchmark program."""
    lines = ["NAN3;"]
    for i in range(workload_lines):
        lines.append(f"let v{i} = {i % 256};")
    lines.append("halt;")
    path.write_text("\n".join(lines) + "\n")


def generate_python_source(path: Path, workload_lines: int) -> None:
    """Generate a Python benchmark program."""
    code = "sum_val = 0\n"
    for i in range(workload_lines):
        code += f"sum_val += {i % 256}\n"
    code += "print(sum_val)\n"
    path.write_text(code)


def benchmark_go(repo_root: Path, iterations: int, warmup: int) -> BenchResult:
    """Benchmark Go compilation and runtime."""
    result = BenchResult(language="Go", compile_times=[], runtime_times=[])
    
    if not shutil.which("go"):
        print("⚠️  Go not found, skipping")
        return result
    
    with tempfile.TemporaryDirectory() as tmpdir:
        tmppath = Path(tmpdir)
        src = tmppath / "bench.go"
        exe = tmppath / "bench"
        
        # Warmup
        for _ in range(warmup):
            generate_go_source(src, 500)
            run_timed(["go", "build", "-o", str(exe), str(src)], tmppath, suppress_io=True, timeout=30)
        
        # Actual benchmark
        for i in range(iterations):
            generate_go_source(src, 500 + i * 10)
            
            # Compile time
            t_compile, success = run_timed(["go", "build", "-o", str(exe), str(src)], tmppath, suppress_io=True, timeout=30)
            if success:
                result.compile_times.append(t_compile)
            
            # Runtime
            if exe.exists():
                t_runtime, success = run_timed([str(exe)], tmppath, suppress_io=True, timeout=10)
                if success:
                    result.runtime_times.append(t_runtime)
    
    return result


def benchmark_cpp(repo_root: Path, iterations: int, warmup: int) -> BenchResult:
    """Benchmark C++ compilation and runtime."""
    result = BenchResult(language="C++", compile_times=[], runtime_times=[])
    
    if not shutil.which("g++"):
        print("⚠️  g++ not found, skipping")
        return result
    
    with tempfile.TemporaryDirectory() as tmpdir:
        tmppath = Path(tmpdir)
        src = tmppath / "bench.cpp"
        exe = tmppath / "bench"
        
        # Warmup
        for _ in range(warmup):
            generate_cpp_source(src, 500)
            run_timed(["g++", "-O3", "-o", str(exe), str(src)], tmppath, suppress_io=True, timeout=30)
        
        # Actual benchmark
        for i in range(iterations):
            generate_cpp_source(src, 500 + i * 10)
            
            # Compile time
            t_compile, success = run_timed(["g++", "-O3", "-o", str(exe), str(src)], tmppath, suppress_io=True, timeout=30)
            if success:
                result.compile_times.append(t_compile)
            
            # Runtime
            if exe.exists():
                t_runtime, success = run_timed([str(exe)], tmppath, suppress_io=True, timeout=10)
                if success:
                    result.runtime_times.append(t_runtime)
    
    return result


def benchmark_nanlang(repo_root: Path, iterations: int, warmup: int) -> BenchResult:
    """Benchmark NanLang compilation and runtime."""
    result = BenchResult(language="NanLang", compile_times=[], runtime_times=[])
    
    nanlang_bin = repo_root / "bin" / "nanlang_cpp"
    if not nanlang_bin.exists():
        nanlang_bin = Path(shutil.which("nanlang_cpp") or "")
    
    if not nanlang_bin.exists():
        print("⚠️  nanlang_cpp not found, skipping")
        return result
    
    with tempfile.TemporaryDirectory() as tmpdir:
        tmppath = Path(tmpdir)
        src = tmppath / "bench.nl"
        exe = tmppath / "bench.nb"
        
        # Warmup
        for _ in range(warmup):
            generate_nanlang_source(src, 500)
            run_timed([str(nanlang_bin), "compile", str(src), "--output", str(exe)], tmppath, suppress_io=True, timeout=30)
        
        # Actual benchmark
        for i in range(iterations):
            generate_nanlang_source(src, 500 + i * 10)
            
            # Compile time
            t_compile, success = run_timed([str(nanlang_bin), "compile", str(src), "--output", str(exe)], tmppath, suppress_io=True, timeout=30)
            if success:
                result.compile_times.append(t_compile)
            
            # Runtime
            if exe.exists():
                t_runtime, success = run_timed([str(nanlang_bin), "run", str(exe)], tmppath, suppress_io=True, timeout=10)
                if success:
                    result.runtime_times.append(t_runtime)
    
    return result


def benchmark_python(repo_root: Path, iterations: int, warmup: int) -> BenchResult:
    """Benchmark Python (no compile time, just runtime)."""
    result = BenchResult(language="Python", compile_times=[], runtime_times=[])
    
    if not shutil.which("python3"):
        print("⚠️  python3 not found, skipping")
        return result
    
    with tempfile.TemporaryDirectory() as tmpdir:
        tmppath = Path(tmpdir)
        src = tmppath / "bench.py"
        
        # Warmup (Python doesn't need separate compile)
        for _ in range(warmup):
            generate_python_source(src, 500)
            run_timed(["python3", str(src)], tmppath, suppress_io=True, timeout=10)
        
        # Actual benchmark
        for i in range(iterations):
            generate_python_source(src, 500 + i * 10)
            
            # No compile time for Python (set to 0)
            result.compile_times.append(0.0)
            
            # Runtime
            t_runtime, success = run_timed(["python3", str(src)], tmppath, suppress_io=True, timeout=10)
            if success:
                result.runtime_times.append(t_runtime)
    
    return result


def plot_compile_comparison(output: Path, results: List[BenchResult]) -> None:
    """Generate compile time comparison chart."""
    if not HAS_MATPLOTLIB:
        print("⚠️  matplotlib not available, skipping chart generation")
        return
    
    labels = []
    means = []
    stdevs = []
    
    for r in results:
        if r.compile_times:
            labels.append(r.language)
            means.append(r.compile_mean * 1000)  # Convert to ms
            stdevs.append(r.compile_stdev * 1000)
    
    if not labels:
        return
    
    fig, ax = plt.subplots(figsize=(12, 6))
    bars = ax.bar(labels, means, yerr=stdevs, capsize=8, alpha=0.8, color=["#3498db", "#e74c3c", "#2ecc71", "#f39c12"])
    
    ax.set_title("Compile Time Comparison (mean ± stdev)", fontsize=14, fontweight="bold")
    ax.set_ylabel("Milliseconds", fontsize=12)
    ax.set_ylim(bottom=0)
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    
    for bar, mean in zip(bars, means):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2, height, f"{mean:.2f} ms",
                ha="center", va="bottom", fontsize=10, fontweight="bold")
    
    fig.tight_layout()
    fig.savefig(output, dpi=150)
    plt.close(fig)
    print(f"✅ Saved compile chart: {output}")


def plot_runtime_comparison(output: Path, results: List[BenchResult]) -> None:
    """Generate runtime comparison chart."""
    if not HAS_MATPLOTLIB:
        print("⚠️  matplotlib not available, skipping chart generation")
        return
    
    labels = []
    means = []
    stdevs = []
    
    for r in results:
        if r.runtime_times:
            labels.append(r.language)
            means.append(r.runtime_mean * 1000)  # Convert to ms
            stdevs.append(r.runtime_stdev * 1000)
    
    if not labels:
        return
    
    fig, ax = plt.subplots(figsize=(12, 6))
    bars = ax.bar(labels, means, yerr=stdevs, capsize=8, alpha=0.8, color=["#3498db", "#e74c3c", "#2ecc71", "#f39c12"])
    
    ax.set_title("Runtime Comparison (mean ± stdev)", fontsize=14, fontweight="bold")
    ax.set_ylabel("Milliseconds", fontsize=12)
    ax.set_ylim(bottom=0)
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    
    for bar, mean in zip(bars, means):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width() / 2, height, f"{mean:.4f} ms",
                ha="center", va="bottom", fontsize=10, fontweight="bold")
    
    fig.tight_layout()
    fig.savefig(output, dpi=150)
    plt.close(fig)
    print(f"✅ Saved runtime chart: {output}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Benchmark Go vs C++ vs NanLang vs Python")
    parser.add_argument("--iterations", type=int, default=15, help="Measured iterations per language")
    parser.add_argument("--warmup", type=int, default=2, help="Warmup iterations")
    parser.add_argument("--repo", type=Path, default=Path.cwd(), help="Repository root")
    
    args = parser.parse_args()
    
    print(f"🚀 NanLang Multi-Language Benchmark")
    print(f"   Iterations: {args.iterations}, Warmup: {args.warmup}")
    print()
    
    # Create output directory
    out_dir = args.repo / "benchmark_results"
    out_dir.mkdir(exist_ok=True)
    
    # Run benchmarks
    print("📊 Running benchmarks...")
    results = []
    
    print("  • Go...")
    results.append(benchmark_go(args.repo, args.iterations, args.warmup))
    
    print("  • C++...")
    results.append(benchmark_cpp(args.repo, args.iterations, args.warmup))
    
    print("  • NanLang...")
    results.append(benchmark_nanlang(args.repo, args.iterations, args.warmup))
    
    print("  • Python...")
    results.append(benchmark_python(args.repo, args.iterations, args.warmup))
    
    # Print results
    print("\n📈 RESULTS:")
    print("-" * 90)
    print(f"{'Language':<12} {'Compile (ms)':<20} {'Runtime (ms)':<20}")
    print("-" * 90)
    
    for r in results:
        compile_str = f"{r.compile_mean*1000:.3f} ± {r.compile_stdev*1000:.3f}" if r.compile_times else "N/A"
        runtime_str = f"{r.runtime_mean*1000:.4f} ± {r.runtime_stdev*1000:.4f}" if r.runtime_times else "N/A"
        print(f"{r.language:<12} {compile_str:<20} {runtime_str:<20}")
    
    # Save CSV
    csv_path = out_dir / "all_langs_raw.csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["language", "compile_ms", "runtime_ms"])
        for r in results:
            for ct, rt in zip([t * 1000 for t in r.compile_times], [t * 1000 for t in r.runtime_times]):
                writer.writerow([r.language, f"{ct:.3f}", f"{rt:.4f}"])
    
    print(f"\n💾 Saved CSV: {csv_path}")
    
    # Save JSON summary
    json_path = out_dir / "all_langs_summary.json"
    summary = {
        r.language: {
            "compile_mean_ms": r.compile_mean * 1000,
            "compile_stdev_ms": r.compile_stdev * 1000,
            "runtime_mean_ms": r.runtime_mean * 1000,
            "runtime_stdev_ms": r.runtime_stdev * 1000,
            "compile_samples": len(r.compile_times),
            "runtime_samples": len(r.runtime_times),
        }
        for r in results
    }
    
    with open(json_path, "w") as f:
        json.dump(summary, f, indent=2)
    
    print(f"💾 Saved JSON: {json_path}")
    
    # Generate charts
    print("\n🎨 Generating charts...")
    plot_compile_comparison(out_dir / "all_langs_compile.png", results)
    plot_runtime_comparison(out_dir / "all_langs_runtime.png", results)
    
    print("\n✅ Benchmark complete!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
