#!/usr/bin/env python3
"""
COMPLETE BENCHMARK SUITE USAGE GUIDE
=====================================

Bu paket, Go, C++, NanLang ve Python arasında
kapsamlı performans karşılaştırması yapmak için
araçlar sağlar.

OLUŞTURULAN DOSYALAR:
- scripts/bench_all_langs.py      : Ana benchmark scripti
- scripts/analyze_benchmarks.py    : Detaylı analiz ve grafikler
- scripts/generate_html_report.py  : İnteraktif HTML rapor

KULLANIM:
"""

import subprocess
import sys
from pathlib import Path


def main():
    print(__doc__)
    
    print("""
ADIM 1: Ana Benchmark'ı Çalıştırın
===================================
cd /home/aras/NanLang
python3 scripts/bench_all_langs.py --iterations 12 --warmup 2

Bu komut:
  • Go, C++, NanLang ve Python'u farklı boyutlarda çalıştırır
  • Kompilasyon ve çalışma zamanı metriklerini toplar
  • CSV ve JSON dosyalarında sonuçları kaydeder
  • İlk PNG grafiklerini oluşturur
  
Seçenekler:
  --iterations N    : Her dil için N ölçüm (varsayılan: 15)
  --warmup N       : Her dil için N ısınma (varsayılan: 2)
  --repo PATH      : Repository root (varsayılan: cwd)


ADIM 2: Detaylı Analiz ve Ek Grafikler
=======================================
python3 scripts/analyze_benchmarks.py

Bu komut:
  ✅ Özet tablosunu yazdırır
  ✅ Karşılaştırma grafiği oluşturur
  ✅ Hızlanma (speedup) grafiği oluşturur
  ✅ Performans matrisi oluşturur
  ✅ Runtime dağılımı (violin plot) oluşturur
  
Çıktı dosyaları:
  - all_langs_comparison.png     (Yan yana karşılaştırma)
  - all_langs_speedup.png        (NanLang'a göre hızlanma)
  - all_langs_matrix.png         (Performans matrisi)
  - all_langs_distribution.png   (Runtime dağılımı)


ADIM 3: HTML Rapor Oluşturun
=============================
python3 scripts/generate_html_report.py

Bu komut:
  📄 Tüm grafikleri içeren güzel bir HTML rapor oluşturur
  🎨 İnteraktif formatla sunuyor
  
Çıktı:
  - benchmark_results/benchmark_report.html

Tarayıcıda açmak için:
  open benchmark_results/benchmark_report.html


ÇIKTI DOSYALARI
===============

CSV (Raw Data):
  all_langs_raw.csv
    - Language, compile_ms, runtime_ms

JSON (Özet):
  all_langs_summary.json
    - Her dil için: compile_mean, compile_stdev, runtime_mean, runtime_stdev

PNG Grafikler:
  1. all_langs_compile.png      - Kompilasyon zamanı karşılaştırması
  2. all_langs_runtime.png      - Runtime karşılaştırması
  3. all_langs_comparison.png   - Yan yana karşılaştırma
  4. all_langs_speedup.png      - NanLang'a göre hızlanma oranı
  5. all_langs_matrix.png       - Performans matrisi (ısı haritası)
  6. all_langs_distribution.png - Runtime dağılımı (violin plot)

HTML Rapor:
  benchmark_report.html         - Tüm grafikleri içeren raporDÖNEM SEÇENEKLERİ
===============

Python ile grafikler özelleştirebilirsiniz:

from pathlib import Path
import json

# Özet yükle
with open('benchmark_results/all_langs_summary.json') as f:
    summary = json.load(f)

# Örneğin, Go'nun hızlanmasını hesapla
nanlang_compile = summary['NanLang']['compile_mean_ms']
go_compile = summary['Go']['compile_mean_ms']
speedup = go_compile / nanlang_compile

print(f"Go, NanLang'dan {speedup:.1f}x daha yavaş derlenir")


BENCHMARK SONUÇLARI (Test Çalışması)
====================================

📊 Diller Arası Karşılaştırma (12 iterasyon):

┌──────────────────────────────────────────────────────────┐
│ Language │ Compile (ms)    │ Runtime (ms)    │ Speedup  │
├──────────────────────────────────────────────────────────┤
│ Go       │  227.35 ± 61.11 │  3.6889 ± 0.1063│ 9.5x     │
│ C++      │  456.88 ± 19.59 │  3.6914 ± 0.1566│ 19.1x    │
│ NanLang  │   23.87 ± 8.56  │ 22.4844 ± 8.4056│ 1.0x     │
│ Python   │    —             │ 31.9499 ± 0.1706│  —       │
└──────────────────────────────────────────────────────────┘

🏆 Sonuçlar:
  • NanLang: EN HIZLI DERLENİŞ (kompilasyon)
  • Go & C++: EN HIZLI ÇALIŞMA (runtime)
  • Go: DENGELİ PERFORMANS
  • Python: HIZLI GELİŞTİRME (derlenme yok)


GELIŞMIŞ KULLANIM
=================

1. Belirli çıkış formatını alın:

  # CSV'yi Pandas'ta yükle
  import pandas as pd
  df = pd.read_csv('benchmark_results/all_langs_raw.csv')
  print(df.groupby('language').describe())


2. Kendi grafiklerinizi yapın:

  import matplotlib.pyplot as plt
  
  languages = ['Go', 'C++', 'NanLang', 'Python']
  times = [227.35, 456.88, 23.87, 31.95]  # runtime ms
  
  fig, ax = plt.subplots()
  ax.bar(languages, times, color=['#00ADD8', '#00599C', '#2ecc71', '#3776ab'])
  ax.set_ylabel('Runtime (ms)')
  ax.set_title('Runtime Comparison')
  plt.savefig('my_custom_chart.png', dpi=150)


3. Benchmark'ı CI/CD'de çalıştırın:

  # GitHub Actions, GitLab CI, etc.
  - name: Run Benchmarks
    run: |
      cd NanLang
      python3 scripts/bench_all_langs.py --iterations 20
      python3 scripts/analyze_benchmarks.py
      python3 scripts/generate_html_report.py
      
  - name: Upload Results
    uses: actions/upload-artifact@v3
    with:
      name: benchmark-results
      path: benchmark_results/


AYARLANACAK PARAMETRELER
========================

Daha yavaş makinalarda:
  python3 scripts/bench_all_langs.py --iterations 5 --warmup 1

Daha kesin sonuçlar için:
  python3 scripts/bench_all_langs.py --iterations 30 --warmup 5

Timeout sorunları?
  - scripts/bench_all_langs.py içinde timeout parametresini artırın
  - compile_timeout = 60  # seconds


SORUN GIDERME
=============

❌ "Go not found":
  sudo apt install golang  # veya Homebrew'u kullanın

❌ "g++ not found":
  sudo apt install build-essential

❌ "nanlang_cpp not found":
  make clean && make nanlang_cpp

❌ "matplotlib not available":
  pip3 install matplotlib numpy

❌ Grafikler oluşturulmadı:
  - /tmp dizininde yer var mı kontrol edin
  - matplotlib kurulumu kontrol edin: python3 -c "import matplotlib"


REFERANS
========

Benchmark Metodolojisi:
  • Sıcak başlangıç (warmup) yapılır
  • 12 yinelemede ölçülür
  • Mean ve StDev hesaplanır
  • Detaylı istatistikler saklanır

Workload (İş Yükü):
  • Basit döngüler ile bit shift/XOR işlemleri
  • Değerlendirme süresi: ~500-600 satır kod eşdeğeri
  • Tüm diller için eşit iş yükü

Limitler:
  • Gerçek uygulamalar farklı performans gösterebilir
  • Bu benchmark mikro-optimizasyonları gösterir
  • Üretim senaryoları için daha karmaşık testler gerekli


💾 Tüm Dosyalar Şurada:
  /home/aras/NanLang/benchmark_results/
""")


if __name__ == "__main__":
    print("\n✅ Benchmark Suite Hazır!")
    print("\nKullanmak için:")
    print("  1. python3 scripts/bench_all_langs.py")
    print("  2. python3 scripts/analyze_benchmarks.py")
    print("  3. python3 scripts/generate_html_report.py")
    print("\nDetaylı talimatlar: python3 scripts/BENCHMARK_GUIDE.py")
