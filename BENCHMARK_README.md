# 🚀 NanLang Multi-Language Benchmark Suite

Comprehensive benchmark comparison: **Go vs C++ vs NanLang vs Python**

## 📊 Quick Results (12 Iterations)

| Language | Compile Time | Runtime | Speedup vs NanLang |
|----------|-------------|---------|-------------------|
| **Go** | 227.35 ± 61.11 ms | 3.6889 ± 0.1063 ms | **9.5x compile** / **6.1x runtime** |
| **C++** | 456.88 ± 19.59 ms | 3.6914 ± 0.1566 ms | **19.1x compile** / **6.1x runtime** |
| **NanLang** | 23.87 ± 8.56 ms | 22.4844 ± 8.4056 ms | **Baseline** |
| **Python** | — (interpreted) | 31.9499 ± 0.1706 ms | **Slowest** |

## 🏆 Winners

- 🥇 **Fastest Compilation**: NanLang (23.87 ms)
- 🥇 **Fastest Runtime**: Go & C++ (tied at 3.69 ms)
- 🥇 **Best Balance**: Go
- 🥇 **Easiest Development**: Python (no compile step)

## 📦 What's Included

### Scripts (4 Files)

1. **`bench_all_langs.py`** - Main benchmark engine
   - Compiles and runs each language with same workload
   - Measures compilation and runtime separately
   - Generates CSV, JSON, and PNG outputs

2. **`analyze_benchmarks.py`** - Advanced analysis
   - Prints detailed statistics table
   - Generates 4 additional comparison charts
   - Calculates speedup ratios
   - Creates performance matrix heatmap

3. **`generate_html_report.py`** - Interactive HTML report
   - Combines all charts into beautiful HTML
   - Professional styling and explanations
   - Easy browser viewing

4. **`BENCHMARK_GUIDE.py`** - Documentation
   - Complete usage instructions
   - Examples and advanced usage
   - Troubleshooting guide

### Output Files (11 Files)

**Data:**
- `all_langs_raw.csv` - Raw measurement data
- `all_langs_summary.json` - Statistical summary

**Visualization (PNG):**
- `all_langs_compile.png` - Compile time comparison
- `all_langs_runtime.png` - Runtime comparison
- `all_langs_comparison.png` - Side-by-side detailed comparison
- `all_langs_speedup.png` - Speedup ratios vs NanLang
- `all_langs_matrix.png` - Performance matrix heatmap
- `all_langs_distribution.png` - Runtime distribution (violin plot)

**Reports:**
- `benchmark_report.html` - Complete interactive report

## 🚀 Quick Start

### Run All Benchmarks

```bash
cd /home/aras/NanLang

# 1. Run benchmarks (takes ~2-3 minutes)
python3 scripts/bench_all_langs.py --iterations 12 --warmup 2

# 2. Generate analysis charts
python3 scripts/analyze_benchmarks.py

# 3. Create HTML report
python3 scripts/generate_html_report.py

# 4. View report in browser
open benchmark_results/benchmark_report.html
```

### Or Run Everything at Once

```bash
cd /home/aras/NanLang && \
python3 scripts/bench_all_langs.py && \
python3 scripts/analyze_benchmarks.py && \
python3 scripts/generate_html_report.py && \
echo "✅ All benchmarks complete! Check benchmark_results/"
```

## 📈 Interpreting Results

### Compile Time
- **NanLang wins significantly** (23.87 ms) - ~10x faster than Go, ~19x faster than C++
- Go is fast but not as fast as NanLang
- C++ is slowest due to optimization stages
- Python has no compile phase

### Runtime Performance
- **Go and C++ dominate** (3.69 ms) - compiled languages advantage
- NanLang slower (22.48 ms) due to VM/interpreter
- Python slowest (31.95 ms) due to interpretation overhead

### Use Case Recommendations

| Use Case | Best Choice | Reason |
|----------|------------|--------|
| **High-Performance Apps** | Go or C++ | Native runtime speed |
| **Fast Development Cycle** | NanLang | Rapid compilation |
| **Rapid Prototyping** | Python | No compilation needed |
| **Production Balance** | Go | Compile speed + runtime |

## 🐍 Python Examples

### Load and Analyze Data

```python
import json
import pandas as pd

# Load summary
with open('benchmark_results/all_langs_summary.json') as f:
    summary = json.load(f)

# Calculate speedup
go_compile = summary['Go']['compile_mean_ms']
nan_compile = summary['NanLang']['compile_mean_ms']
speedup = go_compile / nan_compile
print(f"Go is {speedup:.1f}x slower than NanLang at compilation")

# Load raw data
df = pd.read_csv('benchmark_results/all_langs_raw.csv')
print(df.groupby('language')[['compile_ms', 'runtime_ms']].describe())
```

### Create Custom Chart

