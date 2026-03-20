CXX      ?= g++
CC       ?= gcc
TARGET_CPP   := nanlang_cpp
TARGET_CLI   := nanlang
TARGET_NPI   := npi
TARGET_CC    := nanlang_cc

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 pangocairo 2>/dev/null)
GTK_LIBS   := $(shell pkg-config --libs gtk+-3.0 pangocairo 2>/dev/null)

CPP_BIN      := bin/$(TARGET_CPP)
CLI_BIN      := bin/$(TARGET_CLI)
NPI_BIN      := bin/$(TARGET_NPI)
CC_BIN       := bin/$(TARGET_CC)

# C++ Source files (core)
SRCS_CPP := src/nanlang_cpp.cpp \
        src/pulse_emitter.cpp \
        src/nan_aot_compiler.cpp

SRCS_CC := src/nanlang_cc.c

SRCS_CLI := src/nanlang_cli.cpp \
        src/nan_cli_lib.cpp

SRCS_NPI := src/npi_cli.cpp \
        src/nan_cli_lib.cpp

OBJS_CPP := $(SRCS_CPP:.cpp=.o)
OBJS_CC := $(SRCS_CC:.c=.o)
OBJS_CLI := $(SRCS_CLI:.cpp=.o)
OBJS_NPI := $(SRCS_NPI:.cpp=.o)

# ── Flag sets ───────────────────────────────────────────────
BASE_FLAGS  := -std=c++17 -Wall -Wextra -Wpedantic \
               -I./include -pthread \
               $(GTK_CFLAGS) \
               -mavx2 -msse4.2 -mrtm

REL_FLAGS   := $(BASE_FLAGS) -O3 -march=native -flto \
               -funroll-loops -fomit-frame-pointer \
               -DNDEBUG

DBG_FLAGS   := $(BASE_FLAGS) -O0 -g3 -fsanitize=address,undefined \
               -fno-omit-frame-pointer -DDEBUG

FAST_FLAGS  := $(REL_FLAGS) -fprofile-generate

# Default: release
CXXFLAGS ?= $(REL_FLAGS)
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O3 -DNDEBUG

# ── Demo parameters (overridable) ───────────────────
COUNT      ?= 64
PATTERN    ?= 0xAA
ITERATIONS ?= 100
ROUNDS     ?= 256
SIZE       ?= 4096
SEGMENTS   ?= 3
CPU_KEY    ?= 0xCAFEBABEDEADBEEF
NIBBLE     ?= 0x0B
OFFSET     ?= 0
LEN        ?= 4

# User parameters (for compile/run/native/patch)
SRC   ?= hello.nl
OUT   ?= output.elf
NB    ?= output.elf
FILE  ?= nanlang
OFF   ?= 0x00
BYTES ?= 90
OPT   ?= 0
MODE  ?=
PREPROCESSOR ?= scripts/nan_preprocessor.raku
PREPROCESSED ?= out/preprocessed.nl
EXAMPLE      ?= examples/pulse_controller.nl
EXAMPLE_OUT  ?= out/pulse_controller.elf

# ── Helper functions ──────────────────────────────────────
ifeq ($(OPT),1)
  OPT_FLAG := --opt
else
  OPT_FLAG :=
endif

# ══════════════════════════════════════════════════════════════
# BUILD TARGETS
# ══════════════════════════════════════════════════════════════

.PHONY: all release debug fast clean mrcleaner demo demo-all \
        demo-pulse demo-sigbuf demo-simd demo-dma demo-zerocopy \
        demo-regmap demo-binary demo-compress demo-selfheal \
        demo-jit demo-linker demo-color demo-fold demo-xplatform \
        demo-hwfp demo-waitfree demo-deadlock demo-branch \
        demo-pipeline demo-ooo \
        preprocess compile run native patch \
        list-libs list-examples compile-example \
        install-local install-system \
        npi-clean npi-deps npi-build npi-full full \
        info help

all: unified

unified: $(CLI_BIN) $(CPP_BIN) $(CC_BIN) $(NPI_BIN)
	@mkdir -p out lib
	@echo "  [OK] Unified built. Usage: ./$(CLI_BIN) --help"

release: CXXFLAGS = $(REL_FLAGS)
release: $(CPP_BIN)
	@echo "  [OK] C++ core release: ./$(CPP_BIN)"

debug: CXXFLAGS = $(DBG_FLAGS)
debug: $(CPP_BIN)
	@echo "  [OK] C++ debug: ./$(CPP_BIN) (ASan)"

fast: CXXFLAGS = $(FAST_FLAGS)
fast: $(CPP_BIN)
	@echo "  [OK] Fast build (PGO+LTO): ./$(CPP_BIN)"

