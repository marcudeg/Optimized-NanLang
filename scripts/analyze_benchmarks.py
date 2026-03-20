#!/usr/bin/env python3
"""
Advanced benchmark analysis and visualization for multi-language comparison.
Generates detailed charts, comparisons, and performance metrics.
"""

import json
import csv
from pathlib import Path
import statistics
import sys

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except Exception:
    HAS_MATPLOTLIB = False


def load_summary(path: Path) -> dict:
    """Load benchmark summary JSON."""
    with open(path) as f:
        return json.load(f)


def load_raw_data(path: Path) -> dict:
    """Load raw CSV data."""
    data = {}
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            lang = row['language']
            if lang not in data:
                data[lang] = {'compile': [], 'runtime': []}
            if row['compile_ms'] != 'N/A':
                data[lang]['compile'].append(float(row['compile_ms']))
            if row['runtime_ms'] != 'N/A':
                data[lang]['runtime'].append(float(row['runtime_ms']))
    return data


def print_summary_table(summary: dict) -> None:
    """Print formatted summary table."""
    print("\n" + "="*100)
    print("BENCHMARK SUMMARY - All Languages Performance Comparison".center(100))
    print("="*100)
    print()
    print(f"{'Language':<12} | {'Compile (ms)':<25} | {'Runtime (ms)':<25} | {'Speedup vs NanLang':<20}")
    print("-"*100)
    
    # Get baseline (NanLang)
    nanlang_compile = summary.get('NanLang', {}).get('compile_mean_ms', 1)
    nanlang_runtime = summary.get('NanLang', {}).get('runtime_mean_ms', 1)
    
    for lang in ['Go', 'C++', 'NanLang', 'Python']:
        if lang not in summary:
            continue
        
        data = summary[lang]
        compile_mean = data.get('compile_mean_ms', 0)
        compile_std = data.get('compile_stdev_ms', 0)
        runtime_mean = data.get('runtime_mean_ms', 0)
        runtime_std = data.get('runtime_stdev_ms', 0)
        
        compile_str = f"{compile_mean:.2f} ± {compile_std:.2f}" if compile_mean > 0 else "—"
        runtime_str = f"{runtime_mean:.4f} ± {runtime_std:.4f}"
        
        # Speedup calculation
        if compile_mean > 0:
            compile_speedup = compile_mean / nanlang_compile
            runtime_speedup = nanlang_runtime / runtime_mean
            speedup_str = f"Compile: {compile_speedup:.1f}x | Runtime: {runtime_speedup:.2f}x"
        else:
            speedup_str = "—"
        
        print(f"{lang:<12} | {compile_str:<25} | {runtime_str:<25} | {speedup_str:<20}")
    
    print("="*100)
    print()