```python
import matplotlib.pyplot as plt
import json

with open('benchmark_results/all_langs_summary.json') as f:
    data = json.load(f)

languages = list(data.keys())
runtimes = [data[l]['runtime_mean_ms'] for l in languages]

fig, ax = plt.subplots(figsize=(10, 6))
ax.bar(languages, runtimes, color=['#00ADD8', '#00599C', '#2ecc71', '#3776ab'])
ax.set_ylabel('Runtime (ms)')
ax.set_title('Runtime Performance Comparison')
ax.grid(axis='y', linestyle='--', alpha=0.3)

# Add values on bars
for i, v in enumerate(runtimes):
    ax.text(i, v, f'{v:.4f}', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
plt.savefig('my_custom_benchmark.png', dpi=150)
```

## 🔧 Advanced Options

### Custom Iteration Count

```bash
# Testing (fewer iterations, faster):
python3 scripts/bench_all_langs.py --iterations 5 --warmup 1

# Detailed benchmark (more iterations, more accurate):
python3 scripts/bench_all_langs.py --iterations 30 --warmup 5
```

### Benchmark Specific Workload

Edit the generate_*_source functions in `bench_all_langs.py` to modify workload:

```python
def generate_go_source(path: Path, workload_lines: int) -> None:
    """Generate custom Go benchmark."""
    code = """package main
func main() {
    // Add your custom benchmark code here
    // Current: simple arithmetic in a loop
}
"""
```

## 🛠️ Troubleshooting

| Issue | Solution |
|-------|----------|
| "Go not found" | `sudo apt install golang` or `brew install go` |
| "g++ not found" | `sudo apt install build-essential` |
| "nanlang_cpp not found" | `cd /home/aras/NanLang && make clean && make` |
| "matplotlib not available" | `pip3 install matplotlib numpy` |
| Timeout errors | Increase timeout in `bench_all_langs.py` (line ~100) |

## 📊 Benchmark Methodology

### Workload
- Simple arithmetic operations (bit shifts, XOR)
- Equivalent to ~500-600 lines of actual code
- Same workload across all languages for fairness

### Process
1. **Warmup**: 2 iterations to stabilize system
2. **Measurement**: 12 iterations with various problem sizes
3. **Statistics**: Calculate mean, stdev, median, p95, CI95
4. **Export**: CSV (raw), JSON (summary), PNG (charts)

### Notes
- Warmup runs are discarded (prevents cold start bias)
- Each iteration uses different workload size
- Measurements exclude I/O overhead
- All compiled with maximum optimizations (-O3)

## 📚 Files Location

```
/home/aras/NanLang/
├── scripts/
│   ├── bench_all_langs.py           ← Main benchmark
│   ├── analyze_benchmarks.py          ← Analysis & charts
│   ├── generate_html_report.py        ← HTML reporter
│   └── BENCHMARK_GUIDE.py             ← This guide
│
└── benchmark_results/
    ├── all_langs_raw.csv              ← Raw data
    ├── all_langs_summary.json         ← Summary stats
    ├── all_langs_*.png                ← 6 charts
    └── benchmark_report.html          ← HTML report
```

## 🎯 Real-World Implications

### Production vs Benchmark
This benchmark shows **peak performance** under ideal conditions. Real applications may show different results:

- **Large applications**: Compilation advantage diminishes
- **Complex logic**: VM overhead becomes less relevant
- **I/O bound**: All languages converge to similar performance
- **Memory intensive**: Cache effects dominate

### When to Use Each

| Language | Best For |
|----------|----------|
| **NanLang** | Education, rapid iteration, domain-specific tasks |
| **Go** | Microservices, cloud tools, concurrent systems |
| **C++** | Embedded, game engines, performance-critical code |
| **Python** | Data science, ML, rapid development, automation |

## 💡 Insights

1. **Compilation speed != runtime speed**
   - Fast compilation (NanLang) doesn't guarantee fast runtime
   - Go balances both reasonably well

2. **VM overhead is significant**
   - NanLang's interpretation adds ~20ms
   - Python's adds ~30ms
   - Native code (Go/C++) is ~6x faster

3. **Development velocity matters**
   - Fast compilation (NanLang) reduces development friction
   - Matters more for iterative development
   - Less important for batch processing

4. **Scaling effects**
   - This benchmark: micro-optimization focused
   - Real apps: architecture and algorithm design matter more
   - Choose language based on use case, not micro-benchmarks alone

## 🚀 Future Enhancements

Potential additions to the benchmark suite:

- [ ] Additional languages (Rust, Java, C#)
- [ ] Memory allocation benchmarks
- [ ] Concurrency/parallelism tests
- [ ] I/O performance tests
- [ ] Statistical significance testing
- [ ] Historical trend charts
- [ ] Regression detection
- [ ] Cloud deployment benchmarks

## 📝 License

Same as NanLang project

## 👥 Contributing

To extend benchmarks:

1. Add new language generator function
2. Add benchmark function following existing pattern
3. Add to main() benchmark loop
4. Test with `python3 scripts/bench_all_langs.py`

---

**Last Updated**: March 19, 2026
**Framework**: Python 3.7+ with matplotlib
**Status**: ✅ Production Ready