$(CPP_BIN): $(OBJS_CPP)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTK_LIBS)
	@strip --strip-debug $(CPP_BIN) 2>/dev/null || true

$(TARGET_CPP): $(CPP_BIN)
	@true

$(CC_BIN): $(OBJS_CC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(CC_BIN) $(OBJS_CC)
	@strip --strip-debug $(CC_BIN) 2>/dev/null || true

$(TARGET_CC): $(CC_BIN)
	@true

$(CLI_BIN): $(OBJS_CLI)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(CLI_BIN) $(OBJS_CLI)
	@strip --strip-debug $(CLI_BIN) 2>/dev/null || true

$(NPI_BIN): $(OBJS_NPI)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $(NPI_BIN) $(OBJS_NPI)
	@strip --strip-debug $(NPI_BIN) 2>/dev/null || true

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ══════════════════════════════════════════════════════════════
# USER COMMAND TARGETS (parameterized)
# ══════════════════════════════════════════════════════════════

compile: unified
	@mkdir -p out
	@tmp_file=$$(mktemp out/.nan_preprocessed.XXXXXX.nl); \
	trap 'rm -f "$$tmp_file"' EXIT; \
	echo "  [PRE] raku $(PREPROCESSOR) $(SRC) -> $$tmp_file"; \
	raku $(PREPROCESSOR) $(SRC) > "$$tmp_file" && \
	echo "  [BUILD] ./$(CLI_BIN) compile $$tmp_file $(MODE) --output $(OUT)" && \
	./$(CLI_BIN) compile "$$tmp_file" $(MODE) --output $(OUT)

preprocess:
	@mkdir -p out
	@echo "  [PRE] raku $(PREPROCESSOR) $(SRC) -> $(PREPROCESSED)"
	@raku $(PREPROCESSOR) $(SRC) > $(PREPROCESSED)
	@echo "  [OK] Preprocessed file: $(PREPROCESSED)"

run: unified
	./$(CLI_BIN) run $(NB)

native:
	@if [ -x ./nanlang ]; then \
		./nanlang --build-native $(SRC) --output $(OUT) $(OPT_FLAG); \
	else \
		echo "  [WARN] Native mode is not exposed by ./$(CLI_BIN) yet."; \
		echo "  [HINT] Build legacy binary (./nanlang) or implement --build-native in unified CLI."; \
		exit 1; \
	fi

patch:
	@if [ -x ./nanlang ]; then \
		./nanlang --patch $(FILE) --offset $(OFF) --bytes "$(BYTES)"; \
	else \
		echo "  [WARN] Patch mode is not exposed by ./$(CLI_BIN) yet."; \
		echo "  [HINT] Build legacy binary (./nanlang) or implement --patch in unified CLI."; \
		exit 1; \
	fi

list-libs:
	@ls -1 lib/*.nl 2>/dev/null || echo "  (no .nl libraries in lib/)"

list-examples:
	@ls -1 examples/*.nl 2>/dev/null || echo "  (no .nl examples in examples/)"

compile-example:
	@$(MAKE) compile SRC=$(EXAMPLE) OUT=$(EXAMPLE_OUT)

# ══════════════════════════════════════════════════════════════
# DEMO TARGETS
# ══════════════════════════════════════════════════════════════

demo: demo-all

demo-all: unified
	./$(CLI_BIN) demo all \
	  --count $(COUNT) --pattern $(PATTERN) \
	  --iterations $(ITERATIONS) --rounds $(ROUNDS) \
	  --size $(SIZE) --segments $(SEGMENTS) \
	  --cpu-key $(CPU_KEY) --nibble $(NIBBLE) \
	  --offset $(OFFSET) --len $(LEN)

demo-pulse: unified
	./$(CLI_BIN) demo pulse --count $(COUNT) --pattern $(PATTERN)

demo-sigbuf: unified
	./$(CLI_BIN) demo sigbuf --count $(COUNT)

demo-simd: unified
	./$(CLI_BIN) demo simd --iterations $(ITERATIONS)

demo-dma: unified
	./$(CLI_BIN) demo dma --size $(SIZE)

demo-zerocopy: unified
	./$(CLI_BIN) demo zerocopy --offset $(OFFSET) --len $(LEN)

demo-regmap: unified
	./$(CLI_BIN) demo regmap

demo-binary: unified
	./$(CLI_BIN) demo binary --cpu-key $(CPU_KEY)

demo-compress: unified
	./$(CLI_BIN) demo compress

demo-selfheal: unified
	./$(CLI_BIN) demo selfheal --nibble $(NIBBLE)

demo-jit: unified
	./$(CLI_BIN) demo jit

demo-linker: unified
	./$(CLI_BIN) demo linker --segments $(SEGMENTS) --strip-nops

demo-color: unified
	./$(CLI_BIN) demo color

demo-fold: unified
	./$(CLI_BIN) demo fold

demo-xplatform: unified
	./$(CLI_BIN) demo xplatform

demo-hwfp: unified
	./$(CLI_BIN) demo hwfp

demo-waitfree: unified
	./$(CLI_BIN) demo waitfree --count $(COUNT)

demo-deadlock: unified
	./$(CLI_BIN) demo deadlock

demo-branch: unified
	./$(CLI_BIN) demo branch --rounds $(ROUNDS)

demo-pipeline: unified
	./$(CLI_BIN) demo pipeline

demo-ooo: unified
	./$(CLI_BIN) demo ooo

# ══════════════════════════════════════════════════════════════
# NPI INTEGRATION (v1.1.0)
# ══════════════════════════════════════════════════════════════

npi-clean:
	@rm -rf .npi_cache out/npi* out/*.manifest

npi-deps: $(NPI_BIN)
	@echo "  [NPI] Resolving packages to ./lib ..."
	@./$(NPI_BIN) -r req.txt --build --dry-run || true

npi-build: $(NPI_BIN)
	@echo "  [NPI] CLI built: ./$(NPI_BIN)"

npi-full: $(NPI_BIN)
	@echo "  [NPI] Full pipeline: deps + stage + manifest ..."
	@./$(NPI_BIN) -r req.txt --build --verbose

full: unified $(NPI_BIN)
	./$(CLI_BIN) pkg -r req.txt --dry-run
	./$(CLI_BIN) build --src hello.nl --output out/hello.elf --r req.txt
	@echo "  [OK] Full unified toolchain v4.0.0 ready!
	Usage: ./$(CLI_BIN) build --src hello.nl --output out/hello.elf --r req.txt
	./$(CLI_BIN) pkg -r req.txt --dry-run
	./$(CLI_BIN) compile hello.nl --output out/hello.elf"

install-local:
	@./installer.sh --local --skip-deps

install-system:
	@./installer.sh --skip-deps

# ══════════════════════════════════════════════════════════════
# CLEAN & INFO
# ══════════════════════════════════════════════════════════════

clean:
	@echo "  [CLEAN] Purging build artifacts..."
	rm -f bin/* *.o output.nb temp.nb app.elf *.patched *.gcda *.gcno nan_temp__.cpp
	rm -rf out/ .npi_cache/
	find . -type f -name "*.o" -delete
	@echo "  [OK] Local cleanup done."

info:
	@echo "Compiler : $(CXX)"
	@echo "C++ Bin  : $(CPP_BIN)"
	@echo "CC Bin   : $(CC_BIN)"
	@echo "CLI Bin  : $(CLI_BIN)"
	@echo "NPI Bin  : $(NPI_BIN)"
	@echo "Sources  : $(SRCS_CPP) $(SRCS_CC) $(SRCS_CLI) $(SRCS_NPI)"
	@echo "CXXFlags : $(CXXFLAGS)"
	@echo "CFlags   : $(CFLAGS)"

help:
	@echo ""
	@echo "Targets:"
	@echo "  make [release|debug|fast]"
	@echo "  make preprocess SRC=<.nl> PREPROCESSED=<out/.nl>"
	@echo "  make compile  SRC=<.nl>  OUT=<.elf/.s/.bin/.sh> MODE='--elf|--asm|--bin|--sh'"
	@echo "  make compile-example EXAMPLE=<examples/*.nl> EXAMPLE_OUT=<out/*.elf>"
	@echo "  make run      NB=<.elf|script>"
	@echo "  make native   SRC=<.nl>  OUT=<name>  OPT=1"
	@echo "  make patch    FILE=<bin> OFF=0xNN BYTES='XX YY'"
	@echo "  make demo-<n>"
	@echo "  make demo-all COUNT=128 PATTERN=0xBE ITERATIONS=500"
	@echo "  make list-libs | list-examples"
	@echo "  make install-local | install-system"
	@echo "  make npi-build | npi-deps | npi-full"
	@echo "  make clean | info"

# HER ŞEYİ (Kütüphaneler dahil) fabrikadan çıktığı ana döndürür
mrcleaner: clean npi-clean
	@echo "  [PURGE] Removing staged libraries and logs..."
	rm -rf out/ .npi_cache/
	rm -f npi_manifest.conf
	@echo "  [OK] System is pristine. Zero residue."
