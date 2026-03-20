# NanLang v4.0.3 — "The Absolute Path"

Signal-driven systems programming language with a unified AOT toolchain.

NanLang is built as a **dual-engine pipeline**:

- **Raku** — preprocessor, grammar validation, and pulse-flow lint
- **C++17** — CLI and core AOT compiler (`nanlang`, `nanlang_cpp`, `npi`, `nanlang_cc`)

All compilation is **Ahead-of-Time**. There is no VM and no bytecode runtime.

---

## Highlights

- `.nl` source compilation with enforced Raku preprocessing
- Pure AOT pipeline: source → IR → x86\_64 machine code → output
- Multiple native output formats:
  - `--asm` → x86\_64 assembly text
  - `--bin` → raw machine code payload
  - `--elf` → runnable Linux ELF64 *(default)*
  - `--sh`  → portable shell wrapper with embedded base64 ELF
- GTK3-backed UI scene renderer (`ui-render`, `nan_graphics.nl`)
- Arch Linux packaging under `archrelease/` (release + debug split packages)

---

## Quick Start

### 1. Build

```bash
make unified
./bin/nanlang version
```

For a native-CPU-optimised build:

```bash
make release ARCH_FLAGS="-march=native"
```

### 2. Write and Compile a Program

```bash
cat > app.nl <<'NL'
NAN3;
say "hello";
pulse ^;
halt;
NL

./bin/nanlang compile app.nl --output app.elf
./bin/nanlang run app.elf
```

### 3. All Native Output Modes

```bash
./bin/nanlang compile app.nl --asm --output app.s
./bin/nanlang compile app.nl --bin --output app.bin
./bin/nanlang compile app.nl --elf --output app.elf
./bin/nanlang compile app.nl --sh  --output run_app.sh
```

---

## CLI Reference

```
nanlang help
nanlang version
nanlang compile <src.nl> [--asm|--bin|--elf|--sh] [--output <out>]
nanlang run     <file.elf|shell-script>
nanlang build   --src <src.nl> --output <out> [-r <req.txt>]
nanlang pkg     -r <req.txt> [--dry-run]
nanlang demo    <name> [flags]
nanlang ui-render <scene.nl>
```

Output format flags for `compile` (choose one):

| Flag    | Output                              |
|---------|-------------------------------------|
| `--elf` | Linux ELF64 executable *(default)*  |
| `--asm` | x86\_64 assembly text               |
| `--bin` | Raw machine code                    |
| `--sh`  | Self-extracting shell script        |

`--aot` is accepted for backwards compatibility but is now redundant — AOT is always active.

---

## Makefile Shortcuts

```bash
make unified                                        # build all tools
make compile SRC=hello.nl OUT=out/hello.elf         # default ELF
make compile SRC=hello.nl MODE=--asm OUT=out/hello.s
make run     NB=out/hello.elf
make demo-pulse COUNT=128 PATTERN=0xBE
make demo-all
make clean
make info                                           # show active flags
```

Override architecture at any time:

```bash
make release ARCH_FLAGS="-march=native"             # native-CPU build
make debug                                          # ASan + UBSan + DWARF
```

---

## Pipeline Architecture

```
  app.nl
    │
    ▼
  [Raku preprocessor]          ← grammar validation, pulse-flow lint
    │  out/preprocessed.nl
    ▼
  [C++17 AOT compiler]
    │
    ├─ Parser           → IR (IRFunction / IRBasicBlock / IRInstruction)
    ├─ Register Alloc   → x86_64 register assignment + spill to stack
    ├─ Code Generator   → native x86_64 machine code + string data pool
    └─ ELF Builder      → ELF64 header + PT_LOAD segment + chmod 755
    │
    ▼
  app.elf  /  app.s  /  app.bin  /  run_app.sh
```

- `nanlang compile` always runs the Raku preprocessor first.
- `nanlang run` executes native artifacts (`.elf` or shell wrappers). VM execution is permanently removed.
- Strings are embedded via a **RIP-relative data pool** appended after the code section — no runtime allocation needed.

---

## NanLang Graphics Toolkit

A declarative GTK3-backed UI layer is available via top-level `ui` directives.
It uses concatenated scene files rather than event-loop callbacks, keeping the
toolchain linear and compiler-friendly.

**Files:**
- `lib/nan_graphics.nl` — window/widget bootstrap prelude
- `examples/gui_test_interface.nl` — chart and frame scene
- `docs/NAN_GRAPHICS.md` — full directive reference

**Quick demo:**

```bash
cat lib/nan_graphics.nl examples/gui_test_interface.nl > out/gui_demo.nl
./bin/nanlang compile out/gui_demo.nl --output out/gui_demo.elf
./bin/nanlang run out/gui_demo.elf
```

Or render a scene directly without compiling:

```bash
./bin/nanlang ui-render out/gui_demo.nl
```

Supported directives include `ui app begin/end`, `ui window create`, `ui label`,
`ui button`, `ui slider`, `ui chart begin/point/commit`, and `ui frame present`.
See `docs/NAN_GRAPHICS.md` for the full list.

---

## Arch Linux Package

```bash
cd archrelease
rm -rf src pkg *.pkg.tar.zst
makepkg -si
```

Two packages are produced:

| Package | Contents |
|---|---|
| `nanlang-git` | Stripped release binaries under `/usr/bin/` |
| `nanlang-git-debug` | Full DWARF symbols + ASan/UBSan under `/usr/bin/*-debug` |

Via AUR helper:

```bash
yay -S nanlang-git
```

---

## Repository Layout

```
src/            C++17 core implementation
include/        C++ headers
scripts/        Raku preprocessor and benchmark tooling
lib/            NanLang standard libraries (including nan_graphics.nl)
examples/       Example .nl programs
kernel/         Low-level runtime components
archrelease/    Arch Linux PKGBUILD + packaging artifacts
docs/           Extended reference documentation
cmd/            Legacy (Go CLI removed in v4.0.0 C++ migration)
internal/       Legacy (Go internals removed in v4.0.0 C++ migration)
```

---

## Installer / Uninstaller

```bash
./installer.sh          # system-wide install
./installer.sh --local  # local install (no sudo)
./uninstaller.sh
```

---

## License

GPL-3.0. See `LICENSE`.

---

## More Documentation

Full language reference, internals, benchmarks, and philosophy are in `docs/`.

| File | Contents |
|---|---|
| `docs/README.md` | Full language reference |
| `docs/SPECIFICATION.md` | Formal language specification |
| `docs/INTERNALS.md` | Compiler internals |
| `docs/BENCHMARKS.md` | Performance data |
| `docs/NAN_GRAPHICS.md` | Graphics toolkit directive reference |
| `docs/PHILOSOPHY.md` | Design philosophy |
| `docs/LEARNING.md` | Tutorial and learning guide |
| `docs/GLOSSARY.md` | Term glossary |
