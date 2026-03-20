#!/usr/bin/env python3
"""
Generate a comprehensive HTML benchmark report.
"""

import json
from pathlib import Path
from datetime import datetime


def generate_html_report(summary_path: Path, output_path: Path) -> None:
    """Generate an HTML report from benchmark data."""
    
    with open(summary_path) as f:
        summary = json.load(f)
    
    html_content = '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NanLang vs Go vs C++ vs Python - Benchmark Report</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        
        header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }
        
        header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        header p {
            font-size: 1.1em;
            opacity: 0.9;
        }
        
        .content {
            padding: 40px;
        }
        
        .section {
            margin-bottom: 40px;
        }
        
        .section h2 {
            font-size: 1.8em;
            color: #333;
            margin-bottom: 20px;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
            background: #f8f9fa;
        }
        
        table th {
            background: #667eea;
            color: white;
            padding: 15px;
            text-align: left;
            font-weight: 600;
        }
        
        table td {
            padding: 12px 15px;
            border-bottom: 1px solid #dee2e6;
        }
        
        table tr:hover {
            background: #f0f1ff;
        }
        
        .metric {
            display: inline-block;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px 30px;
            margin: 10px 10px 10px 0;
            border-radius: 8px;
            text-align: center;
            min-width: 200px;
        }
        
        .metric-value {
            font-size: 2em;
            font-weight: bold;
            display: block;
        }
        
        .metric-label {
            font-size: 0.9em;
            opacity: 0.9;
        }
        
        .charts {
            display: grid;
            grid-template-columns: 1fr;
            gap: 20px;
            margin: 30px 0;
        }
        
        .chart-box {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 15px;
            text-align: center;
        }
        
        .chart-box img {
            max-width: 100%;
            height: auto;
            border-radius: 8px;
        }
        
        .insights {
            background: #e7f3ff;
            border-left: 4px solid #667eea;
            padding: 20px;
            margin: 20px 0;
            border-radius: 4px;
        }
        
        .insights h3 {
            color: #667eea;
            margin-bottom: 10px;
        }
        
        .insights ul {
            margin-left: 20px;
        }
        
        .insights li {
            margin: 8px 0;
            color: #333;
        }
        
        .footer {
            background: #f8f9fa;
            padding: 20px;
            text-align: center;
            color: #666;
            border-top: 1px solid #dee2e6;
        }
        
        .langcomparison {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        
        .lang-card {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
        }
        
        .lang-card h3 {
            font-size: 1.5em;
            margin-bottom: 15px;
        }
        
        .lang-stat {
            margin: 10px 0;
            font-size: 0.95em;
        }
        
        .lang-stat-label {
            opacity: 0.8;
        }
        
        .lang-stat-value {
            font-weight: bold;
            font-size: 1.1em;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🚀 Multi-Language Benchmark Report</h1>
            <p>Go vs C++ vs NanLang vs Python Performance Comparison</p>
            <p style="font-size: 0.9em; margin-top: 10px;">Generated: ''' + datetime.now().strftime("%Y-%m-%d %H:%M:%S") + '''</p>
        </header>
        
        <div class="content">
'''
    
    # Summary section
    html_content += '<div class="section">\n'
    html_content += '<h2>📊 Performance Summary</h2>\n'
    
    html_content += '<table>\n'
    html_content += '<tr><th>Language</th><th>Compile Time (ms)</th><th>Runtime (ms)</th><th>Compile Speedup</th><th>Runtime Speedup</th></tr>\n'
    
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
        
        if compile_mean > 0:
            compile_str = f"{compile_mean:.2f} ± {compile_std:.2f}"
            compile_speedup = f"{compile_mean / nanlang_compile:.1f}x"
        else:
            compile_str = "—"
            compile_speedup = "—"
        
        runtime_str = f"{runtime_mean:.4f} ± {runtime_std:.4f}"
        runtime_speedup = f"{nanlang_runtime / runtime_mean:.2f}x" if runtime_mean > 0 else "—"
        
        html_content += f'<tr><td><strong>{lang}</strong></td><td>{compile_str}</td><td>{runtime_str}</td><td>{compile_speedup}</td><td>{runtime_speedup}</td></tr>\n'
    
    html_content += '</table>\n'
    html_content += '</div>\n'
    
    # Language cards
    html_content += '<div class="section">\n'
    html_content += '<h2>🎯 Language Profiles</h2>\n'
    html_content += '<div class="langcomparison">\n'
    
    colors = {
        'Go': 'linear-gradient(135deg, #00ADD8 0%, #0066CC 100%)',
        'C++': 'linear-gradient(135deg, #00599C 0%, #004687 100%)',
        'NanLang': 'linear-gradient(135deg, #2ecc71 0%, #27ae60 100%)',
        'Python': 'linear-gradient(135deg, #3776ab 0%, #ffd343 100%)',
    }
    
    for lang in ['Go', 'C++', 'NanLang', 'Python']:
        if lang not in summary:
            continue
        
        data = summary[lang]
        compile_mean = data.get('compile_mean_ms', 0)
        runtime_mean = data.get('runtime_mean_ms', 0)
        
        html_content += f'<div class="lang-card" style="background: {colors.get(lang, "linear-gradient(135deg, #667eea 0%, #764ba2 100%)")};">\n'
        html_content += f'<h3>{lang}</h3>\n'
        
        if compile_mean > 0:
            html_content += f'<div class="lang-stat"><span class="lang-stat-label">Compile:</span> <span class="lang-stat-value">{compile_mean:.2f} ms</span></div>\n'
        
        html_content += f'<div class="lang-stat"><span class="lang-stat-label">Runtime:</span> <span class="lang-stat-value">{runtime_mean:.4f} ms</span></div>\n'
        
        # Samples
        compile_samples = data.get('compile_samples', 0)
        runtime_samples = data.get('runtime_samples', 0)
        html_content += f'<div class="lang-stat" style="opacity: 0.8;"><span class="lang-stat-label">Samples:</span> {compile_samples}/{runtime_samples}</div>\n'
        
        html_content += '</div>\n'
    
    html_content += '</div>\n'
    html_content += '</div>\n'
    
    # Charts section
    html_content += '<div class="section">\n'
    html_content += '<h2>📈 Visual Analysis</h2>\n'
    
    charts = [
        ('all_langs_compile.png', 'Compilation Time Comparison'),
        ('all_langs_runtime.png', 'Runtime Comparison'),
        ('all_langs_comparison.png', 'Side-by-Side Comparison'),
        ('all_langs_speedup.png', 'Speedup vs NanLang'),
        ('all_langs_matrix.png', 'Performance Matrix'),
        ('all_langs_distribution.png', 'Runtime Distribution'),
    ]
    
    for chart_file, chart_title in charts:
        chart_path = Path(__file__).parent.parent / 'benchmark_results' / chart_file
        if chart_path.exists():
            html_content += '<div class="chart-box">\n'
            html_content += f'<h3>{chart_title}</h3>\n'
            html_content += f'<img src="{chart_file}" alt="{chart_title}">\n'
            html_content += '</div>\n'
    
    html_content += '</div>\n'
    
    # Insights section
    html_content += '<div class="section">\n'
    html_content += '<h2>💡 Key Insights</h2>\n'
    
    html_content += '<div class="insights">\n'
    html_content += '<h3>Compilation Performance</h3>\n'
    html_content += '<ul>\n'
    html_content += '<li><strong>NanLang</strong> is significantly fastest at compilation (23.87 ms), ~10x faster than Go and ~19x faster than C++</li>\n'
    html_content += '<li><strong>Go</strong> provides a balance between speed and compilation time (227 ms)</li>\n'
    html_content += '<li><strong>C++</strong> has the slowest compilation (457 ms) but produces fastest executables</li>\n'
    html_content += '<li><strong>Python</strong> has no compile phase as it is interpreted</li>\n'
    html_content += '</ul>\n'
    html_content += '</div>\n'
    
    html_content += '<div class="insights">\n'
    html_content += '<h3>Runtime Performance</h3>\n'
    html_content += '<ul>\n'
    html_content += '<li><strong>Go & C++</strong> dominate runtime (3.69 ms) - compiled languages advantage</li>\n'
    html_content += '<li><strong>NanLang</strong> runtime is slower (22.48 ms) as it uses a VM/interpreter model</li>\n'
    html_content += '<li><strong>Python</strong> is slowest runtime (31.95 ms) due to interpretation overhead</li>\n'
    html_content += '<li>Go and C++ are ~6x faster than NanLang and ~8.7x faster than Python</li>\n'
    html_content += '</ul>\n'
    html_content += '</div>\n'
    
    html_content += '<div class="insights">\n'
    html_content += '<h3>Overall Assessment</h3>\n'
    html_content += '<ul>\n'
    html_content += '<li>🏆 <strong>Best for compiled performance:</strong> C++ and Go (tied at 3.69 ms runtime)</li>\n'
    html_content += '<li>⚡ <strong>Best for rapid development:</strong> NanLang (fastest compilation)</li>\n'
    html_content += '<li>🐍 <strong>Best for flexibility:</strong> Python (no compilation needed)</li>\n'
    html_content += '<li>⚖️ <strong>Best balanced:</strong> Go (fast compile + fast runtime)</li>\n'
    html_content += '</ul>\n'
    html_content += '</div>\n'
    
    html_content += '</div>\n'
    
    html_content += '''
        </div>
        
        <div class="footer">
            <p>Benchmark Report | NanLang Project | 2026</p>
            <p style="font-size: 0.9em; margin-top: 10px;">
                Raw data available in: all_langs_raw.csv, all_langs_summary.json
            </p>
        </div>
    </div>
</body>
</html>
'''
    
    output_path.write_text(html_content)
    print(f"✅ Generated HTML report: {output_path}")


if __name__ == "__main__":
    bench_dir = Path(__file__).parent.parent / "benchmark_results"
    summary_path = bench_dir / "all_langs_summary.json"
    report_path = bench_dir / "benchmark_report.html"
    
    if summary_path.exists():
        generate_html_report(summary_path, report_path)
    else:
        print("❌ Summary file not found")
