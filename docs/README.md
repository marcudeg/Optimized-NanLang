# NanLang — Complete Reference & Wiki

> **NanLang v3.0.2** | Signal-Driven Systems Language | High-Performance Toolchain

---

## Table of Contents

1. [Overview](#1-overview)
2. [Philosophy & Design Goals](#2-philosophy--design-goals)
3. [Architecture Overview](#3-architecture-overview)
4. [Getting Started](#4-getting-started)
   - 4.1 [Prerequisites](#41-prerequisites)
   - 4.2 [Building the Toolchain](#42-building-the-toolchain)
   - 4.3 [Hello, World!](#43-hello-world)
5. [Language Reference](#5-language-reference)
   - 5.1 [Syntax Basics](#51-syntax-basics)
   - 5.2 [Types](#52-types)
   - 5.3 [Operators](#53-operators)
   - 5.4 [Control Flow](#54-control-flow)
   - 5.5 [Functions](#55-functions)
   - 5.6 [Signal Literals](#56-signal-literals)
   - 5.7 [Keywords Reference Table](#57-keywords-reference-table)
6. [Compilation Pipeline](#6-compilation-pipeline)
   - 6.1 [Source → Bytecode](#61-source--bytecode)
   - 6.2 [Bytecode → Native Binary](#62-bytecode--native-binary)
   - 6.3 [Optimization Passes](#63-optimization-passes)
   - 6.4 [The `.nb` Bytecode Format](#64-the-nb-bytecode-format)
7. [CLI Reference](#7-cli-reference)
   - 7.1 [Compilation Commands](#71-compilation-commands)
   - 7.2 [Runtime Commands](#72-runtime-commands)
   - 7.3 [Patching Commands](#73-patching-commands)
   - 7.4 [Demo Commands](#74-demo-commands)
   - 7.5 [Utility Flags](#75-utility-flags)
8. [Makefile Targets](#8-makefile-targets)
9. [Instruction Set Architecture (Nan-ISA)](#9-instruction-set-architecture-nan-isa)
   - 9.1 [Opcode Table](#91-opcode-table)
   - 9.2 [Encoding Format](#92-encoding-format)
   - 9.3 [Instruction Skip Logic](#93-instruction-skip-logic)
10. [Memory Model](#10-memory-model)
    - 10.1 [Linear Memory Pool](#101-linear-memory-pool)
    - 10.2 [Cache-Line Alignment](#102-cache-line-alignment)
    - 10.3 [Zero-Copy Slices](#103-zero-copy-slices)
    - 10.4 [Huge Page Support](#104-huge-page-support)
    - 10.5 [DMA Pulse Transfers](#105-dma-pulse-transfers)
11. [Signal System](#11-signal-system)
    - 11.1 [Signal States](#111-signal-states)
    - 11.2 [Potentials](#112-potentials)
    - 11.3 [Signal Packets (AVX2)](#113-signal-packets-avx2)
    - 11.4 [Async Pulse Emitter](#114-async-pulse-emitter)
    - 11.5 [Signal Buffer](#115-signal-buffer)
    - 11.6 [Clock-Cycle Synchronizer](#116-clock-cycle-synchronizer)
12. [SIMD Engine](#12-simd-engine)
    - 12.1 [SIMDProcessor API](#121-simdprocessor-api)
    - 12.2 [AVX2 vs Scalar Fallback](#122-avx2-vs-scalar-fallback)
    - 12.3 [Non-Temporal Stores](#123-non-temporal-stores)
13. [Compiler Internals](#13-compiler-internals)
    - 13.1 [Constant Folding](#131-constant-folding)
    - 13.2 [Branch Resolution](#132-branch-resolution)
    - 13.3 [Register Coloring](#133-register-coloring)
    - 13.4 [Binary Size Minimizer](#134-binary-size-minimizer)
    - 13.5 [Nan-Linker v2.0](#135-nan-linker-v20)
    - 13.6 [Cross-Platform Emitter](#136-cross-platform-emitter)
14. [JIT Compilation](#14-jit-compilation)
    - 14.1 [How JIT Works in NanLang](#141-how-jit-works-in-nanlang)
    - 14.2 [JITPulseGen API](#142-jitpulsegen-api)
    - 14.3 [W^X Policy & mprotect](#143-wx-policy--mprotect)
    - 14.4 [Platform Support](#144-platform-support)
15. [Performance Subsystem](#15-performance-subsystem)
    - 15.1 [Pipeline Reordering](#151-pipeline-reordering)
    - 15.2 [Out-of-Order Execution (OoO)](#152-out-of-order-execution-ooo)
    - 15.3 [Branch Predictor](#153-branch-predictor)
    - 15.4 [Wait-Free Queue](#154-wait-free-queue)
    - 15.5 [Deadlock Detection](#155-deadlock-detection)
    - 15.6 [Frequency Scaling Hints](#156-frequency-scaling-hints)
    - 15.7 [Minimalist Context Switching](#157-minimalist-context-switching)
16. [Security Features](#16-security-features)
    - 16.1 [ManagedString — Volatile Sanitization](#161-managedstring--volatile-sanitization)
    - 16.2 [Jitter Injection](#162-jitter-injection)
    - 16.3 [Hardware Fingerprinting](#163-hardware-fingerprinting)
    - 16.4 [Hex Obfuscation (XOR Key)](#164-hex-obfuscation-xor-key)
17. [Binary Operations](#17-binary-operations)
    - 17.1 [Binary Patching](#171-binary-patching)
    - 17.2 [RLE Compression](#172-rle-compression)
    - 17.3 [Self-Healing Binaries (Hamming)](#173-self-healing-binaries-hamming)
    - 17.4 [ELF Generation](#174-elf-generation)
    - 17.5 [Resource Embedding](#175-resource-embedding)
18. [Kernel Scheduler](#18-kernel-scheduler)
19. [VS Code Extension](#19-vs-code-extension)
20. [Register Map Reference](#20-register-map-reference)
21. [Demo Gallery](#21-demo-gallery)
22. [Frequently Asked Questions (FAQ)](#22-frequently-asked-questions-faq)
23. [Troubleshooting](#23-troubleshooting)
24. [Contributing](#24-contributing)
25. [Glossary](#25-glossary)
26. [License](#26-license)

---

## 1. Overview

**NanLang** is a compiled, statically typed systems programming language engineered for environments where determinism, low-latency, and direct hardware control are non-negotiable requirements. It occupies a niche between C and domain-specific hardware description languages — offering the ergonomics of a high-level syntax while compiling down to an extremely compact bytecode format (`.nb`) that runs on a purpose-built virtual machine, or alternatively straight to native ELF binaries.

The language was designed from scratch around three axioms:

1. **Everything is a signal.** Data in NanLang is fundamentally modeled as electrical states — high (`^`), low (`_`), or uncertain (`?`). This maps naturally to hardware register values, interrupt lines, and bus transactions.
2. **Timing is a first-class concern.** The runtime exposes CPU cycle counters (RDTSC), spin-wait primitives, and SIMD intrinsics directly to the programmer, enabling nanosecond-precision scheduling.
3. **Security through erasure.** Sensitive data — cryptographic keys, authentication tokens, personal identifiers — is automatically overwritten with zeroes when it goes out of scope, with `volatile` writes to prevent compiler elision.

NanLang v3.0.2 ships with a fully integrated toolchain: a source-to-bytecode compiler, a bytecode virtual machine, a JIT engine, a binary patcher, and a rich set of demo programs that exercise every subsystem.

---

## 2. Philosophy & Design Goals

### Deterministic Execution

Non-determinism in systems software is a reliability hazard. NanLang achieves determinism through:

- A **pre-allocated linear memory pool** (default 8 MB) that eliminates heap fragmentation and runtime `malloc` calls during signal processing.
- **Spinlock-based synchronization** rather than OS mutexes, avoiding unpredictable context-switch latencies.
- **Static branch resolution** at compile time wherever the condition can be proven constant, eliminating speculative mispredictions.

### Minimal Binary Footprint

Every compilation pass runs through the **Binary Size Minimizer**, which chains three reduction stages: NOP stripping, constant folding, and RLE compression. The goal is that each successive build of the same source should produce an equal or smaller output.

### Hardware-First

NanLang does not abstract the hardware away — it exposes it. AVX2 SIMD vectors, CPUID fingerprinting, `mmap`/`munmap` with `MAP_LOCKED`, non-temporal stores, and `RDTSCP` cycle counters are all first-class citizens of the standard library (`nan_arch.h`, `nan_pulse.h`).

### Security by Default

The `ManagedString` type performs a `volatile` zero-wipe on every byte of its storage upon destruction. The `security_delay()` function in the execution engine introduces a randomized NOP loop (0–31 iterations) to defeat timing side-channel analysis. Hardware-keyed XOR obfuscation ties bytecode to a specific CPU's identity.

---

## 3. Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    NanLang Source (.nl)                  │
└────────────────────────┬────────────────────────────────┘
                         │  NanCompiler::generate_bytecode()
                         ▼
┌─────────────────────────────────────────────────────────┐
│                   Bytecode (.nb)                         │
│  Magic: 0x214E414E  |  Opcodes  |  Operands             │
└──────────┬──────────────────────┬───────────────────────┘
           │ NanRuntime::run()    │ NanCompiler::build_native()
           ▼                      ▼
┌──────────────────┐   ┌──────────────────────────────────┐
│   NanLang VM     │   │   Native ELF Binary (.elf)        │
│  (Interpreter)   │   │   g++ backend  |  JITPulseGen     │
└──────────────────┘   └──────────────────────────────────┘
           │
           ▼
┌─────────────────────────────────────────────────────────┐
│               ExecutionEngine                            │
│  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐  │
│  │ RegisterMap  │  │ LinearMemory  │  │ SignalBus   │  │
│  └──────────────┘  └───────────────┘  └─────────────┘  │
└─────────────────────────────────────────────────────────┘
```

The toolchain is divided into six internal modules:

| Module            | Header              | Purpose                                          |
|-------------------|---------------------|--------------------------------------------------|
| Core Types        | `nan_core.h`        | `Potential`, `Signal`, `SignalPacket`, `ManagedString` |
| Pulse Engine      | `nan_pulse.h`       | `AsyncPulseEmitter`, `ClockCycleSync`, `SignalBuffer` |
| Binary Ops        | `nan_binary.h`      | Opcodes, patching, compression, Hamming, ELF     |
| Architecture      | `nan_arch.h`        | SIMD, DMA, zero-copy, register map, huge pages   |
| Compiler          | `nan_compiler.h`    | JIT, linker, coloring, constant folding, cross-platform |
| Performance       | `nan_perf.h`        | Pipeline reorder, OoO, branch predictor, wait-free, deadlock |

---

## 4. Getting Started

### 4.1 Prerequisites

| Requirement       | Minimum Version | Notes                                        |
|-------------------|-----------------|----------------------------------------------|
| GCC or Clang      | GCC 9+ / Clang 10+ | Must support C++17 and AVX2 intrinsics    |
| Linux Kernel      | 5.0+            | `mmap` with `MAP_LOCKED`, `MAP_HUGETLB`      |
| CPU               | Any with AVX2   | Intel Haswell (2013)+ or AMD Ryzen (2017)+   |
| GNU Make          | 4.0+            | For Makefile-based builds                    |
| binutils          | Any recent      | For `strip --strip-debug`                    |

Optional:
- **Visual Studio Code** — for syntax highlighting via the bundled extension (`install_vscode_ext.sh`)
- **Valgrind / AddressSanitizer** — automatically enabled in `make debug`

### 4.2 Building the Toolchain

**Quick build (release mode):**
```bash
make
# or equivalently:
make release
```

**Debug build (AddressSanitizer + UBSanitizer enabled):**
```bash
make debug
```

**PGO-instrumented fast build:**
```bash
make fast
```

**Manual build without Make:**
```bash
./build.sh
```

The build system compiles the following translation units in order:

```
main.cpp
src/engine.cpp
src/pulse_emitter.cpp
src/binary_ops.cpp
src/arch_ops.cpp
src/compiler_ops.cpp
src/perf_ops.cpp
kernel/scheduler.cpp
```

The resulting binary is placed at `./nanlang`.

### 4.3 Hello, World!

Create a file `hello.nl`:
```
fn main() -> void {
    say "Hello, World!";
}
```

Compile to bytecode and run:
```bash
./nanlang --compile hello.nl --output hello.nb
./nanlang --run hello.nb
```

Or compile directly to a native binary:
```bash
./nanlang --build-native hello.nl --output hello --opt
./hello
```

Using Make shortcuts:
```bash
make compile SRC=hello.nl OUT=hello.nb
make run     NB=hello.nb
make native  SRC=hello.nl OUT=hello OPT=1
```

---

## 5. Language Reference

### 5.1 Syntax Basics

NanLang syntax is inspired by Rust and C, with curly-brace blocks, semicolon-terminated statements, and `->` return type annotation. Comments follow C++ conventions.

```nanlang
// Single-line comment

/*
   Multi-line comment
*/

fn add(a: potential, b: potential) -> potential {
    let result: potential = a + b;
    return result;
}
```

Identifiers follow `[a-zA-Z_][a-zA-Z0-9_]*`. Keywords are lowercase and reserved.

### 5.2 Types

NanLang has five built-in types:

| Type        | Description                                             | C++ Equivalent         |
|-------------|---------------------------------------------------------|------------------------|
| `potential` | Unsigned 8-bit energy level (0–255)                     | `uint8_t`              |
| `signal`    | Binary state: High (`^`) or Low (`_`)                   | `enum Signal : uint8_t`|
| `flow`      | Ordered sequence of signals                             | `std::vector<Signal>`  |
| `str`       | Security-managed string with volatile zero-wipe         | `ManagedString`        |
| `shadow`    | Opaque byte buffer (raw memory region)                  | `std::vector<uint8_t>` |

The `void` keyword is used exclusively for functions that return no value.

### 5.3 Operators

**Arithmetic operators** (operate on `potential` values):

| Operator | Description         |
|----------|---------------------|
| `+`      | Addition            |
| `-`      | Subtraction         |
| `*`      | Multiplication      |
| `/`      | Integer division    |
| `==`     | Equality test       |
| `!=`     | Inequality test     |
| `<`      | Less than           |
| `>`      | Greater than        |

**Signal operators** (operate on `signal` values):

| Operator | Description                        |
|----------|------------------------------------|
| `^&^`    | Signal AND (both must be High)     |
| `_?_`    | Signal uncertainty merge           |
| `>>>`    | Signal shift / propagation         |

**Assignment:**

| Operator | Description              |
|----------|--------------------------|
| `=`      | Bind value to identifier |
| `@`      | Hardware register bind   |

### 5.4 Control Flow

```nanlang
// Conditional
if condition {
    // ...
} else {
    // ...
}

// Loop (infinite, break to exit)
loop {
    if done {
        break;
    }
}

// Interrupt handler
on interrupt(0x20) {
    // handle timer interrupt
}
```

The `halt` keyword terminates execution immediately, equivalent to `OP_HALT` in the bytecode VM.

### 5.5 Functions

Functions are declared with `fn`, take typed parameters, and must specify a return type:

```nanlang
fn multiply(x: potential, y: potential) -> potential {
    return x * y;
}

fn emit_signal(line: signal) -> void {
    pulse line;
}
```

Functions may also declare error handlers inline:

```nanlang
fn risky_read(addr: potential) -> potential {
    on Error {
        return 0;
    }
    return @addr;
}
```

### 5.6 Signal Literals

NanLang has dedicated syntax for signal states:

| Literal | Meaning                  | Bytecode Representation |
|---------|--------------------------|-------------------------|
| `^`     | High signal (logic 1)    | `0x01`                  |
| `_`     | Low signal (logic 0)     | `0x00`                  |
| `?`     | Uncertain / floating     | `0xFF`                  |
| `_>_`   | Falling edge transition  | `0x10`                  |
| `_>>_`  | Double falling edge      | `0x11`                  |

### 5.7 Keywords Reference Table

| Keyword     | Category          | Description                                    |
|-------------|-------------------|------------------------------------------------|
| `fn`        | Declaration       | Function declaration                           |
| `let`       | Declaration       | Variable binding                               |
| `say`       | I/O               | Print string to stdout                         |
| `if`        | Control Flow      | Conditional branch                             |
| `else`      | Control Flow      | Alternative branch                             |
| `loop`      | Control Flow      | Unconditional loop                             |
| `break`     | Control Flow      | Exit loop                                      |
| `return`    | Control Flow      | Return from function                           |
| `halt`      | Control Flow      | Terminate execution                            |
| `pulse`     | Signal            | Emit a signal on a line                        |
| `emit`      | Signal            | Enqueue signal to the async emitter            |
| `jitter`    | Security          | Insert randomized timing noise                 |
| `purge`     | Security          | Immediately zero a memory region               |
| `on`        | Event             | Register an event or interrupt handler         |
| `interrupt` | Event             | Specify interrupt vector                       |
| `Error`     | Error Handling    | Error handler scope                            |
| `void`      | Type              | No-return type annotation                      |
| `potential` | Type              | 8-bit energy level (unsigned)                  |
| `signal`    | Type              | Binary state                                   |
| `flow`      | Type              | Signal sequence                                |
| `str`       | Type              | Secure managed string                          |
| `shadow`    | Type              | Raw byte buffer                                |

---

## 6. Compilation Pipeline

### 6.1 Source → Bytecode

The `NanCompiler::generate_bytecode()` method performs a single-pass lexical scan of the `.nl` source file. The current compiler supports the following transformations:

- `say "..."` statements are compiled to `OP_SAY` + 4-byte length prefix + inline UTF-8 string data.
- All other statements are reserved for future instruction set expansion.

The bytecode file begins with a 4-byte magic number: `0x214E414E` (ASCII `!NAN`). The VM validates this header before executing.

**Compilation example:**
```
Source:        say "Hello";
Bytecode:      02 05 00 00 00 48 65 6C 6C 6F
               │  └──────────┘ └────────────┘
               OP_SAY  len=5      "Hello"
```

### 6.2 Bytecode → Native Binary

`NanCompiler::build_native()` transpiles NanLang source to a temporary C++ stub, then invokes `g++` to produce a native ELF binary. With `--opt`, the following g++ flags are applied:

```
-O3 -march=native -flto
```

This approach trades compilation speed for maximum runtime performance, leveraging the host compiler's full optimization machinery (loop vectorization, inlining, link-time optimization).

### 6.3 Optimization Passes

The NanLang compiler applies three optimization passes in sequence:

**Pass 1 — NOP Stripping (`strip_nops`)**
All `0x00` (NOP) bytes are removed from the bytecode stream. NOPs are commonly inserted as padding during segment alignment and serve no semantic purpose in the final binary.

**Pass 2 — Constant Folding (`ConstantFolder::fold_bytecode`)**
Sequences of the form `[ADD|SUB|XOR] [imm1] [imm2]` are evaluated at compile time and replaced with `LOAD [result]`. This eliminates 3-byte arithmetic sequences and replaces them with 2-byte loads.

```
Before:  ADD 0x0A 0x14    (3 bytes)
After:   LOAD 0x1E        (2 bytes)
```

**Pass 3 — RLE Compression**
The resulting bytecode is run-length encoded. Consecutive identical bytes `[val, val, val, ...]` become `[count, val]` pairs, which the runtime decompresses on load.

### 6.4 The `.nb` Bytecode Format

```
Offset   Size   Description
------   ----   -----------
0x00     4      Magic: 0x21 0x4E 0x41 0x4E  ("!NAN")
0x04     1      Opcode (see §9.1)
0x05     N      Operands (opcode-dependent)
...      ...    Additional instructions
```

The format is intentionally minimal — there are no section headers, no symbol tables, no relocation entries. The VM executes opcodes in linear order from byte 4 onward.

---

## 7. CLI Reference

The `nanlang` binary is the unified entry point for all toolchain operations. It accepts `--key value` pairs and `--flag` boolean switches.

### 7.1 Compilation Commands

```bash
# Compile .nl source to .nb bytecode
./nanlang --compile <file.nl> [--output <out.nb>]

# Default output filename: output.nb
./nanlang --compile hello.nl
./nanlang --compile hello.nl --output program.nb

# Build a native ELF binary
./nanlang --build-native <file.nl> [--output <name>] [--opt]

# --opt enables -O3 -march=native -flto
./nanlang --build-native hello.nl --output hello --opt
```

### 7.2 Runtime Commands

```bash
# Execute a compiled .nb bytecode file
./nanlang --run <file.nb>

./nanlang --run output.nb
./nanlang --run program.nb
```

### 7.3 Patching Commands

```bash
# Apply a binary patch at the given hex offset
./nanlang --patch <file> --offset <hex_offset> --bytes "<hex bytes>"

# Example: write NOP (0x90) at offset 0x100 in ./nanlang
./nanlang --patch ./nanlang --offset 0x100 --bytes "90"

# Write multiple bytes
./nanlang --patch ./program --offset 0x200 --bytes "DE AD BE EF"
```

The patcher reads the entire target file into memory, modifies the bytes at the specified offset, and writes the result to `<file>.patched`. The original file is never modified.

### 7.4 Demo Commands

All demo subsystems are accessible via `--demo <name>`. Parameters are passed as additional `--key value` flags.

```bash
./nanlang --demo <name> [parameters]

# Run all demos
./nanlang --demo all

# Run a specific demo
./nanlang --demo pulse --count 128 --pattern 0xBE
./nanlang --demo simd --iterations 500
./nanlang --demo branch --rounds 1024
```

See [§21 Demo Gallery](#21-demo-gallery) for a full list with parameter descriptions.

### 7.5 Utility Flags

```bash
./nanlang --help       # Print full CLI help
./nanlang --version    # Print version string ("NanLang v3.0.2")
```

---

## 8. Makefile Targets

The Makefile exposes a rich set of phony targets for every workflow:

### Build Targets

| Target    | Description                                                       |
|-----------|-------------------------------------------------------------------|
| `make` or `make release` | Full release build with `-O3 -march=native -flto` |
| `make debug`   | Debug build with AddressSanitizer and UBSanitizer            |
| `make fast`    | PGO-instrumented build (`-fprofile-generate`)                |
| `make clean`   | Remove all build artifacts, `.nb` files, patched binaries    |

### User Command Targets

| Target           | Equivalent CLI                                          |
|------------------|---------------------------------------------------------|
| `make compile`   | `./nanlang --compile $(SRC) --output $(OUT)`           |
| `make run`       | `./nanlang --run $(NB)`                                |
| `make native`    | `./nanlang --build-native $(SRC) --output $(OUT) [--opt]` |
| `make patch`     | `./nanlang --patch $(FILE) --offset $(OFF) --bytes "$(BYTES)"` |

Variables can be overridden on the command line:
```bash
make compile SRC=main.nl OUT=main.nb
make native  SRC=main.nl OUT=main OPT=1
make patch   FILE=./main OFF=0x10 BYTES="90 90"
```

### Demo Targets

| Target                | Description                    |
|-----------------------|--------------------------------|
| `make demo`           | Run all demos                  |
| `make demo-pulse`     | Async pulse emitter demo       |
| `make demo-sigbuf`    | Signal buffer demo             |
| `make demo-simd`      | AVX2 SIMD demo                 |
| `make demo-dma`       | DMA transfer demo              |
| `make demo-zerocopy`  | Zero-copy slice demo           |
| `make demo-regmap`    | Register map demo              |
| `make demo-binary`    | Obfuscation demo               |
| `make demo-compress`  | RLE compression demo           |
| `make demo-selfheal`  | Hamming self-healing demo      |
| `make demo-jit`       | JIT compilation demo           |
| `make demo-linker`    | Linker demo                    |
| `make demo-color`     | Register coloring demo         |
| `make demo-fold`      | Constant folding demo          |
| `make demo-xplatform` | Cross-platform emitter demo    |
| `make demo-hwfp`      | Hardware fingerprint demo      |
| `make demo-waitfree`  | Wait-free queue demo           |
| `make demo-deadlock`  | Deadlock detection demo        |
| `make demo-branch`    | Branch predictor demo          |
| `make demo-pipeline`  | Pipeline reorder demo          |
| `make demo-ooo`       | Out-of-order execution demo    |

Demo parameters can be overridden globally:
```bash
make demo-all COUNT=128 PATTERN=0xBE ITERATIONS=500 ROUNDS=1024
```

### Info Target

```bash
make info
# Prints:
# Compiler : g++
# Target   : nanlang
# Sources  : main.cpp src/engine.cpp ...
# Flags    : -std=c++17 -O3 ...
```

---

## 9. Instruction Set Architecture (Nan-ISA)

### 9.1 Opcode Table

The Nan-ISA reduces the most common instructions to a 1-byte encoding, minimizing bytecode size and VM dispatch overhead.

| Opcode | Hex  | Operands       | Description                           |
|--------|------|----------------|---------------------------------------|
| NOP    | 0x00 | none           | No operation (padding)                |
| HALT   | 0x01 | none           | Stop execution                        |
| SAY    | 0x02 | u32 len, data  | Print string of `len` bytes           |
| LOAD   | 0x03 | u8 imm         | Load immediate value into accumulator |
| STORE  | 0x04 | u8 addr        | Store accumulator to address          |
| ADD    | 0x05 | u8 a, u8 b     | Add two immediates                    |
| SUB    | 0x06 | u8 a, u8 b     | Subtract two immediates               |
| JMP    | 0x07 | u32 target     | Unconditional jump                    |
| JZ     | 0x08 | u32 target     | Jump if accumulator is zero           |
| XOR    | 0x09 | u8 a, u8 b     | XOR two immediates                    |
| OR     | 0x0A | u8 a, u8 b     | OR two immediates                     |
| AND    | 0x0B | u8 a, u8 b     | AND two immediates                    |
| SHL    | 0x0C | u8 a, u8 shift | Shift left                            |
| SHR    | 0x0D | u8 a, u8 shift | Shift right                           |
| CALL   | 0x0E | u32 target     | Call subroutine                       |
| RET    | 0x0F | none           | Return from subroutine                |

### 9.2 Encoding Format

Fixed-width operands are always little-endian. The `SAY` instruction uses a variable-length encoding:

```
[0x02] [len:u32 LE] [utf8_bytes...]
```

All other binary instructions use exactly 3 bytes (`[opcode] [operand1] [operand2]`), enabling the constant folder to scan for foldable patterns in a single pass.

### 9.3 Instruction Skip Logic

The `strip_nops()` function removes all `0x00` bytes from a bytecode stream. This is safe because NOPs are guaranteed to have no observable side effects and are only used for alignment padding by the linker.

```cpp
std::vector<uint8_t> strip_nops(const std::vector<uint8_t>& bytecode);
```

The `NanLinker` automatically strips NOPs when `strip_padding = true` (the default).

---

## 10. Memory Model

### 10.1 Linear Memory Pool

The `ExecutionEngine` pre-allocates a contiguous 8 MB buffer at startup:

```cpp
linear_memory.resize(8 * 1024 * 1024);
```

All signal buffers, string storage, and register spill slots draw from this pool. No `malloc` or `new` calls are made during execution, eliminating heap fragmentation, GC pauses, and the associated timing non-determinism.

### 10.2 Cache-Line Alignment

The `CacheAligned<T>` template wrapper forces any value onto a 64-byte cache boundary, preventing false sharing in multi-threaded scenarios:

```cpp
template<typename T>
struct alignas(64) CacheAligned {
    T value{};
};
```

The `alloc_aligned(size, align)` / `free_aligned(ptr)` functions wrap `posix_memalign` (Linux) and `_aligned_malloc` (Windows) for portable aligned heap allocation.

### 10.3 Zero-Copy Slices

`ZeroCopySlice` holds a `const uint8_t*` pointer and a `size_t` length — no data is copied. Sub-slices are created by advancing the pointer:

```cpp
ZeroCopySlice full(data);
ZeroCopySlice slice = full.sub(16, 8);  // bytes [16..24), no copy
```

This is used extensively in the binary patching and compression subsystems to avoid unnecessary allocations when scanning bytecode streams.

### 10.4 Huge Page Support

The `HugePageAlloc` struct requests 2 MB transparent huge pages via `mmap` with `MAP_HUGETLB`:

```cpp
uint8_t* buf = HugePageAlloc::alloc(64 * 1024 * 1024); // 64 MB
```

Huge pages reduce TLB pressure significantly for large signal buffers. If `MAP_HUGETLB` fails (e.g., the system has no reserved huge pages), the allocator silently falls back to a regular anonymous `mmap`.

To pre-reserve huge pages on Linux:
```bash
echo 64 | sudo tee /proc/sys/vm/nr_hugepages
```

### 10.5 DMA Pulse Transfers

The `DMAPulse` struct simulates direct memory access by using `mmap(MAP_LOCKED)` to pin pages in physical RAM, then copying with non-temporal AVX2 stores that bypass L1/L2:

```cpp
uint8_t* src = DMAPulse::map_region(size);
uint8_t* dst = DMAPulse::map_region(size);
DMAPulse::nt_store(dst, src, size);
DMAPulse::unmap_region(src, size);
DMAPulse::unmap_region(dst, size);
```

On non-Linux platforms, `map_region` falls back to `malloc`.

---

## 11. Signal System

### 11.1 Signal States

The core `Signal` enum models binary logic:

```cpp
enum class Signal : uint8_t { Low = 0, High = 1 };
```

Using `enum class` prevents accidental arithmetic on signal values, enforcing type safety throughout the codebase.

### 11.2 Potentials

`Potential` is a typedef for `uint8_t`, representing an 8-bit energy level (0–255). This models hardware voltage levels, PWM duty cycles, or any analog quantity quantized to 256 levels.

### 11.3 Signal Packets (AVX2)

`SignalPacket` wraps an `__m256i` register (256 bits = 32 signals in parallel):

```cpp
struct SignalPacket {
    __m256i data;
    void merge(const SignalPacket& other) {
        data = _mm256_or_si256(data, other.data);
    }
};
```

The `merge()` operation is a hardware-accelerated bitwise OR that combines two 256-bit signal vectors in a single instruction (~1 CPU cycle latency on modern x86).

### 11.4 Async Pulse Emitter

`AsyncPulseEmitter` is a lock-free, non-blocking signal emitter backed by a 1024-byte cache-aligned circular buffer:

```cpp
AsyncPulseEmitter emitter;
emitter.emit(0xAA);     // write signal, returns immediately
size_t pos = emitter.pos();
emitter.flush();        // zero buffer and reset write pointer
```

`emit()` uses `std::atomic::fetch_add` with `memory_order_relaxed` for the position counter, followed by `_mm_sfence()` to order the store. This makes it safe for a single producer without a mutex.

### 11.5 Signal Buffer

`SignalBuffer` is a statically sized FIFO (512 bytes, cache-aligned) for ordered signal delivery:

```cpp
SignalBuffer buf;
buf.push(0xFF);
uint8_t v;
buf.pop(v);
```

Head and tail indices use modular arithmetic, making all operations branch-free and O(1).

### 11.6 Clock-Cycle Synchronizer

`ClockCycleSync` uses the `RDTSCP` instruction to read the CPU's time-stamp counter with serialization:

```cpp
ClockCycleSync clock;
clock.calibrate();       // record baseline TSC
// ... do work ...
uint64_t cycles = clock.elapsed();   // TSC delta

ClockCycleSync::wait_cycles(10000);  // spin for exactly N cycles
```

TSC frequency varies by CPU; divide by `(CPU GHz × 10^9)` to convert cycles to nanoseconds.

---

## 12. SIMD Engine

### 12.1 SIMDProcessor API

```cpp
// XOR 32 bytes in one AVX2 instruction
SIMDProcessor::xor32(a, b, out);

// OR 32 bytes in one AVX2 instruction
SIMDProcessor::or32(a, b, out);

// Count non-zero bytes (popcount-style)
int active = SIMDProcessor::count_active(data, len);
```

All three functions require 32-byte aligned input (use `alignas(32)` on stack buffers).

### 12.2 AVX2 vs Scalar Fallback

Every SIMD function is guarded by `#ifdef __AVX2__`. On CPUs without AVX2, the code automatically falls back to a scalar byte-by-byte loop with identical semantics. This ensures the toolchain compiles and runs correctly on older hardware, albeit slower.

To explicitly disable AVX2 for testing:
```bash
make release CXXFLAGS="-std=c++17 -O2 -I./include -pthread"
```

### 12.3 Non-Temporal Stores

`DMAPulse::nt_store()` uses `_mm256_stream_si256` to write data directly to DRAM without loading cache lines first. This is critical for large transfers (>= several MB) where writing through the cache would evict useful data:

```
Normal store:  [CPU] → [L1] → [L2] → [L3] → [DRAM]
Non-temporal:  [CPU] ─────────────────────→ [DRAM]
```

An `_mm_sfence()` after the non-temporal stores ensures all writes are globally visible before the function returns.

---

## 13. Compiler Internals

### 13.1 Constant Folding

`ConstantFolder::fold_bytecode()` scans the bytecode array for three-byte patterns `[op, imm1, imm2]` where `op` is one of `ADD`, `SUB`, or `XOR`. Each such pattern is replaced with `[LOAD, result]`:

```
Input:  [0x05, 0x0A, 0x14]   // ADD 10, 20
Output: [0x03, 0x1E]          // LOAD 30
```

This reduces code size by 1 byte per folded instruction and eliminates the runtime arithmetic dispatch.

### 13.2 Branch Resolution

`BranchResolver` holds a table of `Branch` entries, each specifying a bytecode offset, a pre-known boolean condition, and two target addresses. At link time, `resolve()` replaces conditional jumps with unconditional `JMP` instructions pointing directly to the statically chosen target:

```cpp
BranchResolver br;
br.add(/*offset=*/4, /*cond=*/true, /*t_true=*/12, /*t_false=*/24);
auto resolved = br.resolve(bytecode);
```

This eliminates all conditional branch instructions from the final bytecode for paths whose conditions are compile-time constants.

### 13.3 Register Coloring

`RegisterColoring` implements a greedy graph-coloring algorithm over the variable interference graph. Variables that are live at the same time conflict; the greedy pass assigns each variable the lowest-numbered register not used by any of its neighbors.

```cpp
RegisterColoring rc;
int x = rc.add_var("x");
int y = rc.add_var("y");
rc.add_conflict(x, y);   // x and y are live simultaneously
bool ok = rc.color();    // ok == true if K=16 registers suffice
rc.dump();               // print: var=x -> R0  var=y -> R1
```

If the graph cannot be colored with K=16 registers, `color()` returns `false` and a register spill to RAM is required.

### 13.4 Binary Size Minimizer

`SizeMinimizer::minimize()` chains all three reduction passes (NOP strip → constant fold → RLE compress) and tracks a `baseline` size. If the compressed output is not smaller than the previous run's baseline, it prints a warning:

```
[SIZE] Binary: 512 -> compressed: 38 bytes
[SIZE] Warning: size did not shrink!
```

### 13.5 Nan-Linker v2.0

`NanLinker` accepts multiple bytecode segments (e.g., from separate compilation units) and concatenates them, optionally stripping NOP padding at segment boundaries:

```cpp
NanLinker linker;
linker.add_segment(segment_a);
linker.add_segment(segment_b);
auto linked = linker.link(/*strip_padding=*/true);
std::cout << linker.total_size();  // raw total before stripping
```

### 13.6 Cross-Platform Emitter

`CrossPlatformEmitter` translates a vector of `IRInstruction` objects into architecture-specific machine code for either `Arch::X86_64` or `Arch::ARM64`:

```cpp
using IR = CrossPlatformEmitter::IRInstruction;
std::vector<IR> ir = {
    {NanOpcode::LOAD, 42},
    {NanOpcode::ADD,  8},
    {NanOpcode::RET,  0}
};
auto x86 = CrossPlatformEmitter::emit(ir, CrossPlatformEmitter::Arch::X86_64);
auto arm  = CrossPlatformEmitter::emit(ir, CrossPlatformEmitter::Arch::ARM64);
```

x86-64 uses `MOV EAX, imm32` + `ADD EAX, imm32` + `RET` encodings. ARM64 uses `MOV X0, #imm` (MOVZ) + `RET` encodings in little-endian 32-bit instruction words.

---

## 14. JIT Compilation

### 14.1 How JIT Works in NanLang

The JIT pipeline consists of three steps:

1. **IR construction** — the source or bytecode is translated to a vector of `IRInstruction` objects.
2. **Code generation** — `CrossPlatformEmitter::emit()` translates IR to raw machine bytes for the host architecture.
3. **Execution** — `JITPulseGen::compile()` allocates a RWX memory page, copies the machine bytes, applies `mprotect` to restrict to RX, and returns a callable function pointer.

### 14.2 JITPulseGen API

```cpp
// Generate a stub: "xor eax, eax; ret" (returns 0)
auto code = JITPulseGen::make_zero_func();

// Compile and get a callable function pointer
auto fn = JITPulseGen::compile(code);
if (fn) {
    fn();  // execute JIT-compiled code
}
```

A simpler single-instruction stub:
```cpp
auto code = JITPulseGen::make_ret_stub();  // just "ret"
auto fn   = JITPulseGen::compile(code);
```

### 14.3 W^X Policy & mprotect

Modern operating systems enforce the W^X (write XOR execute) security policy: a memory page cannot be simultaneously writable and executable. `JITPulseGen::compile()` respects this by:

1. Allocating with `PROT_READ | PROT_WRITE | PROT_EXEC` initially (to allow writing).
2. After `memcpy`, calling `mprotect(mem, size, PROT_READ | PROT_EXEC)` to remove write permission.

This prevents an attacker from modifying JIT-compiled code after it has been validated.

### 14.4 Platform Support

| Platform      | JIT Status          | Notes                                 |
|---------------|---------------------|---------------------------------------|
| Linux x86-64  | ✅ Full support     | `mmap(MAP_ANONYMOUS)` + `mprotect`    |
| Linux ARM64   | ⚠️  Code gen only  | IR→ARM64 bytes work; mmap untested    |
| macOS         | ⚠️  Partial        | Requires `MAP_JIT` on Apple Silicon   |
| Windows       | ❌ Not supported   | `VirtualAlloc` not yet implemented    |

---

## 15. Performance Subsystem

### 15.1 Pipeline Reordering

`PipelineReorder::reorder()` implements a simple list scheduling algorithm. It scans instructions in program order and hoists any instruction whose source registers are ready (not written by an unexecuted preceding instruction):

```cpp
std::vector<PipelineReorder::Inst> prog = {
    {0x05, 1, 2, 3},  // ADD R3 = R1+R2
    {0x05, 3, 4, 5},  // ADD R5 = R3+R4  (depends on R3)
    {0x09, 0, 1, 6},  // XOR R6 = R0^R1  (independent — hoisted)
};
auto reordered = PipelineReorder::reorder(prog);
// Order becomes: ADD R3, XOR R6, ADD R5
```

By executing the independent XOR between the two dependent ADDs, the processor's out-of-order window can overlap their execution, reducing stall cycles.

### 15.2 Out-of-Order Execution (OoO)

`ReservationStation` simulates Tomasulo's algorithm with a fixed 8-entry station array:

```cpp
ReservationStation rs;
int s0 = rs.issue(0x05, 10, 20);  // ADD 10+20 -> station 0
int s1 = rs.issue(0x09,  7,  3);  // XOR  7^3  -> station 1
rs.execute_ready();                // executes both in the same cycle
std::cout << rs.result_bus;        // last computed result
```

Entries with `qj == -1 && qk == -1` (both operands ready) are eligible for immediate execution.

### 15.3 Branch Predictor

`BranchPredictor` implements a 2-bit saturating counter per PC hash (256 entries):

| Counter Value | State              | Prediction |
|---------------|--------------------|------------|
| 0             | Strongly Not-Taken | Not-Taken  |
| 1             | Weakly Not-Taken   | Not-Taken  |
| 2             | Weakly Taken       | Taken      |
| 3             | Strongly Taken     | Taken      |

```cpp
BranchPredictor bp;
bool pred  = bp.predict(pc & 0xFF);
// ... execute branch ...
bp.update(pc & 0xFF, /*actually_taken=*/true);
```

The `hint_byte()` utility generates x86 branch hint prefixes (`0x3E` = taken, `0x2E` = not-taken) that can be prepended to conditional jump instructions.

### 15.4 Wait-Free Queue

`WaitFreeQueue<T, N>` is a lock-free SPSC (single-producer, single-consumer) circular queue. `N` must be a power of two; the modulus operation is replaced by a bitwise AND for efficiency:

```cpp
WaitFreeQueue<uint64_t, 256> q;
q.push(42ULL);
uint64_t v;
q.pop(v);  // v == 42
```

`push()` and `pop()` use `std::atomic` with `memory_order_acquire` / `memory_order_release` semantics — no locks, no condition variables, no futex syscalls. On the critical path this is 2–4 ns per operation on modern x86.

### 15.5 Deadlock Detection

`DeadlockDetector` maintains a resource allocation graph and runs a DFS on every `try_acquire()` call to detect cycles:

```cpp
DeadlockDetector dd;
dd.try_acquire(0, 0);   // Thread 0 acquires Resource 0
dd.try_acquire(1, 1);   // Thread 1 acquires Resource 1
dd.try_acquire(0, 1);   // Thread 0 waits for Resource 1
dd.try_acquire(1, 0);   // Thread 1 waits for Resource 0 -> CYCLE DETECTED
```

When a cycle is detected, `try_acquire()` returns `false` and prints a diagnostic. The requesting thread is not added to the wait graph, preventing the actual deadlock from forming.

### 15.6 Frequency Scaling Hints

`FreqScaler::hint()` inserts architecture-specific instructions to guide the CPU's dynamic frequency scaling:

| Load Level    | Instruction(s)              | Effect                         |
|---------------|-----------------------------|--------------------------------|
| `LIGHT`       | 10× `__builtin_ia32_pause()`| Signal underutilization; allow power-down |
| `MEDIUM`      | `_mm_sfence()`              | Memory barrier; neutral        |
| `HEAVY`       | `_mm_lfence()`              | Prevent speculative load; maximize frequency |

### 15.7 Minimalist Context Switching

`MiniContext` is a stub for fiber-like context switching, saving only the callee-saved registers (`RSP`, `RBP`, `RBX`, `R12`–`R15`, `RIP`). The current implementation is a placeholder; production use requires inline assembly or a platform-specific `setjmp`/`longjmp` equivalent.

---

## 16. Security Features

### 16.1 ManagedString — Volatile Sanitization

Any `str`-typed variable in NanLang is backed by `ManagedString`. Its destructor performs a volatile zero-wipe:

```cpp
~ManagedString() {
    volatile char* p = &content[0];
    for (size_t i = 0; i < content.size(); ++i) p[i] = 0;
}
```

The `volatile` qualifier prevents the compiler from eliding the writes as "dead stores." This ensures that cryptographic keys, passwords, and session tokens do not linger in memory after the string goes out of scope — a critical defense against cold-boot attacks and memory-scanning malware.

### 16.2 Jitter Injection

The `ExecutionEngine::security_delay()` method introduces a randomized delay of 0–31 NOP iterations:

```cpp
void security_delay() {
    volatile int garbage = 0;
    int limit = rand() % 32;
    for (int i = 0; i < limit; ++i) {
        garbage += i;
        __builtin_ia32_pause();
    }
}
```

This randomizes the timing of security-sensitive operations (key derivation, signature verification), making it significantly harder for an attacker to use timing measurements to infer secret values.

The `jitter` keyword in NanLang source inserts a call to `security_delay()` at the specified point in the code.

### 16.3 Hardware Fingerprinting

`HWFingerprint` reads the CPU's identity via the `CPUID` instruction (leaf 1, EBX and EDX registers) and embeds an 8-byte fingerprint at the tail of the binary:

```cpp
uint64_t id = HWFingerprint::get_cpu_id();
HWFingerprint::embed(binary);          // appends 8 bytes
bool ok = HWFingerprint::validate(id); // true only on the same CPU
```

This enables CPU-bound software licensing: a binary fingerprinted on machine A will fail validation on machine B. Combined with the XOR obfuscation (§16.4), the bytecode becomes effectively non-portable.

### 16.4 Hex Obfuscation (XOR Key)

`hw_key_xor()` encrypts (and decrypts — XOR is its own inverse) a byte vector using a rolling 8-byte key derived from the CPU ID:

```cpp
auto encrypted = hw_key_xor(bytecode, cpu_id);
auto decrypted = hw_key_xor(encrypted, cpu_id);
assert(decrypted == bytecode);
```

Key bytes are cycled with `key[i % 8]`, so each byte of the bytecode is XORed with a different byte of the 64-bit CPU ID. This is not cryptographically strong encryption, but it effectively prevents static disassembly without knowledge of the target machine's CPU ID.

---

## 17. Binary Operations

### 17.1 Binary Patching

`BinaryPatcher` reads an entire binary into a `std::vector<uint8_t>` image, applies in-memory patches, and writes the result to a new file:

```cpp
BinaryPatcher patcher("./program");
patcher.patch(0x100, {0x90, 0x90});        // write 2 NOPs at offset 0x100
patcher.find_and_patch(needle, replacement); // search-and-replace
patcher.save("./program.patched");
```

The original file is never modified. The `.patched` suffix is appended automatically when using the CLI `--patch` flag.

### 17.2 RLE Compression

Run-length encoding achieves excellent compression on bytecode streams that contain long runs of identical bytes (e.g., NOP padding, zero-initialized data):

```
Original:  AA AA AA BB CC CC DD DD DD DD   (10 bytes)
RLE:       03 AA 01 BB 02 CC 04 DD         (8 bytes)
```

```cpp
auto compressed   = rle_compress(data);
auto decompressed = rle_decompress(compressed);
assert(decompressed == data);
```

Maximum run length per pair is 255 bytes (fits in a `uint8_t`).

### 17.3 Self-Healing Binaries (Hamming)

The `hamming_encode` / `hamming_decode_correct` pair implements Hamming(7,4) error correction for nibbles (4-bit values):

```cpp
uint8_t encoded   = hamming_encode(0x0B);   // encode nibble 0xB
uint8_t corrupted = encoded ^ 0x08;         // flip bit 3
uint8_t corrected = hamming_decode_correct(corrupted);
assert(corrected == 0x0B);                  // error corrected
```

The 7-bit code can detect 2-bit errors and correct any single-bit error. This enables NanLang binaries stored on unreliable media (EEPROM, RF channels) to self-repair single-bit corruption without a retransmission.

### 17.4 ELF Generation

`write_minimal_elf()` produces a valid 64-bit ELF executable containing a single `PT_LOAD` segment. The entry point is set to `0x400000 + 64 + entry_offset`, placing execution directly at the first instruction of the provided code buffer:

```cpp
std::vector<uint8_t> code = { /* x86-64 machine code */ };
write_minimal_elf("output.elf", code);
```

The generated ELF has no section headers, no dynamic linking, and no symbol table. It is the smallest possible self-contained executable that Linux's ELF loader will accept.

### 17.5 Resource Embedding

`embed_resources()` appends a catalog of named binary resources to the end of a binary image. The catalog format is:

```
[count: u32] [name_len: u16] [name: bytes] [data_len: u32] [data: bytes] ...
```

This is analogous to Go's `//go:embed` or Rust's `include_bytes!`, but implemented as a post-link binary transformation:

```cpp
embed_resources(binary, {
    {"config.json", config_bytes},
    {"shader.spv",  shader_bytes}
});
```

---

## 18. Kernel Scheduler

The `kernel/scheduler.cpp` module implements `TaskScheduler`, a spin-lock-based task synchronization primitive for multi-core signal processing:

```cpp
class TaskScheduler {
    std::atomic<bool> sync_flag{false};
    std::vector<std::thread> thread_pool;
public:
    void core_sync();      // acquire spin-lock, execute critical section, release
    void join_workers();   // join all pooled threads
};
```

**Why spin-locks instead of `std::mutex`?**

For sub-microsecond critical sections (such as aligning a signal packet across cores), the overhead of `pthread_mutex_lock` — which involves a syscall if the lock is contended — exceeds the duration of the critical section itself. A spin-lock using `std::atomic::exchange` with `memory_order_acquire` avoids the kernel entirely, at the cost of burning CPU cycles while waiting.

The `__builtin_ia32_pause()` instruction inside the spin loop hints to the CPU that the thread is in a spin-wait state, allowing the processor to reduce power consumption and avoid memory order violations on hyperthreaded cores.

---

## 19. VS Code Extension

The `install_vscode_ext.sh` script installs a full VS Code language extension for NanLang `.nl` files:

```bash
bash install_vscode_ext.sh
# Restart VS Code to activate
```

The extension provides:

- **Syntax highlighting** for all NanLang keywords, types, operators, signal literals, strings, numbers, and comments.
- **Auto-closing pairs** for `{}`, `[]`, `()`, and `""`.
- **Block comment** support (`/* ... */`) and line comment support (`//`).
- **Bracket matching** for all three bracket types.

The TextMate grammar (`nanlang.tmLanguage.json`) defines the following scopes:

| Token Class         | TextMate Scope                        |
|---------------------|---------------------------------------|
| Keywords            | `keyword.control.nl`                  |
| Types               | `storage.type.nl`                     |
| Strings             | `string.quoted.double.nl`             |
| Escape sequences    | `constant.character.escape.nl`        |
| Signal High (`^`)   | `constant.language.signal.high.nl`    |
| Signal Low (`_`)    | `constant.language.signal.low.nl`     |
| Uncertain (`?`)     | `constant.language.uncertain.nl`      |
| Trap patterns       | `constant.language.trap.nl`           |
| Signal operators    | `keyword.operator.signal.nl`          |
| Arithmetic operators| `keyword.operator.arithmetic.nl`      |
| Numbers             | `constant.numeric.nl`                 |
| Line comments       | `comment.line.double-slash.nl`        |
| Block comments      | `comment.block.nl`                    |

---

## 20. Register Map Reference

The NanLang VM uses a 16-register file (R0–R15), all 64-bit signed integers:

| Register | Conventional Use              |
|----------|-------------------------------|
| R0       | Accumulator / return value    |
| R1–R3    | Function arguments            |
| R4–R7    | General purpose temporaries   |
| R8–R11   | Callee-saved                  |
| R12      | Stack pointer (VM)            |
| R13      | Frame pointer (VM)            |
| R14      | Link register (return address)|
| R15      | Zero register (always 0)      |

Registers are allocated by `RegisterMap::allocate()` in ascending order. When all 16 registers are occupied, `allocate()` returns `-1`, signaling that a register spill to the linear memory pool is required.

Register release:
```cpp
rm.free_reg(r1);   // zero the register and mark it free
```

---

## 21. Demo Gallery

Each demo exercises a specific NanLang subsystem. Run any demo with:
```bash
./nanlang --demo <name> [--param value ...]
```

| Demo Name    | Parameters                        | What It Shows                                                        |
|--------------|-----------------------------------|----------------------------------------------------------------------|
| `pulse`      | `--count N` `--pattern 0xXX`      | Emits N signals via `AsyncPulseEmitter`, reports cycles/emit         |
| `sigbuf`     | `--count N`                       | Push N items into `SignalBuffer`, pop all, report stats              |
| `simd`       | `--iterations N`                  | Runs N iterations of AVX2 XOR+OR on 32-byte vectors                 |
| `dma`        | `--size N`                        | Allocates two N-byte regions, performs non-temporal transfer         |
| `zerocopy`   | `--offset N` `--len N`            | Creates a zero-copy sub-slice of a 32-byte buffer                   |
| `regmap`     | (none)                            | Allocates 3 registers, reads values, frees one                      |
| `binary`     | `--cpu-key 0xXX`                  | Encrypts/decrypts 8 bytes with hardware-key XOR                     |
| `compress`   | (none)                            | RLE compresses a 10-byte buffer, verifies round-trip                 |
| `selfheal`   | `--nibble 0xX`                    | Hamming-encodes nibble, corrupts 1 bit, corrects, verifies          |
| `jit`        | (none)                            | JIT-compiles "xor eax,eax; ret" and executes it                     |
| `linker`     | `--segments N` `[--strip-nops]`   | Links N segments, optionally strips NOPs, reports sizes              |
| `color`      | (none)                            | Register-colors 4 variables with 3 conflict edges                   |
| `fold`       | (none)                            | Folds ADD+XOR constants in a 9-byte bytecode sequence               |
| `xplatform`  | (none)                            | Emits same IR to x86-64 and ARM64, reports byte counts              |
| `hwfp`       | (none)                            | Reads CPU-ID, embeds fingerprint in buffer, validates               |
| `waitfree`   | `--count N`                       | Pushes N items into wait-free queue, pops all                       |
| `deadlock`   | (none)                            | Demonstrates cycle detection in a 2-thread, 2-resource scenario     |
| `branch`     | `--rounds N`                      | Runs N branch predictions, reports accuracy percentage              |
| `pipeline`   | (none)                            | Reorders a 4-instruction program with data dependency               |
| `ooo`        | (none)                            | Issues 2 instructions to reservation station, executes both         |
| `all`        | (all of the above)                | Runs every demo in sequence                                          |

---

## 22. Frequently Asked Questions (FAQ)

**Q: Is NanLang a general-purpose programming language?**
No. NanLang is a domain-specific language for signal-driven, low-latency systems — think embedded firmware, real-time signal processors, and hardware interfacing layers. For general application development, languages like Rust or C++ are more appropriate.

**Q: Why is the `.nb` format so minimal? No sections, no symbols?**
The bytecode format is intentionally stripped. Symbol tables and section headers exist to support debugging and dynamic linking — neither of which is relevant for a deterministic single-pass VM that runs pre-verified code in a controlled environment. The savings (dozens of bytes of overhead) matter when you're targeting microcontrollers with 64 KB of flash.

**Q: Can I use NanLang on macOS or Windows?**
The core compiler and VM work on any POSIX platform. Some features — `MAP_LOCKED`, `MAP_HUGETLB`, JIT mmap, and non-temporal stores — are Linux-specific and will silently fall back to portable alternatives on other platforms. JIT compilation is not supported on Windows.

**Q: How do I add a new opcode?**
1. Add the opcode to the `NanOpcode` enum in `include/nan_binary.h`.
2. Add a case to `ExecutionEngine` in `src/engine.cpp`.
3. Add a constant-folding rule to `ConstantFolder::fold()` in `include/nan_compiler.h` if applicable.
4. Add x86-64 and ARM64 encoding to `CrossPlatformEmitter::emit()` if the opcode should be JIT-compilable.

**Q: What is the maximum bytecode file size?**
The VM reads operand lengths as `uint32_t`, so individual string payloads can be up to 4 GB. In practice, the 8 MB linear memory pool is the binding constraint for runtime data.

**Q: Is the Hamming implementation suitable for production ECC?**
The current Hamming(7,4) implementation is a demonstration. For production use on storage media, a stronger code (BCH, Reed-Solomon, or LDPC) should be used. The API surface and philosophy are identical; only the encoding/decoding functions need to be replaced.

**Q: Why does `security_delay()` use `rand()` instead of a cryptographic RNG?**
The goal of the jitter is statistical — to spread the timing distribution of security operations over a window large enough to defeat timing correlation attacks. A CSPRNG adds no practical benefit here, and `rand()` has zero system-call overhead. For high-security deployments, replace `rand()` with `arc4random()` or read from `/dev/urandom`.

---

## 23. Troubleshooting

### Build Failures

**`error: unknown target CPU 'native'`**
Your compiler version does not support `-march=native`. Try:
```bash
make release CXXFLAGS="-std=c++17 -O2 -mavx2 -I./include -pthread"
```

**`error: '_mm256_loadu_si256' was not declared`**
AVX2 intrinsics are not available. Ensure you have `-mavx2` in your CXXFLAGS and that your CPU supports AVX2 (`grep avx2 /proc/cpuinfo`).

**Linker error: `undefined reference to 'pthread_create'`**
Make sure `-pthread` is in both CXXFLAGS and LDFLAGS. The default Makefile already includes this.

### Runtime Issues

**`[RT] Invalid magic.`**
The `.nb` file is either corrupt or was not produced by NanLang. Verify with:
```bash
xxd output.nb | head -1
# Should show: 4e 41 4e 21  (!NAN in little-endian)
```

**`[JIT] mmap failed`**
Your system may have `vm.mmap_min_addr` set higher than 0, or `/proc/sys/kernel/perf_event_paranoid` may be restricting memory access. Try:
```bash
sudo sysctl -w vm.mmap_min_addr=0
```

**`[DMA] mmap failed, falling back to malloc`**
The system has no locked memory quota for your user. Increase `RLIMIT_MEMLOCK`:
```bash
ulimit -l unlimited
```

**Build succeeds but `./nanlang` crashes immediately**
Run the debug build and check for sanitizer output:
```bash
make debug
./nanlang --demo pulse
```

### Performance Issues

**SIMD operations seem slow**
Verify AVX2 is actually being used:
```bash
objdump -d ./nanlang | grep ymm
```
If no `ymm` registers appear, AVX2 is not enabled. Check your CXXFLAGS for `-mavx2`.

**High latency on `demo-pulse`**
The first run always has cold cache effects. Run twice and compare:
```bash
./nanlang --demo pulse --count 1024
./nanlang --demo pulse --count 1024
```

---

## 24. Contributing

Contributions are welcome. Please follow these guidelines:

### Code Style

- **C++17** required. No C++20 features.
- Follow existing naming conventions: `snake_case` for functions and variables, `PascalCase` for structs and classes.
- All public-facing functions must have a doc comment explaining purpose, parameters, and return value.
- SIMD code must always have a scalar fallback guarded by `#ifdef __AVX2__`.

### Adding a New Demo

1. Add the demo function declaration to the `namespace nanlang` block in `main.cpp`.
2. Implement the demo in the appropriate `src/` file.
3. Add a case to `run_demo()` in `main.cpp`.
4. Add a Makefile target `demo-<name>`.
5. Document the demo in this README under [§21](#21-demo-gallery).

### Submitting Patches

1. Fork the repository and create a feature branch.
2. Ensure `make debug` builds cleanly with no sanitizer errors.
3. Run `make demo-all` and confirm all demos produce expected output.
4. Submit a pull request with a clear description of the change and its motivation.

---

## 25. Glossary

| Term                   | Definition                                                                                          |
|------------------------|-----------------------------------------------------------------------------------------------------|
| **AVX2**               | Advanced Vector Extensions 2. Intel/AMD SIMD extension operating on 256-bit (32-byte) registers.  |
| **Bytecode**           | Platform-neutral binary encoding of a program, interpreted by the NanLang VM.                     |
| **Cache Line**         | The smallest unit of data transferred between CPU cache levels; typically 64 bytes on x86.         |
| **Constant Folding**   | Compile-time evaluation of expressions whose operands are known constants.                         |
| **DMA**                | Direct Memory Access. Data transfer that bypasses the CPU's cache hierarchy.                       |
| **ELF**                | Executable and Linkable Format. The standard binary format for Linux executables.                  |
| **Hamming Code**       | Error-correcting code that can detect 2-bit errors and correct 1-bit errors in a data word.        |
| **Huge Page**          | A memory page larger than the default 4 KB (typically 2 MB on x86). Reduces TLB pressure.         |
| **JIT**                | Just-In-Time compilation. Generating and executing native machine code at runtime.                 |
| **Nan-ISA**            | The NanLang Instruction Set Architecture: the 16-opcode bytecode encoding used by the NanLang VM. |
| **NOP**                | No Operation. An instruction that does nothing; used for alignment padding.                        |
| **OoO**                | Out-of-Order execution. Executing instructions in a different order than programmed, for performance.|
| **Potential**          | In NanLang, an 8-bit unsigned integer modeling a hardware voltage or energy level (0–255).         |
| **RDTSC / RDTSCP**     | x86 instructions that read the CPU's Time Stamp Counter (cycle count since reset).                 |
| **RLE**                | Run-Length Encoding. A lossless compression scheme encoding runs of identical values as (count, value) pairs.|
| **Signal**             | In NanLang, a binary state: High (^) or Low (_), modeling a logic level on a hardware line.        |
| **Spin-lock**          | A synchronization primitive where a thread repeatedly polls an atomic flag instead of sleeping.    |
| **SPSC Queue**         | Single-Producer Single-Consumer queue. Lock-free when only one thread writes and one thread reads. |
| **TLB**                | Translation Lookaside Buffer. CPU cache for virtual-to-physical address translations.              |
| **Tomasulo Algorithm** | A hardware algorithm for dynamic scheduling of instructions to avoid data hazards.                 |
| **Volatile**           | A C/C++ qualifier preventing the compiler from optimizing away memory accesses.                    |
| **W^X**                | Write XOR Execute. Security policy preventing memory pages from being simultaneously writable and executable.|
| **Zero-Copy**          | Data processing technique where data is referenced by pointer rather than copied.                  |

---

## 26. License

NanLang and its toolchain are released under the terms described in the `LICENSE` file included in the distribution. See that file for full details.

---

*NanLang v3.0.2 — Signal-Driven Systems Language*
*Last updated: 2026*

---

> **Wiki Navigation:** [Overview](#1-overview) · [Language Ref](#5-language-reference) · [CLI](#7-cli-reference) · [Memory Model](#10-memory-model) · [JIT](#14-jit-compilation) · [Security](#16-security-features) · [FAQ](#22-frequently-asked-questions-faq) · [Glossary](#25-glossary)