def create_comparison_chart(summary: dict, output_path: Path) -> None:
    """Create side-by-side comparison chart."""
    if not HAS_MATPLOTLIB:
        return
    
    languages = []
    compile_means = []
    compile_stds = []
    runtime_means = []
    runtime_stds = []
    
    for lang in ['Go', 'C++', 'NanLang', 'Python']:
        if lang in summary:
            languages.append(lang)
            data = summary[lang]
            compile_means.append(data.get('compile_mean_ms', 0))
            compile_stds.append(data.get('compile_stdev_ms', 0))
            runtime_means.append(data.get('runtime_mean_ms', 0))
            runtime_stds.append(data.get('runtime_stdev_ms', 0))
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
    
    # Compile time chart
    x_pos = np.arange(len(languages))
    colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12']
    
    ax1.bar(x_pos, compile_means, yerr=compile_stds, capsize=8, color=colors[:len(languages)], alpha=0.8)
    ax1.set_xlabel('Language', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Time (ms)', fontsize=12, fontweight='bold')
    ax1.set_title('Compilation Time Comparison', fontsize=13, fontweight='bold')
    ax1.set_xticks(x_pos)
    ax1.set_xticklabels(languages)
    ax1.grid(axis='y', linestyle='--', alpha=0.4)
    
    for i, (mean, std) in enumerate(zip(compile_means, compile_stds)):
        if mean > 0:
            ax1.text(i, mean + std, f'{mean:.1f}', ha='center', va='bottom', fontweight='bold')
    
    # Runtime chart
    ax2.bar(x_pos, runtime_means, yerr=runtime_stds, capsize=8, color=colors[:len(languages)], alpha=0.8)
    ax2.set_xlabel('Language', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Time (ms)', fontsize=12, fontweight='bold')
    ax2.set_title('Runtime Comparison', fontsize=13, fontweight='bold')
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels(languages)
    ax2.grid(axis='y', linestyle='--', alpha=0.4)
    
    for i, (mean, std) in enumerate(zip(runtime_means, runtime_stds)):
        ax2.text(i, mean + std, f'{mean:.4f}', ha='center', va='bottom', fontweight='bold', fontsize=9)
    
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"✅ Saved comparison chart: {output_path}")


def create_speedup_chart(summary: dict, output_path: Path) -> None:
    """Create speedup relative to NanLang."""
    if not HAS_MATPLOTLIB:
        return
    
    nanlang_compile = summary.get('NanLang', {}).get('compile_mean_ms', 1)
    nanlang_runtime = summary.get('NanLang', {}).get('runtime_mean_ms', 1)
    
    languages = []
    compile_speedups = []
    runtime_speedups = []
    
    for lang in ['Go', 'C++', 'NanLang', 'Python']:
        if lang in summary:
            languages.append(lang)
            data = summary[lang]
            compile_mean = data.get('compile_mean_ms', 0)
            runtime_mean = data.get('runtime_mean_ms', 1)
            
            if compile_mean > 0:
                compile_speedups.append(compile_mean / nanlang_compile)
            else:
                compile_speedups.append(0)
            
            runtime_speedups.append(nanlang_runtime / runtime_mean if runtime_mean > 0 else 0)
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    x_pos = np.arange(len(languages))
    colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12']
    
    # Compile speedup
    bars1 = ax1.bar(x_pos, compile_speedups, color=colors[:len(languages)], alpha=0.8)
    ax1.axhline(y=1, color='red', linestyle='--', linewidth=2, label='NanLang baseline')
    ax1.set_xlabel('Language', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Speedup Factor', fontsize=12, fontweight='bold')
    ax1.set_title('Compilation Speedup vs NanLang', fontsize=13, fontweight='bold')
    ax1.set_xticks(x_pos)
    ax1.set_xticklabels(languages)
    ax1.grid(axis='y', linestyle='--', alpha=0.4)
    ax1.legend()
    
    for i, speedup in enumerate(compile_speedups):
        if speedup > 0:
            label = f'{speedup:.1f}x' if speedup != 1.0 else 'baseline'
            ax1.text(i, speedup, label, ha='center', va='bottom', fontweight='bold')
    
    # Runtime speedup
    bars2 = ax2.bar(x_pos, runtime_speedups, color=colors[:len(languages)], alpha=0.8)
    ax2.axhline(y=1, color='red', linestyle='--', linewidth=2, label='NanLang baseline')
    ax2.set_xlabel('Language', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Speedup Factor', fontsize=12, fontweight='bold')
    ax2.set_title('Runtime Speedup vs NanLang', fontsize=13, fontweight='bold')
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels(languages)
    ax2.grid(axis='y', linestyle='--', alpha=0.4)
    ax2.legend()
    
    for i, speedup in enumerate(runtime_speedups):
        label = f'{speedup:.2f}x' if speedup != 1.0 else 'baseline'
        ax2.text(i, speedup, label, ha='center', va='bottom', fontweight='bold')
    
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"✅ Saved speedup chart: {output_path}")


def create_violin_chart(raw_data: dict, output_path: Path) -> None:
    """Create violin plot of runtime distributions."""
    if not HAS_MATPLOTLIB:
        return
    
    fig, ax = plt.subplots(figsize=(12, 6))
    
    data_to_plot = []
    labels = []
    
    for lang in ['Go', 'C++', 'NanLang', 'Python']:
        if lang in raw_data and raw_data[lang]['runtime']:
            data_to_plot.append(raw_data[lang]['runtime'])
            labels.append(lang)
    
    if data_to_plot:
        parts = ax.violinplot(data_to_plot, positions=range(len(labels)), showmeans=True, showmedians=True)
        
        ax.set_xlabel('Language', fontsize=12, fontweight='bold')
        ax.set_ylabel('Runtime (ms)', fontsize=12, fontweight='bold')
        ax.set_title('Runtime Distribution (Violin Plot)', fontsize=13, fontweight='bold')
        ax.set_xticks(range(len(labels)))
        ax.set_xticklabels(labels)
        ax.grid(axis='y', linestyle='--', alpha=0.4)
        
        fig.tight_layout()
        fig.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        print(f"✅ Saved distribution chart: {output_path}")


def create_performance_matrix(summary: dict, output_path: Path) -> None:
    """Create a heatmap-style performance matrix."""
    if not HAS_MATPLOTLIB:
        return
    
    languages = []
    metrics = []
    
    for lang in ['Go', 'C++', 'NanLang', 'Python']:
        if lang in summary:
            languages.append(lang)
            data = summary[lang]
            
            compile = data.get('compile_mean_ms', 0)
            runtime = data.get('runtime_mean_ms', 0)
            
            metrics.append([compile, runtime])
    
    if not metrics:
        return
    
    # Normalize metrics for visualization
    max_compile = max(m[0] for m in metrics) if any(m[0] > 0 for m in metrics) else 1
    max_runtime = max(m[1] for m in metrics)
    
    fig, ax = plt.subplots(figsize=(8, 6))
    
    # Create matrix
    matrix = []
    for compile_ms, runtime_ms in metrics:
        matrix.append([
            (compile_ms / max_compile) * 100 if compile_ms > 0 else 0,
            (runtime_ms / max_runtime) * 100
        ])
    
    im = ax.imshow(matrix, cmap='RdYlGn_r', aspect='auto', vmin=0, vmax=100)
    
    ax.set_xticks([0, 1])
    ax.set_xticklabels(['Compile', 'Runtime'])
    ax.set_yticks(range(len(languages)))
    ax.set_yticklabels(languages)
    ax.set_title('Performance Matrix (Relative %)', fontsize=13, fontweight='bold')
    
    # Add values
    for i, lang_metrics in enumerate(matrix):
        for j, val in enumerate(lang_metrics):
            text = ax.text(j, i, f'{val:.0f}%', ha='center', va='center', 
                         color='white' if val > 60 else 'black', fontweight='bold')
    
    fig.colorbar(im, ax=ax, label='Relative Performance %')
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    print(f"✅ Saved performance matrix: {output_path}")


def main() -> int:
    bench_dir = Path(__file__).parent.parent / "benchmark_results"
    
    summary_path = bench_dir / "all_langs_summary.json"
    raw_path = bench_dir / "all_langs_raw.csv"
    
    if not summary_path.exists():
        print("❌ Benchmark summary not found. Run bench_all_langs.py first.")
        return 1
    
    print("📊 Loading benchmark data...")
    summary = load_summary(summary_path)
    raw_data = load_raw_data(raw_path) if raw_path.exists() else {}
    
    print_summary_table(summary)
    
    print("🎨 Generating advanced charts...")
    create_comparison_chart(summary, bench_dir / "all_langs_comparison.png")
    create_speedup_chart(summary, bench_dir / "all_langs_speedup.png")
    create_performance_matrix(summary, bench_dir / "all_langs_matrix.png")
    
    if raw_data:
        create_violin_chart(raw_data, bench_dir / "all_langs_distribution.png")
    
    print("\n✅ Analysis complete!")
    print(f"\n📁 Results saved to: {bench_dir}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
