#!/bin/bash
set -e

COMPILER=${COMPILER:-g++}
# -flto burada hayat kurtaracak, kullanılmayan kodları linker silecek
CXXFLAGS="${CXXFLAGS:--std=c++20 -O3 -march=native -flto -Wall -Wextra}"
CXXFLAGS="$CXXFLAGS -Iinclude"
OUTPUT="${OUTPUT:-nanlang}"

echo "Building NanLang (Optimized Edition)..."

# SADECE ÇEKİRDEK VE ANA CLI DOSYALARINI DERLE
# main.cpp ve npi_cli.cpp dosyasını sildik çünkü kendi main()'leri var!
$COMPILER $CXXFLAGS -c src/engine.cpp -o /tmp/engine.o
$COMPILER $CXXFLAGS -c src/arch_ops.cpp -o /tmp/arch_ops.o
$COMPILER $CXXFLAGS -c src/binary_ops.cpp -o /tmp/binary_ops.o
$COMPILER $CXXFLAGS -c src/compiler_ops.cpp -o /tmp/compiler_ops.o
$COMPILER $CXXFLAGS -c src/nan_aot_compiler.cpp -o /tmp/nan_aot_compiler.o
$COMPILER $CXXFLAGS -c src/nan_cli_lib.cpp -o /tmp/nan_cli_lib.o
$COMPILER $CXXFLAGS -c src/nanlang_cli.cpp -o /tmp/nanlang_cli.o
$COMPILER $CXXFLAGS -c src/perf_ops.cpp -o /tmp/perf_ops.o
$COMPILER $CXXFLAGS -c src/pulse_emitter.cpp -o /tmp/pulse_emitter.o
$COMPILER $CXXFLAGS -c kernel/scheduler.cpp -o /tmp/scheduler.o

echo "Linking final binary..."
$COMPILER $CXXFLAGS \
    /tmp/engine.o /tmp/arch_ops.o /tmp/binary_ops.o /tmp/compiler_ops.o \
    /tmp/nan_aot_compiler.o /tmp/nan_cli_lib.o /tmp/nanlang_cli.o \
    /tmp/perf_ops.o /tmp/pulse_emitter.o /tmp/scheduler.o \
    -o $OUTPUT

# Gereksiz sembolleri temizle (Binary size için kritik!)
strip --strip-unneeded $OUTPUT

echo "✓ Build successful! Binary size: $(du -h $OUTPUT | cut -f1)"