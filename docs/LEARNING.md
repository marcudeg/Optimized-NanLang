# LEARNING.md — The Complete NanLang Learning Guide

> **NanLang v3.0.2** | From Zero to Systems Programmer
> A comprehensive, hands-on curriculum for learning the NanLang signal-driven language from absolute basics to advanced internals.

---

## How to Use This Guide

This document is structured as a progressive curriculum. Each chapter builds on the previous one. If you are completely new to systems programming, start at Chapter 1 and work through every exercise before moving on. If you already know C or C++, you can skim Chapters 1–3 and jump to Chapter 4 where NanLang-specific concepts begin.

Every chapter contains:
- **Concept explanations** — the theory behind the feature
- **Code examples** — annotated NanLang source you can compile and run
- **Exercises** — hands-on tasks to cement the knowledge
- **Common mistakes** — pitfalls that beginners regularly hit
- **Deep dives** — optional sections for learners who want to understand the internals

**Prerequisites:**
- A Linux machine with GCC 9+ and AVX2-capable CPU
- NanLang toolchain built (`make release`)
- A text editor (VS Code with the NanLang extension is recommended)

---

## Table of Contents

**Part I — Foundations**
- [Chapter 1: Your First NanLang Program](#chapter-1-your-first-nanlang-program)
- [Chapter 2: The Compilation Pipeline](#chapter-2-the-compilation-pipeline)
- [Chapter 3: Types and Values](#chapter-3-types-and-values)
- [Chapter 4: Variables and Bindings](#chapter-4-variables-and-bindings)
- [Chapter 5: Operators and Expressions](#chapter-5-operators-and-expressions)

**Part II — Control Flow**
- [Chapter 6: Conditional Logic](#chapter-6-conditional-logic)
- [Chapter 7: Loops and Iteration](#chapter-7-loops-and-iteration)
- [Chapter 8: Functions](#chapter-8-functions)
- [Chapter 9: Error Handling and Interrupts](#chapter-9-error-handling-and-interrupts)

**Part III — Signals and Hardware**
- [Chapter 10: The Signal System](#chapter-10-the-signal-system)
- [Chapter 11: Potentials and Energy Levels](#chapter-11-potentials-and-energy-levels)
- [Chapter 12: Pulse Emission](#chapter-12-pulse-emission)
- [Chapter 13: Hardware Register Access](#chapter-13-hardware-register-access)

**Part IV — Memory and Performance**
- [Chapter 14: The Memory Model](#chapter-14-the-memory-model)
- [Chapter 15: Zero-Copy Programming](#chapter-15-zero-copy-programming)
- [Chapter 16: SIMD and Vectorization](#chapter-16-simd-and-vectorization)
- [Chapter 17: Timing and Cycle Counting](#chapter-17-timing-and-cycle-counting)

**Part V — Compiler Features**
- [Chapter 18: Constant Folding](#chapter-18-constant-folding)
- [Chapter 19: Register Allocation](#chapter-19-register-allocation)
- [Chapter 20: The Linker](#chapter-20-the-linker)
- [Chapter 21: JIT Compilation](#chapter-21-jit-compilation)
- [Chapter 22: Cross-Platform Code Generation](#chapter-22-cross-platform-code-generation)

**Part VI — Security Programming**
- [Chapter 23: Secure Strings and Memory Wiping](#chapter-23-secure-strings-and-memory-wiping)
- [Chapter 24: Timing Attack Mitigation](#chapter-24-timing-attack-mitigation)
- [Chapter 25: Hardware Fingerprinting](#chapter-25-hardware-fingerprinting)
- [Chapter 26: Binary Obfuscation](#chapter-26-binary-obfuscation)

**Part VII — Binary Engineering**
- [Chapter 27: Binary Patching](#chapter-27-binary-patching)
- [Chapter 28: Compression and Self-Healing](#chapter-28-compression-and-self-healing)
- [Chapter 29: ELF Internals](#chapter-29-elf-internals)

**Part VIII — Concurrency**
- [Chapter 30: Wait-Free Data Structures](#chapter-30-wait-free-data-structures)
- [Chapter 31: Deadlock Detection](#chapter-31-deadlock-detection)
- [Chapter 32: The Kernel Scheduler](#chapter-32-the-kernel-scheduler)

**Part IX — Advanced Topics**
- [Chapter 33: The Branch Predictor](#chapter-33-the-branch-predictor)
- [Chapter 34: Pipeline Reordering](#chapter-34-pipeline-reordering)
- [Chapter 35: Out-of-Order Execution](#chapter-35-out-of-order-execution)
- [Chapter 36: Building a Complete Program](#chapter-36-building-a-complete-program)
- [Chapter 37: Debugging NanLang Programs](#chapter-37-debugging-nanlang-programs)
- [Chapter 38: Performance Profiling](#chapter-38-performance-profiling)

**Appendices**
- [Appendix A: Quick Reference Card](#appendix-a-quick-reference-card)
- [Appendix B: Common Error Messages](#appendix-b-common-error-messages)
- [Appendix C: Exercises Answer Key](#appendix-c-exercises-answer-key)
- [Appendix D: Further Reading](#appendix-d-further-reading)

---

# Part I — Foundations

---

## Chapter 1: Your First NanLang Program

### 1.1 What is NanLang?

NanLang is a systems programming language. That means it is designed for writing software that lives very close to the hardware — operating systems, device drivers, embedded firmware, signal processors, and real-time control systems. Unlike Python or JavaScript, NanLang does not have a garbage collector, a runtime VM abstracting away the machine, or high-level abstractions for tasks like network requests or database queries.

What NanLang gives you instead:
- **Direct control over CPU cycles** via hardware intrinsics and RDTSC cycle counters
- **Deterministic memory usage** via a pre-allocated linear pool — no surprise allocations
- **Signal primitives** that map naturally to hardware concepts (logic levels, voltage potentials, interrupt lines)
- **A compact bytecode format** that runs on the NanLang VM, or compiles to native ELF binaries

Think of NanLang as sitting between C (maximum control, no safety) and Rust (maximum safety, some runtime cost). NanLang prioritizes latency predictability above all else.

### 1.2 Setting Up

Before writing any code, verify your toolchain is ready:

```bash
# Check your GCC version (need 9+)
gcc --version

# Check AVX2 support
grep -o avx2 /proc/cpuinfo | head -1

# Build NanLang
cd NanLang/
make release

# Verify the binary works
./nanlang --version
# Expected output: NanLang v3.0.2
```

Install the VS Code extension for syntax highlighting:
```bash
bash install_vscode_ext.sh
# Restart VS Code after installation
```

### 1.3 Hello, World!

Create a file called `hello.nl`:

```nanlang
fn main() -> void {
    say "Hello, World!";
}
```

Compile it to bytecode:
```bash
./nanlang --compile hello.nl --output hello.nb
```

Run it:
```bash
./nanlang --run hello.nb
```

You should see:
```
Hello, World!
```

Congratulations — you have just compiled and executed your first NanLang program.

### 1.4 What Just Happened?

Let's break down every part of that program:

```nanlang
fn main() -> void {
```
- `fn` — keyword that declares a function
- `main` — the name of the function (entry point by convention)
- `()` — the parameter list (empty in this case)
- `->` — the return type annotation arrow
- `void` — this function returns nothing
- `{` — opens the function body block

```nanlang
    say "Hello, World!";
```
- `say` — a built-in I/O keyword that prints a string followed by a newline
- `"Hello, World!"` — a string literal
- `;` — every statement ends with a semicolon

```nanlang
}
```
- Closes the function body block

### 1.5 Multiple Output Lines

You can have multiple `say` statements:

```nanlang
fn main() -> void {
    say "Line one";
    say "Line two";
    say "Line three";
}
```

Save as `multi.nl`, compile and run:
```bash
./nanlang --compile multi.nl --output multi.nb
./nanlang --run multi.nb
```

Output:
```
Line one
Line two
Line three
```

### 1.6 Comments

NanLang supports two comment styles:

```nanlang
// This is a single-line comment. Everything after // is ignored.

/*
   This is a multi-line comment.
   It can span many lines.
   Use it for longer explanations.
*/

fn main() -> void {
    say "Comments don't affect output"; // inline comment
}
```

Comments are purely for the human reader. The compiler strips them completely during the lexing phase — they produce zero bytes in the output bytecode.

### 1.7 Exercises — Chapter 1

**Exercise 1.1:** Write a NanLang program that prints your name, your city, and the current year, each on a separate line.

**Exercise 1.2:** Write a program with at least one single-line comment and one multi-line comment that explain what the program does. Verify that removing the comments does not change the output.

**Exercise 1.3:** What happens if you forget the semicolon at the end of a `say` statement? Try it and observe the error.

**Exercise 1.4:** Can you put the `say` keyword on the same line as `fn main() -> void {`? Why or why not?

### 1.8 Common Mistakes

**Mistake: Forgetting the semicolon**
```nanlang
// Wrong:
say "Hello"

// Right:
say "Hello";
```

**Mistake: Using single quotes instead of double quotes**
```nanlang
// Wrong:
say 'Hello';

// Right:
say "Hello";
```

**Mistake: Lowercase `void` written as `Void`**
```nanlang
// Wrong:
fn main() -> Void {

// Right:
fn main() -> void {
```

---

## Chapter 2: The Compilation Pipeline

### 2.1 From Source to Execution

NanLang programs travel through several stages before they produce output. Understanding this pipeline is not optional — it directly shapes how you write and debug programs.

```
hello.nl  (source text)
    │
    │  ./nanlang --compile
    ▼
hello.nb  (NanLang bytecode)
    │
    │  ./nanlang --run
    ▼
  Output  (stdout)
```

Alternatively, you can skip the bytecode step entirely and build a native binary:

```
hello.nl  (source text)
    │
    │  ./nanlang --build-native
    ▼
hello     (native ELF executable)
    │
    │  ./hello
    ▼
  Output  (stdout)
```

### 2.2 The Compiler — Stage 1: Lexing

The compiler reads your `.nl` file character by character, grouping characters into **tokens**. A token is the smallest meaningful unit in the language: a keyword, an identifier, a string literal, a semicolon, a brace, etc.

Given this source:
```nanlang
fn main() -> void {
    say "Hello";
}
```

The lexer produces this token stream:
```
KEYWORD(fn)  IDENT(main)  LPAREN  RPAREN  ARROW  KEYWORD(void)
LBRACE
KEYWORD(say)  STRING("Hello")  SEMICOLON
RBRACE
```

This token stream is then passed to the code generator.

### 2.3 The Compiler — Stage 2: Code Generation

The current NanLang compiler performs **single-pass** code generation. It scans the token stream and emits bytecode instructions directly, without building an intermediate AST (Abstract Syntax Tree).

For a `say "..."` statement, it emits:
- `0x02` — the `OP_SAY` opcode (1 byte)
- The string length as a 4-byte little-endian `uint32_t`
- The raw UTF-8 bytes of the string

For `fn main() -> void { ... }`, the compiler records the entry point but emits no prologue bytecode in the current version.

### 2.4 The Bytecode File Format

Open a compiled `.nb` file with a hex viewer:
```bash
./nanlang --compile hello.nl --output hello.nb
xxd hello.nb
```

You will see something like:
```
00000000: 4e41 4e21 0205 0000 0048 656c 6c6f  NAN!.....Hello
```

Let's decode this:
```
Bytes: 4E 41 4E 21
       │  │  │  └── 0x21 = '!'
       │  │  └───── 0x4E = 'N'
       │  └──────── 0x41 = 'A'
       └─────────── 0x4E = 'N'
       Together: "NAN!" — the magic number (stored little-endian as 0x214E414E)

Bytes: 02
       └── OP_SAY opcode

Bytes: 05 00 00 00
       └── uint32_t little-endian = 5 (length of "Hello")

Bytes: 48 65 6C 6C 6F
       └── UTF-8 for "Hello"
```

The magic number `0x214E414E` is validated by the VM before execution. If the file does not start with these 4 bytes, you get `[RT] Invalid magic.`

### 2.5 The VM — How `--run` Works

When you execute `./nanlang --run hello.nb`, the `NanRuntime::run()` method:

1. Opens the file in binary mode
2. Reads and validates the 4-byte magic header
3. Enters a dispatch loop: reads one byte (the opcode), executes the corresponding handler, repeats until EOF

For `OP_SAY`:
1. Reads 4 bytes as the string length `len`
2. Reads `len` bytes into a buffer
3. Prints the buffer contents followed by `\n`

This is essentially a minimal **bytecode interpreter**. It is intentionally simple — the goal is not to build a feature-rich VM, but to provide a thin execution layer for testing compiled NanLang programs before deploying them as native binaries.

### 2.6 Native Compilation

The `--build-native` path is fundamentally different. Instead of generating NanLang bytecode, the compiler:

1. Generates a temporary C++ source file (`nan_temp__.cpp`) that is semantically equivalent to the NanLang source
2. Invokes `g++` to compile that C++ file to a native ELF binary
3. Deletes the temporary file

```bash
./nanlang --build-native hello.nl --output hello --opt
```

The `--opt` flag adds `-O3 -march=native -flto` to the g++ invocation, enabling aggressive optimization.

Try it:
```bash
./nanlang --build-native hello.nl --output hello --opt
file hello
# hello: ELF 64-bit LSB executable, x86-64
./hello
# Hello, World!
```

### 2.7 Build Modes

The Makefile supports three build modes for the toolchain itself:

```bash
make release   # -O3 -march=native -flto  (fastest runtime)
make debug     # -O0 -g3 -fsanitize=address,undefined  (safest for development)
make fast      # -O3 -fprofile-generate  (PGO instrumentation)
```

During learning, always use `make debug` so that memory errors and undefined behavior are caught immediately with detailed diagnostics.

### 2.8 Exercises — Chapter 2

**Exercise 2.1:** Compile `hello.nl` and use `xxd` to inspect the `.nb` file. Manually decode every byte and explain what each one means.

**Exercise 2.2:** Write a program with three `say` statements. How many bytes does the `.nb` file contain? Calculate the expected size by hand before looking.

**Exercise 2.3:** Build a native binary from your program with `--opt`. Run both the VM version and the native version and compare the output. Are they identical?

**Exercise 2.4:** Intentionally corrupt the first byte of a `.nb` file (e.g., change `4E` to `00` with a hex editor or with `printf '\x00' | dd of=hello.nb bs=1 count=1 conv=notrunc`). What error do you get when you try to run it?

---

## Chapter 3: Types and Values

### 3.1 NanLang's Type System

NanLang is **statically typed** — every value has a type that is known at compile time. Unlike C++, there is no implicit type coercion; you cannot accidentally use a `potential` where a `signal` is expected.

NanLang has five built-in types:

| Type        | Size     | Range              | Purpose                               |
|-------------|----------|--------------------|---------------------------------------|
| `potential` | 1 byte   | 0–255              | 8-bit energy/voltage level            |
| `signal`    | 1 byte   | High or Low        | Binary logic state                    |
| `flow`      | variable | N signals          | Ordered sequence of signal states     |
| `str`       | variable | UTF-8 text         | Security-managed string               |
| `shadow`    | variable | raw bytes          | Opaque byte buffer                    |

And one pseudo-type:

| Type   | Purpose                          |
|--------|----------------------------------|
| `void` | Used only as a function return type to indicate "returns nothing" |

### 3.2 The `potential` Type

A `potential` is an **unsigned 8-bit integer** — it can hold any whole number from 0 to 255. The name comes from the concept of electrical potential (voltage). In NanLang, a `potential` represents a quantized energy level.

```nanlang
fn show_potentials() -> void {
    let zero: potential = 0;
    let max: potential  = 255;
    let mid: potential  = 128;
    say "Potentials declared";
}
```

Why `uint8_t` and not `int` or `uint64_t`? Because NanLang is designed for embedded and hardware contexts where 8-bit registers are extremely common. The 0–255 range maps directly to PWM duty cycles, ADC samples, register values, and interrupt priority levels.

### 3.3 The `signal` Type

A `signal` represents a **binary logic state**: either `High` (logic 1) or `Low` (logic 0). In the C++ implementation this is an `enum class Signal : uint8_t`.

```nanlang
fn signal_example() -> void {
    let line_a: signal = ^;   // High
    let line_b: signal = _;   // Low
    say "Signals declared";
}
```

Signal literals:
- `^` — High (one caret, representing a raised voltage)
- `_` — Low (underscore, representing ground)
- `?` — Uncertain/floating (neither definitively high nor low)

The `enum class` enforcement means you cannot accidentally write `signal + 1` or compare a signal with a potential directly. This prevents a whole class of hardware programming bugs where logic levels get mixed with numeric values.

### 3.4 The `flow` Type

A `flow` is an ordered sequence of `signal` values — a digital waveform or bus transaction. It maps to `std::vector<Signal>` in the runtime.

```nanlang
fn flow_example() -> void {
    let bus_data: flow = [^, _, ^, ^, _, _, ^, _];
    say "8-bit flow declared";
}
```

Flows are the primary way to model multi-bit data in NanLang. An 8-bit flow represents one byte transferred over a parallel bus. A 32-bit flow represents a word.

### 3.5 The `str` Type

`str` is NanLang's **security-managed string** type. Its C++ backing is `ManagedString`, which performs a `volatile` zero-wipe of all its bytes when it goes out of scope.

```nanlang
fn secure_string() -> void {
    let password: str = "correct-horse-battery-staple";
    // When password goes out of scope at the closing brace,
    // every byte of "correct-horse-battery-staple" is overwritten with 0x00
    // using volatile writes that the compiler cannot optimize away.
    say "Password handled securely";
}
```

**When to use `str` vs a raw string literal:**
- Use `str` for any data that is sensitive: passwords, cryptographic keys, session tokens, PINs
- String literals in `say "..."` statements are handled differently — they are embedded directly in the bytecode and do not get wiped

### 3.6 The `shadow` Type

A `shadow` is a raw, opaque byte buffer — essentially `std::vector<uint8_t>`. It has no semantic interpretation; it is just a sequence of bytes.

```nanlang
fn shadow_example() -> void {
    let buffer: shadow = [0xDE, 0xAD, 0xBE, 0xEF];
    say "4-byte shadow buffer declared";
}
```

`shadow` buffers are used for:
- Raw memory regions that will be passed to hardware
- Bytecode payloads before they are parsed
- Encrypted or compressed data blobs

### 3.7 Type Safety

NanLang's type system prevents common hardware programming errors:

```nanlang
// This is a type error — you cannot assign a signal to a potential:
let level: potential = ^;   // ERROR: type mismatch

// This is correct:
let level: potential = 1;
let line: signal     = ^;
```

The compiler catches these errors at compile time, before the program ever runs. In a real embedded system, mixing voltage levels with numeric counters can cause hardware damage — NanLang's type system makes this class of bug impossible.

### 3.8 Exercises — Chapter 3

**Exercise 3.1:** Declare one variable of each type (`potential`, `signal`, `str`, `shadow`) in a function. Add a comment above each declaration explaining what the variable could represent in a real hardware system.

**Exercise 3.2:** What is the largest value a `potential` can hold? What is the smallest? What happens in hardware when a counter overflows an 8-bit register?

**Exercise 3.3:** Look at the `nan_core.h` header file. Find the C++ definition of `Signal`. Why is `enum class` used instead of a plain `enum`? What protection does `enum class` provide?

**Exercise 3.4:** Why does the `ManagedString` destructor use `volatile char*` instead of just `char*`? Research "dead store elimination" and explain the connection.

---

## Chapter 4: Variables and Bindings

### 4.1 Declaring Variables with `let`

In NanLang, variables are declared with the `let` keyword followed by the name, a colon, the type, and an initial value:

```nanlang
let name: type = value;
```

Examples:
```nanlang
fn variables() -> void {
    let counter: potential = 0;
    let threshold: potential = 200;
    let active: signal = ^;
    let device_name: str = "sensor-01";
    say "Variables initialized";
}
```

All variables **must** be initialized at declaration. NanLang does not allow uninitialized variables because reading uninitialized memory is undefined behavior — in a signal processing context, reading garbage from a register can corrupt an entire data stream.

### 4.2 Variable Naming Rules

Variable names (identifiers) must follow these rules:

- Start with a letter (`a-z`, `A-Z`) or underscore (`_`)
- Contain only letters, digits (`0-9`), and underscores
- Are case-sensitive: `Counter` and `counter` are different variables
- Cannot be a keyword

Good names:
```nanlang
let signal_strength: potential = 127;
let _internal_flag: signal = _;
let deviceId: potential = 42;
```

Bad names:
```nanlang
let 1st_reading: potential = 0;   // starts with digit
let let: potential = 0;           // 'let' is a keyword
let signal strength: potential;   // space in name
```

### 4.3 Variable Scope

Variables in NanLang are **block-scoped**. A variable only exists within the `{...}` block where it was declared:

```nanlang
fn scope_demo() -> void {
    let outer: potential = 10;

    {
        let inner: potential = 20;
        // Both 'outer' and 'inner' are accessible here
        say "Inside inner block";
    }

    // 'inner' is no longer accessible here
    // Only 'outer' is accessible
    say "Back in outer scope";
}
```

When a block closes (`}`), all variables declared in that block are destroyed. For `str` variables, this triggers the volatile zero-wipe. For `potential`, `signal`, and `shadow` variables, the storage is simply reclaimed.

### 4.4 The Hardware Register Binding Operator

The `@` operator binds a variable to a hardware register address:

```nanlang
fn register_access() -> void {
    let gpio_port: potential = @0x40020000;
    // gpio_port now reads from memory-mapped register at 0x40020000
    say "Hardware register bound";
}
```

This is the NanLang equivalent of C's:
```c
volatile uint8_t* gpio_port = (volatile uint8_t*)0x40020000;
```

The `@` prefix on the value side tells the compiler to perform a hardware register read rather than a normal memory read. On the assignment side, `@addr = value` writes to the register.

### 4.5 Mutable vs Immutable Bindings

By default, `let` creates an **immutable binding** — you cannot reassign it after declaration. This is intentional: in a real-time signal processor, unexpected mutation of a timing variable can cause data corruption.

```nanlang
fn immutable_demo() -> void {
    let freq: potential = 440;
    // freq = 880;  <- this would be an error in strict mode
    say "Immutable binding declared";
}
```

To create a mutable binding (one you can reassign), use `let mut`:

```nanlang
fn mutable_demo() -> void {
    let mut count: potential = 0;
    count = 1;
    count = 2;
    say "Mutable counter reassigned";
}
```

The distinction between mutable and immutable bindings forces you to be explicit about which values change during execution — making the program's data flow much easier to reason about.

### 4.6 Constants

Constants are values known at compile time and never change. They are declared without `let`, directly in function or file scope:

```nanlang
const SAMPLE_RATE: potential = 44;
const MAX_GAIN:    potential = 255;
const CLOCK_LINE:  signal    = ^;

fn use_constants() -> void {
    say "Constants are always immutable";
}
```

Constants are evaluated at compile time and inlined wherever they are used — they produce zero runtime overhead.

### 4.7 Exercises — Chapter 4

**Exercise 4.1:** Write a function that declares variables representing the configuration of a hypothetical microcontroller: clock speed (as a potential), a reset line (as a signal), a device name (as a str), and a firmware image buffer (as a shadow).

**Exercise 4.2:** Experiment with scope. Declare a variable in an inner block, then try to use it after the block closes. What error do you get?

**Exercise 4.3:** Declare a `let` binding and then try to reassign it without `mut`. What happens? Now add `mut` and confirm the reassignment works.

**Exercise 4.4:** Declare two constants at the top of a file and use them inside a function. What is the difference between a constant and an immutable `let` binding?

---

## Chapter 5: Operators and Expressions

### 5.1 Arithmetic on Potentials

Since `potential` is an 8-bit unsigned integer, arithmetic follows 8-bit unsigned rules:

```nanlang
fn arithmetic() -> void {
    let a: potential = 10;
    let b: potential = 20;
    let sum:  potential = a + b;   // 30
    let diff: potential = b - a;   // 10
    let prod: potential = a * b;   // 200
    let quot: potential = b / a;   // 2
    say "Arithmetic done";
}
```

**8-bit overflow behavior:** When the result exceeds 255, it wraps around modulo 256. This is defined behavior in NanLang (unlike C signed integer overflow).

```nanlang
fn overflow_demo() -> void {
    let a: potential = 250;
    let b: potential = 10;
    let result: potential = a + b;  // 260 mod 256 = 4
    say "250 + 10 = 4 (overflow wraps)";
}
```

This wrapping behavior is actually useful in many hardware contexts — timer registers wrap naturally, CRC accumulators overflow intentionally, and PWM duty cycle calculations rely on 8-bit modular arithmetic.

### 5.2 Comparison Operators

```nanlang
fn comparisons() -> void {
    let x: potential = 100;
    let y: potential = 200;

    // These evaluate to a signal (^ = true, _ = false)
    let equal:   signal = x == y;   // _ (false)
    let unequal: signal = x != y;   // ^  (true)
    let less:    signal = x < y;    // ^  (true)
    let greater: signal = x > y;    // _ (false)

    say "Comparisons produce signals";
}
```

Notice that comparison operators return a `signal`, not a `potential`. This is intentional — comparisons produce binary outcomes (true/false), which map to `High`/`Low` signal states. This allows comparison results to be fed directly into hardware logic without conversion.

### 5.3 Signal Operators

Signal operators work on `signal` values and produce `signal` results:

```nanlang
fn signal_ops() -> void {
    let a: signal = ^;   // High
    let b: signal = _;   // Low

    // AND: both must be High for result to be High
    let and_result: signal = a ^&^ b;   // _ (Low)

    // Uncertainty merge: if either is uncertain, result is uncertain
    let unc_a: signal = ?;
    let merged: signal = a _?_ unc_a;   // ? (uncertain)

    // Signal shift / propagation
    let shifted: signal = a >>> b;

    say "Signal operations complete";
}
```

**The `^&^` operator** is the signal AND. The unusual notation uses carets to visually represent "high states on both sides." A AND output is High only when both inputs are High.

**The `_?_` operator** is the uncertainty merge. It propagates the `?` (uncertain/floating) state — if either input is floating, the output is floating. This models real hardware behavior where floating inputs produce undefined output.

**The `>>>` operator** is the signal propagation / shift operator. It models a signal traveling from the source through a propagation path to the destination.

### 5.4 The Hardware Bind Operator

The `@` operator, introduced in Chapter 4, is also used in expressions:

```nanlang
fn io_port() -> void {
    // Read from a memory-mapped I/O address
    let status: potential = @0xFF000000;

    // Write to a memory-mapped I/O address
    @0xFF000004 = 0xAB;

    say "I/O port access complete";
}
```

This is the primary way NanLang programs interact with hardware peripherals. Memory-mapped I/O is used by virtually all modern microcontrollers and SoCs.

### 5.5 Operator Precedence

NanLang uses standard mathematical precedence for arithmetic operators:

```
Highest:  *  /           (multiplication, division)
          +  -           (addition, subtraction)
          <  >  ==  !=   (comparisons)
          ^&^  _?_  >>>  (signal operators)
Lowest:   =  @           (assignment)
```

Use parentheses to override precedence:
```nanlang
fn precedence() -> void {
    let a: potential = 2 + 3 * 4;     // = 14 (not 20)
    let b: potential = (2 + 3) * 4;   // = 20
    say "Precedence respected";
}
```

### 5.6 Exercises — Chapter 5

**Exercise 5.1:** Write a function that computes the average of three `potential` values. What happens when the sum exceeds 255? How would you handle this in real embedded code?

**Exercise 5.2:** Write a signal logic function that models a NAND gate: output should be Low only when both inputs are High.

**Exercise 5.3:** What is `250 - 5` in 8-bit arithmetic? What is `5 - 10` in 8-bit arithmetic? What result do you get and why?

**Exercise 5.4:** Write an expression that reads a hardware register, masks the lower 4 bits, and stores the result. Use the AND operator with a mask value of `0x0F`.

---

# Part II — Control Flow

---

## Chapter 6: Conditional Logic

### 6.1 The `if` Statement

The `if` statement executes a block of code only if a condition is `High` (true):

```nanlang
fn check_voltage(level: potential) -> void {
    if level > 200 {
        say "Voltage is HIGH — warning!";
    }
    say "Check complete";
}
```

Notice there are no parentheses around the condition. The condition itself is any expression that evaluates to a `signal`.

### 6.2 `if` / `else`

Add an `else` block to handle the false case:

```nanlang
fn classify_signal(strength: potential) -> void {
    if strength > 127 {
        say "Strong signal";
    } else {
        say "Weak signal";
    }
}
```

### 6.3 `if` / `else if` / `else`

Chain multiple conditions with `else if`:

```nanlang
fn classify_level(level: potential) -> void {
    if level > 200 {
        say "Critical";
    } else if level > 150 {
        say "Warning";
    } else if level > 100 {
        say "Normal";
    } else {
        say "Low";
    }
}
```

The conditions are checked from top to bottom. The first condition that is `High` triggers its block; the rest are skipped.

### 6.4 Conditions from Signal Comparisons

Since comparisons return `signal` values, you can store a condition in a variable and reuse it:

```nanlang
fn reuse_condition(a: potential, b: potential) -> void {
    let is_equal: signal = a == b;

    if is_equal {
        say "Values match";
    }

    // Later...
    if is_equal {
        say "Still matching (same check, reused)";
    }
}
```

This is useful when the same condition is checked multiple times — you avoid recomputing the comparison.

### 6.5 Checking Signal States Directly

Since `signal` values are `High` or `Low`, you can use a signal directly as a condition:

```nanlang
fn check_line(ready: signal) -> void {
    if ready {
        say "Line is ready — proceeding";
    } else {
        say "Line not ready — waiting";
    }
}
```

This is idiomatic NanLang. Checking `if ready == ^` would be redundant; comparing a boolean to `true` is unnecessary in any language.

### 6.6 Nested Conditionals

Conditionals can be nested to arbitrary depth:

```nanlang
fn nested_check(voltage: potential, current: potential) -> void {
    if voltage > 100 {
        if current > 50 {
            say "Both high — safe operating range";
        } else {
            say "High voltage, low current — check load";
        }
    } else {
        say "Low voltage — device may be off";
    }
}
```

Keep nesting shallow (maximum 3–4 levels) for readability. Deeply nested conditionals are a sign that the logic should be split into smaller functions.

### 6.7 Exercises — Chapter 6

**Exercise 6.1:** Write a function `classify_adc(reading: potential) -> void` that classifies an ADC reading into four bands: 0–63 (very low), 64–127 (low), 128–191 (medium), 192–255 (high), and prints the band.

**Exercise 6.2:** Write a function that takes two signals and prints "BOTH HIGH", "BOTH LOW", or "MIXED" based on their states.

**Exercise 6.3:** Rewrite the nested conditional example in 6.6 without any nesting, using `else if` chains instead. Which version is easier to read?

---

## Chapter 7: Loops and Iteration

### 7.1 The `loop` Keyword

NanLang provides a single looping construct: `loop`. It creates an infinite loop that runs forever until a `break` statement is encountered:

```nanlang
fn count_pulses() -> void {
    let mut count: potential = 0;

    loop {
        count = count + 1;
        if count == 10 {
            break;
        }
    }

    say "Counted to 10";
}
```

There is no `while`, `for`, or `do-while`. This is intentional. In hardware programming, most loops are either:
- **Infinite loops** (event loops, polling loops, interrupt handlers) — `loop { ... }`
- **Counted loops** with explicit break conditions — `loop { if done { break; } }`

The `loop` primitive maps to these two patterns directly without adding syntactic complexity.

### 7.2 Event Loop Pattern

The most common pattern in embedded software is the **event loop** — a loop that runs forever, checking for events and responding to them:

```nanlang
fn event_loop() -> void {
    loop {
        let status: potential = @0xFF000000;

        if status > 0 {
            say "Event detected";
            // handle event...
            @0xFF000004 = 0xFF;  // acknowledge
        }

        // loop continues forever, polling the status register
    }
}
```

In production firmware, you would never use a pure spin-loop because it wastes CPU power. But for learning, it clearly expresses the polling logic.

### 7.3 Counted Loop Pattern

To iterate a specific number of times:

```nanlang
fn send_burst(count: potential) -> void {
    let mut sent: potential = 0;

    loop {
        if sent == count {
            break;
        }

        pulse ^;      // emit a high pulse on the default line
        sent = sent + 1;
    }

    say "Burst complete";
}
```

### 7.4 The `break` Statement

`break` immediately exits the innermost `loop`. It can be placed anywhere inside the loop body:

```nanlang
fn find_first(threshold: potential) -> void {
    let mut i: potential = 0;

    loop {
        let reading: potential = @0xFF000010;

        if reading > threshold {
            say "Found above threshold";
            break;
        }

        i = i + 1;
        if i == 255 {
            say "Scan complete — not found";
            break;
        }
    }
}
```

### 7.5 Nested Loops and Break

`break` exits only the **innermost** loop:

```nanlang
fn scan_matrix() -> void {
    let mut row: potential = 0;

    loop {
        if row == 8 { break; }

        let mut col: potential = 0;
        loop {
            if col == 8 { break; }   // exits inner loop only
            col = col + 1;
        }

        row = row + 1;
    }

    say "Matrix scan complete";
}
```

To exit an outer loop from an inner loop, use a flag variable:

```nanlang
fn scan_with_early_exit() -> void {
    let mut found: signal = _;
    let mut row: potential = 0;

    loop {
        if row == 8  { break; }
        if found == ^ { break; }  // outer break via flag

        let mut col: potential = 0;
        loop {
            if col == 8 { break; }
            let val: potential = @0xFF000000;
            if val > 200 {
                found = ^;
                break;
            }
            col = col + 1;
        }

        row = row + 1;
    }
}
```

### 7.6 Exercises — Chapter 7

**Exercise 7.1:** Write a loop that computes the sum of all `potential` values from 1 to 20. Print the result. What happens if you sum to 256?

**Exercise 7.2:** Write a loop that simulates polling a sensor 100 times and breaks early if the reading exceeds 150. Print how many iterations ran.

**Exercise 7.3:** Why does NanLang not have a `for` loop or a `while` loop? Rewrite a typical `for (int i = 0; i < N; i++)` loop in NanLang's `loop`/`break` style. Which do you find clearer?

---

## Chapter 8: Functions

### 8.1 Function Declaration

Functions are the primary unit of code organization in NanLang:

```nanlang
fn function_name(param1: type1, param2: type2) -> return_type {
    // body
    return value;
}
```

A complete example:

```nanlang
fn add(a: potential, b: potential) -> potential {
    return a + b;
}

fn main() -> void {
    say "Adding potentials";
}
```

### 8.2 Calling Functions

Call a function by writing its name followed by arguments in parentheses:

```nanlang
fn double(x: potential) -> potential {
    return x * 2;
}

fn main() -> void {
    let result: potential = double(42);
    say "Function called";
}
```

Arguments are passed **by value** — the function receives a copy of each argument. Modifying a parameter inside the function does not affect the caller's variable.

### 8.3 Multiple Parameters

```nanlang
fn clamp(value: potential, min: potential, max: potential) -> potential {
    if value < min { return min; }
    if value > max { return max; }
    return value;
}
```

### 8.4 Void Functions

Functions with `-> void` do not return a value. They are called for their side effects (output, hardware writes, signal emission):

```nanlang
fn blink(line: signal, times: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == times { break; }
        pulse line;
        i = i + 1;
    }
}
```

### 8.5 The `return` Statement

`return` immediately exits the function and optionally provides a value to the caller:

```nanlang
fn is_valid(reading: potential) -> signal {
    if reading == 0   { return _; }   // invalid: zero reading
    if reading == 255 { return _; }   // invalid: saturated
    return ^;                          // valid
}
```

Early `return` from a void function is also legal:

```nanlang
fn process(data: potential) -> void {
    if data == 0 {
        say "Ignoring zero data";
        return;   // exit early
    }
    say "Processing non-zero data";
}
```

### 8.6 Recursive Functions

Functions can call themselves recursively:

```nanlang
fn power(base: potential, exp: potential) -> potential {
    if exp == 0 { return 1; }
    return base * power(base, exp - 1);
}
```

Be cautious with recursion in embedded contexts — each call consumes stack space. For deep recursion on microcontrollers with limited stack (sometimes only 256 bytes), prefer iterative solutions.

### 8.7 Function Decomposition

A key skill is breaking a large problem into small, focused functions:

**Bad — everything in one function:**
```nanlang
fn main() -> void {
    // 50 lines of mixed initialization, reading, processing, output...
}
```

**Good — decomposed into focused functions:**
```nanlang
fn initialize_hardware() -> void { /* ... */ }
fn read_sensor() -> potential { /* ... */ }
fn process_reading(raw: potential) -> potential { /* ... */ }
fn transmit_result(result: potential) -> void { /* ... */ }

fn main() -> void {
    initialize_hardware();
    let raw:    potential = read_sensor();
    let result: potential = process_reading(raw);
    transmit_result(result);
}
```

Each function has one responsibility, a clear input, and a clear output. This makes the code testable, reusable, and easy to debug.

### 8.8 Exercises — Chapter 8

**Exercise 8.1:** Write a function `max(a: potential, b: potential) -> potential` that returns the larger of two values. Write a function `min` similarly.

**Exercise 8.2:** Write a function `is_power_of_two(n: potential) -> signal` that returns High if `n` is a power of two.

**Exercise 8.3:** Write a recursive function that computes the nth Fibonacci number (capped at 255 due to potential overflow). What is the maximum n you can compute before overflow?

**Exercise 8.4:** Decompose a hypothetical "read ADC and transmit over UART" task into 4–6 small functions, each doing exactly one thing.

---

## Chapter 9: Error Handling and Interrupts

### 9.1 The `on Error` Block

NanLang functions can declare an inline error handler using `on Error`:

```nanlang
fn read_register(addr: potential) -> potential {
    on Error {
        say "Register read failed";
        return 0;
    }
    return @addr;
}
```

If the hardware register access at `@addr` triggers a fault (invalid address, bus error, etc.), the `on Error` block executes. This is similar to a try/catch block in exception-based languages, but it is evaluated at the hardware level.

### 9.2 The `on interrupt` Handler

NanLang can register interrupt handlers for hardware interrupt vectors:

```nanlang
on interrupt(0x20) {
    // This block executes when interrupt vector 0x20 fires
    // (e.g., a hardware timer tick)
    say "Timer interrupt fired";
    @0xFFFFFFF0 = 0x01;  // acknowledge interrupt
}
```

The integer argument to `interrupt()` is the interrupt vector number. On x86, vector `0x20` is the first user-defined hardware interrupt (PIC timer). On ARM Cortex-M, vectors start at 16 (0x10).

### 9.3 The `halt` Keyword

`halt` terminates execution immediately, regardless of where it appears in the code:

```nanlang
fn critical_check(voltage: potential) -> void {
    if voltage > 240 {
        say "CRITICAL: Overvoltage detected — halting";
        halt;  // stops the VM or native process immediately
    }
    say "Voltage nominal";
}
```

In the bytecode VM, `halt` maps to `OP_HALT` (opcode `0x01`). In a native binary, it maps to `_exit(0)`.

### 9.4 The `purge` Keyword

`purge` immediately zeroes a memory region without waiting for the variable to go out of scope:

```nanlang
fn handle_key(key: str) -> void {
    // Use the key for authentication...
    // ...
    purge key;  // immediately overwrite the key bytes with zeros
    say "Key purged";
}
```

This is the explicit version of what `str`'s destructor does automatically. Use `purge` when you need to guarantee that sensitive data is wiped at a specific point in the code rather than at block exit.

### 9.5 Exercises — Chapter 9

**Exercise 9.1:** Write a function that reads from a hardware address inside an `on Error` block and returns a safe default value on failure.

**Exercise 9.2:** Why is `halt` important in a safety-critical system? Give two real-world examples where you would use it.

**Exercise 9.3:** What is the difference between `purge key` and simply waiting for `key` to go out of scope? When does the distinction matter?

---

# Part III — Signals and Hardware

---

## Chapter 10: The Signal System

### 10.1 Signals as Physical States

In NanLang, a `signal` is not just a boolean — it models a physical electrical state. When you write `let line: signal = ^;`, you are asserting that a particular wire or pin is at logic-high voltage (~3.3V or ~5V depending on the logic family).

This distinction matters because:
- Physical signals can be **uncertain** (`?`) — a floating input with no driver
- Physical signals can have **edge transitions** (`_>_` for falling, `^>^` for rising conceptually)
- Physical signals can be **corrupted** by noise — which is why error correction (Hamming) is part of the standard library

### 10.2 The Three Signal States

| State | Literal | Physical Meaning              | Binary Value |
|-------|---------|-------------------------------|--------------|
| High  | `^`     | Logic 1, positive voltage     | 1            |
| Low   | `_`     | Logic 0, ground               | 0            |
| Uncertain | `?` | Floating / undefined         | 0xFF         |

The `?` state is crucial for modeling open-collector bus lines (like I2C) where multiple devices can pull the bus low but no device actively drives it high — the bus "floats" when no device is pulling it.

### 10.3 Signal Operations in Practice

```nanlang
fn gate_logic(a: signal, b: signal) -> void {
    // AND gate
    let and_out: signal = a ^&^ b;

    // NOT gate (using XOR with High)
    // (implementation depends on hardware operator support)

    // Uncertainty propagation
    let merged: signal = a _?_ b;

    say "Gate operations complete";
}
```

### 10.4 The `pulse` Keyword

`pulse` emits a signal on a hardware line:

```nanlang
fn strobe(line: signal) -> void {
    pulse line;   // drive the line to the signal's state
}
```

In the runtime, `pulse` maps to an `AsyncPulseEmitter::emit()` call, which writes the signal to a cache-aligned ring buffer and issues a store fence. This makes `pulse` operations non-blocking and cache-friendly.

### 10.5 The `emit` Keyword

`emit` is similar to `pulse` but explicitly targets the async emitter queue:

```nanlang
fn batch_emit() -> void {
    emit ^;    // enqueue High signal
    emit _;    // enqueue Low signal
    emit ^;    // enqueue High signal
    say "3 signals queued";
}
```

`emit` is used when you want to queue multiple signals for later transmission, rather than immediately driving a hardware line.

### 10.6 Exercises — Chapter 10

**Exercise 10.1:** Model a simple SPI clock signal: 8 alternating pulses (High, Low, High, Low...). Use a loop and `pulse`.

**Exercise 10.2:** Write a function that detects a falling edge: returns `^` if the previous sample was High and the current sample is Low.

**Exercise 10.3:** Why does NanLang have a distinct `?` signal state? Name two hardware scenarios where a floating signal would cause a real problem.

---

## Chapter 11: Potentials and Energy Levels

### 11.1 Potentials as Voltage Quantization

The `potential` type represents a voltage level quantized to 256 steps. A `potential` of 0 maps to 0V, and 255 maps to the full rail voltage (3.3V or 5V depending on hardware). Any intermediate value represents a proportional voltage.

This is the natural output format of an **ADC** (Analog-to-Digital Converter): an 8-bit ADC reading maps directly to a `potential`.

### 11.2 Working with ADC Readings

```nanlang
fn read_temperature() -> potential {
    // ADC channel 0: temperature sensor
    let raw_adc: potential = @0x40012400;  // ADC data register

    // Convert to Celsius (simplified linear scaling)
    // raw = 0 -> 0°C, raw = 255 -> 100°C
    // celsius = raw * 100 / 255 ≈ raw * 0.39
    // Using integer arithmetic: celsius = (raw * 39) / 100
    // But we only have potential (0-255), so:
    let celsius: potential = raw_adc / 2;  // simplified: 0-127°C

    return celsius;
}
```

### 11.3 DAC Output

Writing a `potential` to a DAC (Digital-to-Analog Converter) register generates an analog voltage:

```nanlang
fn set_volume(level: potential) -> void {
    // DAC channel 1: audio output
    @0x40007408 = level;  // write to DAC data holding register
    say "Volume set";
}
```

### 11.4 PWM Duty Cycle

Potentials are ideal for PWM (Pulse Width Modulation) duty cycles:

```nanlang
fn set_pwm(duty: potential) -> void {
    // duty: 0 = 0% (always low), 255 = 100% (always high)
    // Timer capture/compare register:
    @0x40010034 = duty;
    say "PWM duty cycle set";
}

fn fade_up() -> void {
    let mut brightness: potential = 0;
    loop {
        set_pwm(brightness);
        brightness = brightness + 1;
        if brightness == 0 { break; }  // wraps to 0 after 255
    }
}
```

### 11.5 Exercises — Chapter 11

**Exercise 11.1:** Write a function `map_range` that takes a `potential` value in range [0, 255] and maps it to a value in range [lo, hi] using linear interpolation.

**Exercise 11.2:** Write a function that reads an 8-bit ADC value and classifies it: below 64 (low), 64–191 (normal), above 191 (high). Return the classification as a `potential` code (1, 2, or 3).

---

## Chapter 12: Pulse Emission

### 12.1 The AsyncPulseEmitter

At the runtime level, all `pulse` and `emit` operations go through the `AsyncPulseEmitter` — a lock-free, cache-aligned ring buffer. Understanding its behavior helps you write more efficient signal code.

```nanlang
fn emit_pattern(count: potential, pattern: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == count { break; }
        emit pattern;
        i = i + 1;
    }
    say "Pattern emitted";
}
```

**What happens at the hardware level:**
1. Each `emit` call atomically increments a write position counter (`fetch_add` with `memory_order_relaxed`)
2. The signal byte is written to `cache_buf[pos % sizeof(cache_buf)]`
3. An `_mm_sfence()` (store fence) ensures the write is globally visible before the next operation

The ring buffer is 1024 bytes (64 cache lines). If you emit more than 1024 signals without flushing, earlier signals are overwritten — this is intentional, modeled on hardware FIFO overflow behavior.

### 12.2 Flushing the Emitter

When a transmission is complete, flush the emitter to reset its state:

```nanlang
fn transmit_frame(data: potential, size: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == size { break; }
        emit data;
        i = i + 1;
    }
    // Signal that the frame is complete — this resets the ring buffer
    // (In NanLang source this is handled by the runtime automatically,
    //  but you can trigger it explicitly at the bytecode level)
    say "Frame transmitted";
}
```

### 12.3 The ClockCycleSync

For precise timing, use the `ClockCycleSync` to measure the cost of an operation in CPU cycles:

```nanlang
fn timed_emit(count: potential) -> void {
    // calibrate starts the RDTSC counter
    // emit your signals
    // elapsed() returns the delta
    say "Timing complete";
}
```

At the C++ level:
```cpp
ClockCycleSync clock;
clock.calibrate();
for (int i = 0; i < count; ++i) emitter.emit(pattern);
uint64_t cycles = clock.elapsed();
// cycles / count = cycles per emit operation
```

On a 3 GHz CPU, 1 ns = 3 cycles. A well-implemented `emit()` should cost fewer than 10 cycles (< 3 ns).

### 12.4 Exercises — Chapter 12

**Exercise 12.1:** Run `make demo-pulse COUNT=1000` and note the cycles-per-emit. Repeat with `COUNT=64`. Why might the two numbers differ?

**Exercise 12.2:** Run `make demo-sigbuf COUNT=200`. What happens when you push more items than the buffer capacity (512)? Check the source in `src/pulse_emitter.cpp`.

---

## Chapter 13: Hardware Register Access

### 13.1 Memory-Mapped I/O

Modern microcontrollers and SoCs map hardware peripheral registers directly into the processor's address space. Reading from or writing to these addresses communicates with the hardware.

Example (STM32F4 GPIO):
```
Address     | Register          | Purpose
0x40020000  | GPIOA_MODER       | Pin direction (input/output)
0x40020008  | GPIOA_IDR         | Input data (read pins)
0x40020014  | GPIOA_ODR         | Output data (write pins)
```

In NanLang:
```nanlang
const GPIOA_MODER: potential = 0x40;   // simplified address (lower byte)
const GPIOA_IDR:   potential = 0x48;
const GPIOA_ODR:   potential = 0x54;

fn configure_gpio_output(pin: potential) -> void {
    let mode: potential = @GPIOA_MODER;
    // Set pin mode to output (bit manipulation)
    @GPIOA_MODER = mode;
    say "GPIO configured";
}
```

### 13.2 The RegisterMap

The NanLang VM's `RegisterMap` tracks which virtual registers (R0–R15) are currently in use. You can interact with it programmatically using the register bind operator:

```nanlang
fn register_demo() -> void {
    let r0: potential = @R0;   // read virtual register R0
    @R1 = 42;                  // write 42 to virtual register R1
    say "Register operations done";
}
```

The register map demo shows allocation and deallocation:
```bash
./nanlang --demo regmap
# [REGS] R0=42 R1=99 R2=7 used=3
# [REGS] After free R1 used=2
```

### 13.3 Exercises — Chapter 13

**Exercise 13.1:** Run `make demo-regmap`. Walk through the output and match each line to the corresponding code in `src/arch_ops.cpp`.

**Exercise 13.2:** What is "register spill"? When does it happen in the NanLang register allocator, and where does the spilled value go?

---

# Part IV — Memory and Performance

---

## Chapter 14: The Memory Model

### 14.1 Why a Linear Memory Pool?

Dynamic memory allocation (`malloc`/`new`) introduces non-determinism: the time taken to allocate a block depends on the current state of the heap — whether free blocks are fragmented, how many other threads are competing, etc. In a real-time signal processor, a `malloc` that takes 50 µs instead of 50 ns can cause an entire frame of audio or sensor data to be lost.

NanLang's solution: **allocate everything up front**. The `ExecutionEngine` pre-allocates an 8 MB contiguous buffer at startup. All runtime data — registers, string storage, signal buffers — is drawn from this pool. The allocation is a single, predictable `std::vector::resize()` call that happens once, before the main loop runs.

```cpp
// From src/engine.cpp
ExecutionEngine() {
    // Allocate 8MB to minimize dynamic allocation during runtime.
    linear_memory.resize(8 * 1024 * 1024);
}
```

### 14.2 Cache-Line Awareness

Modern CPUs fetch data in 64-byte cache lines. If a frequently accessed variable shares a cache line with a different variable modified by another thread (**false sharing**), each modification invalidates the other thread's cache, causing massive performance degradation.

NanLang prevents this with `alignas(64)`:

```nanlang
// At the C++ level, hot variables are declared like this:
// alignas(64) uint8_t cache_buf[CACHE_LINE * 16]{};
// alignas(64) std::atomic<size_t> write_pos{0};
```

Each of these gets its own 64-byte cache line. The CPU can fetch and modify them independently.

### 14.3 Exercises — Chapter 14

**Exercise 14.1:** Run `make demo-dma SIZE=65536`. What does the output tell you about the transfer correctness? Increase SIZE to 1048576 (1 MB). Does the result change?

**Exercise 14.2:** Research "false sharing" and write a two-paragraph explanation of why `alignas(64)` in `AsyncPulseEmitter` prevents it.

---

## Chapter 15: Zero-Copy Programming

### 15.1 The Cost of Copying

Every time you copy a byte from one buffer to another, you:
1. Read it from the source into a CPU register (possibly from RAM, possibly from cache)
2. Write it from the register to the destination (possibly evicting useful cache lines)

For a 1 MB bytecode file, copying it costs approximately 1 million memory reads and writes. At ~10 ns each (when cold in memory), that is 10 ms — 10 ms of pure overhead that produces no useful work.

### 15.2 ZeroCopySlice

`ZeroCopySlice` avoids this overhead by holding only a pointer and a length — no data is copied:

```cpp
// Full buffer:
ZeroCopySlice full(data);      // ptr = data.data(), len = data.size()

// Sub-slice (bytes 16..32):
ZeroCopySlice slice = full.sub(16, 16);  // ptr = data.data()+16, len = 16
// NO data was copied. slice just points into the middle of data.
```

In NanLang source:
```nanlang
fn process_header(firmware: shadow) -> void {
    // Conceptually: take a zero-copy view of the first 64 bytes
    // of the firmware image (the ELF header)
    say "Processing ELF header zero-copy";
}
```

### 15.3 When NOT to Use Zero-Copy

Zero-copy is not always the right choice:
- If the source buffer may be modified or freed before you finish reading the slice, you have a dangling pointer — a critical bug.
- If you need to modify the data (patching), you must copy it first; you cannot modify through a `const` slice.

The `BinaryPatcher` demonstrates the correct pattern: it reads the file into an owned buffer (`image`), modifies the buffer in place, and writes the result. No zero-copy for modifications.

### 15.4 Exercises — Chapter 15

**Exercise 15.1:** Run `make demo-zerocopy OFFSET=8 LEN=4`. The output shows `first_byte=0xXX`. Manually compute what this byte should be based on the initialization in `src/arch_ops.cpp`.

**Exercise 15.2:** What would happen if you created a `ZeroCopySlice` from a local vector, returned the slice from the function, and then accessed it? Why is this dangerous?

---

## Chapter 16: SIMD and Vectorization

### 16.1 What is SIMD?

SIMD (Single Instruction, Multiple Data) allows one CPU instruction to operate on many data elements simultaneously. The AVX2 extension adds 256-bit (32-byte) vector registers to x86-64 CPUs.

Normal scalar XOR:
```
Instruction: XOR  Processes: 1 byte  Throughput: 1 byte/instruction
```

AVX2 vector XOR:
```
Instruction: VPXOR  Processes: 32 bytes  Throughput: 32 bytes/instruction
```

For signal processing workloads that operate on streams of bytes, SIMD gives a 32× throughput improvement.

### 16.2 SIMDProcessor Usage

```nanlang
// At the C++ level, SIMD operations look like this:
// SIMDProcessor::xor32(a, b, out);   // 32 XOR ops in 1 instruction
// SIMDProcessor::or32(a, b, out);    // 32 OR ops in 1 instruction
// int active = SIMDProcessor::count_active(out, 32);
```

Run the SIMD demo:
```bash
./nanlang --demo simd --iterations 1000
# [SIMD] iterations=1000 active_bytes=32
```

### 16.3 The Scalar Fallback

Every SIMD function in NanLang has an automatic scalar fallback:

```cpp
static void xor32(const uint8_t* a, const uint8_t* b, uint8_t* out) noexcept {
#ifdef __AVX2__
    __m256i va = _mm256_loadu_si256(...);
    __m256i vb = _mm256_loadu_si256(...);
    _mm256_storeu_si256(..., _mm256_xor_si256(va, vb));
#else
    for (int i = 0; i < 32; ++i) out[i] = a[i] ^ b[i];  // scalar fallback
#endif
}
```

If the compiler was invoked without `-mavx2`, or the CPU does not support AVX2, the loop runs instead. The output is identical, just slower.

### 16.4 Alignment Requirements

AVX2 loads/stores perform best with 32-byte aligned data. NanLang uses `alignas(32)` on SIMD input buffers:

```nanlang
// At the C++ level:
// alignas(32) uint8_t a[32], b[32], out[32];
```

Unaligned access (`_mm256_loadu_si256`) is used to handle cases where alignment cannot be guaranteed, at a slight performance cost on older CPUs.

### 16.5 Exercises — Chapter 16

**Exercise 16.1:** Run `make demo-simd ITERATIONS=10000`. Record the number of active bytes. Why is it always 32?

**Exercise 16.2:** Look at `include/nan_arch.h`. Explain in plain English what `SIMDProcessor::count_active()` does. Could this be implemented more efficiently with a hardware popcount instruction?

---

## Chapter 17: Timing and Cycle Counting

### 17.1 RDTSC — Reading the CPU Clock

The x86 `RDTSC` (Read Time-Stamp Counter) instruction returns the number of CPU cycles since the processor was last reset. NanLang wraps this in `ClockCycleSync`:

```cpp
ClockCycleSync clock;
clock.calibrate();   // records baseline TSC
// ... work ...
uint64_t cycles = clock.elapsed();  // TSC delta since calibrate()
```

Converting cycles to nanoseconds:
```
nanoseconds = cycles / (CPU_frequency_GHz)
Example: 3000 cycles / 3.0 GHz = 1000 ns = 1 µs
```

### 17.2 Why RDTSCP Instead of RDTSC?

`RDTSCP` (with the `P` suffix) is a **serializing** variant that prevents out-of-order execution from placing the TSC read in the wrong position relative to the code being timed. `RDTSC` without serialization can be speculatively executed, giving incorrect timings.

### 17.3 Cycle-Level Spin Wait

`ClockCycleSync::wait_cycles()` provides a spin-wait accurate to within a few cycles:

```nanlang
// At the C++ level:
// ClockCycleSync::wait_cycles(3000);  // wait ~1 µs on 3 GHz CPU
```

This is more precise than `usleep()` or `nanosleep()` which go through the OS scheduler and have minimum latencies of ~50 µs.

### 17.4 Exercises — Chapter 17

**Exercise 17.1:** Run `make demo-pulse COUNT=64` and record cycles/emit. Then run `make demo-pulse COUNT=1024`. Why are the numbers different?

**Exercise 17.2:** Research the difference between `RDTSC` and `RDTSCP`. Under what circumstances would using `RDTSC` give you wrong timing results?

---

# Part V — Compiler Features

---

## Chapter 18: Constant Folding

### 18.1 What is Constant Folding?

Constant folding is a compiler optimization that evaluates arithmetic expressions involving only constants at compile time, replacing the expression with its result.

Without folding:
```
Bytecode: [ADD] [10] [20]   → runtime: compute 10+20=30
```

With folding:
```
Bytecode: [LOAD] [30]       → runtime: just load 30
```

The saving is one byte per folded instruction and elimination of a runtime arithmetic operation.

### 18.2 NanLang's Constant Folder

```cpp
// From include/nan_compiler.h
static std::vector<uint8_t> fold_bytecode(const std::vector<uint8_t>& in) {
    // Scan for [ADD|SUB|XOR] [imm1] [imm2] patterns
    // Replace with [LOAD] [result]
}
```

Run the demo:
```bash
./nanlang --demo fold
# [FOLD] original=9 folded=7
```

The 9-byte input becomes 7 bytes: two 3-byte arithmetic instructions each become 2-byte LOAD instructions, saving 2 bytes total.

### 18.3 When Folding Cannot Apply

Folding only applies to instructions with **literal operands**. If an operand comes from a register or memory, its value is not known at compile time and folding is impossible:

```
ADD R0, R1     <- cannot fold: R0 and R1 are runtime values
ADD 10, 20     <- can fold: both operands are compile-time constants
```

### 18.4 Exercises — Chapter 18

**Exercise 18.1:** Create a bytecode sequence by hand (using `echo` + `xxd`) that contains two foldable ADD instructions. Run it through the constant folder and verify the output is smaller.

**Exercise 18.2:** Why doesn't NanLang fold `MUL 2, 128`? (Hint: check the `ConstantFolder::fold()` function for which opcodes are supported.)

---

## Chapter 19: Register Allocation

### 19.1 The Register Coloring Problem

When the compiler translates a program to machine code, it must assign each variable to a physical CPU register. The problem: CPUs have a limited number of registers (NanLang's VM has 16), but a program can have many more live variables simultaneously.

Two variables **conflict** if they are both live at the same point — they need different registers. This is graph coloring: assign a "color" (register number) to each variable such that no two conflicting variables share a color.

### 19.2 NanLang's Greedy Coloring

```cpp
// From include/nan_compiler.h
bool color() {
    for (int i = 0; i < (int)vars.size(); ++i) {
        // Find which colors (registers) are used by neighbors
        // Assign the lowest unused color
    }
    return true;  // or false if spill needed
}
```

Run the demo:
```bash
./nanlang --demo color
# [COLOR] success=1
#   var=x → R0
#   var=y → R1
#   var=z → R2
#   var=w → R2
```

Notice `z` and `w` share R2! They do not conflict (they are never live at the same time), so they can safely share a register.

### 19.3 Register Spilling

When the graph cannot be colored with K=16 colors, some variables must be **spilled** to memory:
1. The variable is written to a location in the linear memory pool
2. Before each use, it is loaded from memory
3. After each write, it is stored back to memory

Spilling adds 2 memory accesses (load + store) per use. For hot variables accessed thousands of times per second, this is significant.

### 19.4 Exercises — Chapter 19

**Exercise 19.1:** Add a 5th variable to the register coloring demo (modify the C++ and recompile). Does the coloring still succeed? What is the assignment?

**Exercise 19.2:** Create a conflict graph that requires exactly 3 colors. How many registers would spill if you only had 2 available?

---

## Chapter 20: The Linker

### 20.1 What Does a Linker Do?

A linker combines multiple compiled segments (object files or bytecode chunks) into a single executable. It:
1. Concatenates code segments in order
2. Resolves cross-segment references (symbol addresses)
3. Strips padding (in NanLang's case, NOP bytes)

### 20.2 NanLinker

```cpp
NanLinker linker;
linker.add_segment(segment_a);   // bytecode from module A
linker.add_segment(segment_b);   // bytecode from module B
auto linked = linker.link(/*strip_padding=*/true);
```

Run the demo:
```bash
./nanlang --demo linker --segments 3 --strip-nops
# [LINKER] segments=3 total_raw=24 linked=18 strip_nops=1
```

24 raw bytes → 18 linked bytes. The 6-byte reduction comes from stripping NOP padding bytes inserted by the compiler for segment alignment.

### 20.3 Exercises — Chapter 20

**Exercise 20.1:** Run the linker demo with and without `--strip-nops`. Compare the sizes. What percentage of the raw bytes are NOP padding?

**Exercise 20.2:** In the Makefile, the build rule chains multiple `.cpp` files. Which file corresponds to each "segment" in the NanLinker's model?

---

## Chapter 21: JIT Compilation

### 21.1 How JIT Works

JIT (Just-In-Time) compilation generates native machine code at runtime and executes it. The steps:

1. **IR construction:** translate source or bytecode to an intermediate representation
2. **Code generation:** emit machine bytes for the target architecture
3. **Memory allocation:** allocate an executable memory page via `mmap`
4. **Execution:** cast the memory address to a function pointer and call it

### 21.2 The W^X Security Constraint

Modern operating systems enforce **W^X** (Write XOR Execute): a memory page cannot be simultaneously writable and executable. NanLang respects this:

```cpp
// Step 1: allocate RWX page for writing
void* mem = mmap(nullptr, size, PROT_READ|PROT_WRITE|PROT_EXEC, ...);

// Step 2: write machine code
memcpy(mem, machine_code.data(), size);

// Step 3: remove write permission (now RX only)
mprotect(mem, size, PROT_READ|PROT_EXEC);

// Step 4: execute
auto fn = reinterpret_cast<void(*)()>(mem);
fn();
```

### 21.3 Running the JIT Demo

```bash
./nanlang --demo jit
# [JIT] Stub executed successfully.
```

The stub being executed is `{0x31, 0xC0, 0xC3}` — three x86-64 bytes:
- `0x31 0xC0` = `XOR EAX, EAX` (set EAX to zero)
- `0xC3` = `RET` (return to caller)

This is the simplest possible valid function: it returns the integer 0.

### 21.4 Exercises — Chapter 21

**Exercise 21.1:** Look at `JITPulseGen::make_ret_stub()` in `include/nan_compiler.h`. What does it return? What would happen if you called the function pointer returned from compiling this stub?

**Exercise 21.2:** Why is JIT not supported on Windows in the current implementation? What Windows API call would replace `mmap` + `mprotect`?

---

## Chapter 22: Cross-Platform Code Generation

### 22.1 The IR Approach

NanLang's `CrossPlatformEmitter` separates **what** code does (IR) from **how** it is implemented on a specific CPU (machine code). This is the same approach used by LLVM, GCC, and virtually every modern compiler.

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

### 22.2 x86-64 Encoding

For the IR above, the x86-64 output is:
```
B8 2A 00 00 00   MOV EAX, 42     (5 bytes: opcode + imm32)
05 08 00 00 00   ADD EAX, 8      (5 bytes: opcode + imm32)
C3               RET              (1 byte)
```

### 22.3 ARM64 Encoding

ARM64 uses fixed-width 32-bit instructions in little-endian order:
```
00 05 80 D2      MOV X0, #42     (4 bytes: MOVZ encoding)
C0 03 5F D6      RET             (4 bytes: BR LR encoding)
```

Notice ARM64 is 8 bytes for the same logic that takes 11 bytes on x86-64. ARM's fixed-width ISA is more compact for simple operations.

### 22.4 Exercises — Chapter 22

**Exercise 22.1:** Run `make demo-xplatform`. How many bytes are the x86-64 and ARM64 outputs? Add a SUB instruction to the IR and re-run. How do the sizes change?

**Exercise 22.2:** Look at the ARM64 MOV encoding in `CrossPlatformEmitter::emit()`. The encoding is `0xD2800000 | ((operand & 0xFFFF) << 5)`. Decode this: what is the maximum immediate value that can be encoded in one MOVZ instruction?

---

# Part VI — Security Programming

---

## Chapter 23: Secure Strings and Memory Wiping

### 23.1 The Memory Remanence Problem

After a program calls `free()` on a buffer containing a password, the bytes remain in RAM until they are overwritten by a future allocation. An attacker with physical access to the machine can read these bytes from RAM (even after power-off for a short period — "cold boot attack"). An attacker with software access (via a memory scanning vulnerability) can find them in the process's address space.

### 23.2 The Volatile Solution

```cpp
~ManagedString() {
    volatile char* p = &content[0];
    for (size_t i = 0; i < content.size(); ++i) p[i] = 0;
}
```

The `volatile` qualifier tells the compiler: "this memory access has observable side effects that you must not optimize away." Without `volatile`, a smart compiler would recognize that `p[i] = 0` writes to memory that is never read again, classify it as a dead store, and eliminate it as an optimization. With `volatile`, the writes are guaranteed to happen.

### 23.3 Using `str` Correctly

```nanlang
fn authenticate(password: str) -> signal {
    // password bytes are automatically wiped when this function returns
    if password == "correct-horse-battery-staple" {
        return ^;
    }
    return _;
}
```

Implicit wipe on scope exit is convenient, but explicit `purge` is clearer:

```nanlang
fn authenticate_explicit(password: str) -> signal {
    let result: signal = password == "correct-horse-battery-staple";
    purge password;   // explicit: wipe now, not at scope exit
    return result;
}
```

### 23.4 Exercises — Chapter 23

**Exercise 23.1:** Write a function that takes a `str` containing a cryptographic key, uses it in a computation (any computation), and explicitly purges it afterward. Add comments explaining why each step is necessary.

**Exercise 23.2:** Research "dead store elimination" in compiler optimization. Find a real-world security vulnerability that was caused by the compiler optimizing away a memset call.

---

## Chapter 24: Timing Attack Mitigation

### 24.1 What is a Timing Attack?

A timing attack exploits the fact that different inputs cause a program to run for different durations. By measuring response times, an attacker can infer information about secret values.

Classic example — string comparison:
```
"aaaa" vs "correct-horse-battery-staple"
  → fails at position 0 immediately → very fast
"correct-horse-battery-staplx"
  → fails at position 28 → slower
```

By measuring timing, the attacker learns how many characters they got right.

### 24.2 The `jitter` Keyword

NanLang's `jitter` keyword inserts the `security_delay()` call:

```nanlang
fn compare_constant(a: str, b: str) -> signal {
    jitter;   // randomize execution time
    return a == b;
}
```

The `security_delay()` function:
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

This adds 0–31 `PAUSE` instruction cycles of random delay, making the timing distribution wide enough that an attacker cannot reliably measure differences.

### 24.3 Constant-Time Comparison

For truly timing-safe string comparison, all characters must be compared even after a mismatch is found:

```nanlang
fn constant_time_equal(a: str, b: str) -> signal {
    // Compare ALL bytes regardless of mismatches
    // XOR all bytes together: if any differ, result is non-zero
    jitter;
    return a == b;   // NanLang's == on str is constant-time by design
}
```

### 24.4 Exercises — Chapter 24

**Exercise 24.1:** Explain why a naive string comparison (short-circuit on first mismatch) leaks information about the secret. Describe an attack scenario with 4-character PIN codes.

**Exercise 24.2:** Why does `security_delay()` use `rand() % 32` rather than a fixed 32-iteration loop? What would a fixed loop accomplish (hint: think about "constant" vs "random" timing)?

---

## Chapter 25: Hardware Fingerprinting

### 25.1 CPU Identity via CPUID

The x86 `CPUID` instruction returns CPU-specific information including the vendor string, model, stepping, and feature flags. The combination of EBX and EDX from leaf 1 is unique (or nearly unique) per CPU model family.

```cpp
uint32_t eax, ebx, ecx, edx;
__asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
uint64_t cpu_id = ((uint64_t)ebx << 32) | edx;
```

### 25.2 Embedding and Validating

```bash
./nanlang --demo hwfp
# [HWFP] CPU-ID=0xXXXXXXXXXXXXXXXX  binary_with_fp=12 bytes  validate=OK
```

The fingerprint is appended as 8 bytes at the end of the binary. At startup, the program reads these bytes and compares them to the live CPUID output. If they differ, the binary has been moved to a different CPU.

### 25.3 Exercises — Chapter 25

**Exercise 25.1:** Run `make demo-hwfp` twice. Is the CPU-ID the same both times? What would change if you ran it on a different computer?

**Exercise 25.2:** Is hardware fingerprinting with CPUID a strong DRM mechanism? What are its limitations? Research how virtual machines handle CPUID.

---

## Chapter 26: Binary Obfuscation

### 26.1 XOR-Based Obfuscation

NanLang's `hw_key_xor()` function encrypts bytecode by XORing each byte with a byte from the CPU-ID key:

```cpp
for (size_t i = 0; i < data.size(); ++i)
    out[i] = data[i] ^ key[i % 8];
```

This is symmetric: applying the same function twice with the same key recovers the original:

```bash
./nanlang --demo binary --cpu-key 0xCAFEBABEDEADBEEF
# [OBFUS] Original[0]=16 Encrypted[0]=206 Decrypted[0]=16  [MATCH]
```

### 26.2 Limitations

XOR obfuscation is not encryption:
- The key is predictable (derived from CPUID)
- Known-plaintext attacks work trivially (if you know the original bytecode, you can recover the key)
- It only prevents casual static disassembly, not determined reverse engineering

For real protection, use AES-GCM or ChaCha20-Poly1305 with a proper key management system. NanLang's XOR is a demonstration, not a security guarantee.

### 26.3 Exercises — Chapter 26

**Exercise 26.1:** Verify the XOR property: encrypt a value, then encrypt the result with the same key. You should get the original back. Explain why this works mathematically.

**Exercise 26.2:** What is a "known-plaintext attack"? How would you apply it to recover the CPU-ID key if you had both the original and obfuscated bytecode?

---

# Part VII — Binary Engineering

---

## Chapter 27: Binary Patching

### 27.1 What is Binary Patching?

Binary patching modifies an existing compiled binary without recompiling from source. Use cases:
- Fixing a critical bug in a deployed system without access to source
- Bypassing a feature check during testing
- Applying a security hotfix faster than a full recompile/redeploy

```bash
./nanlang --patch ./nanlang --offset 0x100 --bytes "90 90 90"
# Writes three NOPs (0x90) at offset 0x100 in the nanlang binary
# Output: ./nanlang.patched
```

### 27.2 The BinaryPatcher Workflow

1. Load the entire file into a `std::vector<uint8_t>` (the `image`)
2. Apply the patch: `memcpy(image.data() + offset, patch_bytes.data(), N)`
3. Save to `<original>.patched` — the original is never modified

The `find_and_patch()` method searches for a byte sequence and patches it:
```cpp
patcher.find_and_patch(
    {0xDE, 0xAD, 0xBE, 0xEF},     // search for this
    {0x90, 0x90, 0x90, 0x90}       // replace with this
);
```

### 27.3 Exercises — Chapter 27

**Exercise 27.1:** Compile `hello.nl` to `hello.nb`. Use `xxd` to find the offset of the letter 'H' in "Hello". Use `--patch` to change it to a different letter. Verify the change with `xxd hello.nb.patched`.

**Exercise 27.2:** What are the risks of binary patching a production system? Name three things that could go wrong.

---

## Chapter 28: Compression and Self-Healing

### 28.1 RLE Compression

Run-Length Encoding represents consecutive identical bytes as `(count, value)` pairs:

```
Input:   AA AA AA BB CC CC CC CC   (8 bytes)
Output:  03 AA 01 BB 04 CC         (6 bytes, 25% smaller)
```

```bash
./nanlang --demo compress
# [COMPRESS] original=10 compressed=8 restored=10 [OK]
```

RLE is particularly effective on NOP-padded bytecode where long runs of `0x00` are common.

### 28.2 Hamming Error Correction

Hamming(7,4) encodes 4 data bits into 7 bits by adding 3 parity bits. Any single-bit error in the 7-bit code can be detected and corrected.

```bash
./nanlang --demo selfheal --nibble 0x0B
# [HAMMING] nibble=0xb encoded=0x6d corrupted=0x65 corrected=0xb [HEALED]
```

The process:
1. Encode nibble `0x0B` (4 bits) → 7-bit code `0x6D`
2. Corrupt bit 3: `0x6D XOR 0x08 = 0x65`
3. Decode and correct: syndrome bits identify the flipped bit → correct → `0x0B`

### 28.3 When to Use Each

| Technique | Best For                                      | Overhead            |
|-----------|-----------------------------------------------|---------------------|
| RLE       | Compressing homogeneous data (NOP padding)    | 2 bytes per run     |
| Hamming   | Detecting/correcting bit errors in storage    | 75% expansion (4→7 bits) |

### 28.4 Exercises — Chapter 28

**Exercise 28.1:** Compute by hand: what is the Hamming encoding of nibble `0x05` (binary: 0101)? Verify with the demo.

**Exercise 28.2:** RLE has a worst-case: alternating bytes get larger. What input would cause RLE to expand rather than compress? (e.g., `AA BB AA BB...`)

---

## Chapter 29: ELF Internals

### 29.1 The ELF Format

Every Linux executable is an ELF (Executable and Linkable Format) file. It contains:
- A 64-byte **ELF header** with magic bytes, architecture, entry point address
- One or more **program headers** describing memory segments
- The actual code and data

NanLang's `write_minimal_elf()` generates the smallest possible valid ELF: one header, one LOAD segment, and raw machine code.

### 29.2 ELF Header Fields

The 64-byte header NanLang writes:
```
Offset  Size  Value       Meaning
0x00    4     7f 45 4c 46  Magic: \x7FELF
0x04    1     02           64-bit (EI_CLASS)
0x05    1     01           Little-endian (EI_DATA)
0x06    1     01           ELF version 1 (EI_VERSION)
0x10    2     00 02        ET_EXEC (executable file)
0x12    2     3e 00        x86-64 architecture
0x14    4     01 00 00 00  ELF version
0x18    8     entry point  Virtual address of entry (0x400000 + 64 + offset)
0x28    8     40 00 ...    Program header offset (= 64)
0x34    2     40 00        ELF header size (64)
0x36    2     38 00        Program header entry size (56)
0x38    2     01 00        Number of program headers (1)
```

### 29.3 Exercises — Chapter 29

**Exercise 29.1:** Use `write_minimal_elf()` from the C++ API to generate a simple ELF containing the bytes `{0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3}` (MOV EAX, 0; RET). Run it with `./output.elf`. What is the exit code?

**Exercise 29.2:** Open a system binary (e.g., `/bin/ls`) with `xxd | head -4`. Identify the magic bytes, architecture field, and entry point address.

---

# Part VIII — Concurrency

---

## Chapter 30: Wait-Free Data Structures

### 30.1 The Problem with Mutexes

A mutex (mutual exclusion lock) protects shared data by allowing only one thread at a time to access it. But mutexes have a serious problem: if thread A holds the lock and is preempted by the OS scheduler, all other threads waiting for the lock are blocked for the duration of thread A's preemption — potentially milliseconds.

In real-time signal processing, a millisecond stall means dropped samples, corrupted frames, and audible glitches.

### 30.2 Wait-Free vs Lock-Free

| Property       | Blocking | Lock-Free | Wait-Free |
|----------------|----------|-----------|-----------|
| Can deadlock?  | Yes      | No        | No        |
| Can livelock?  | Yes      | Possible  | No        |
| Worst-case latency | Unbounded | Unbounded | Bounded |
| Implementation | Simple   | Moderate  | Complex   |

**Wait-free** guarantees that every thread makes progress in a bounded number of steps, regardless of what other threads are doing. This is the strongest concurrency guarantee possible.

### 30.3 WaitFreeQueue

```cpp
WaitFreeQueue<uint64_t, 256> q;
q.push(42ULL);
uint64_t v;
q.pop(v);  // v == 42
```

```bash
./nanlang --demo waitfree --count 64
# [WAITFREE] count=64 pushed=64 popped=64
```

The key insight: head and tail are separate atomic variables on separate cache lines. The producer only writes `tail`, the consumer only writes `head`. They never contend.

### 30.4 Exercises — Chapter 30

**Exercise 30.1:** Run `make demo-waitfree COUNT=300`. What happens when count exceeds 255 (the queue capacity)? How many items are successfully pushed?

**Exercise 30.2:** Draw a diagram showing the ring buffer state after pushing 5 items and popping 3. Identify head, tail, and the 2 remaining items.

---

## Chapter 31: Deadlock Detection

### 31.1 What is a Deadlock?

A deadlock occurs when two or more threads are each waiting for a resource held by the other:

```
Thread 0: holds Resource A, waiting for Resource B
Thread 1: holds Resource B, waiting for Resource A
→ Neither can proceed. Ever.
```

### 31.2 The Resource Allocation Graph

NanLang's `DeadlockDetector` models the situation as a directed graph:
- Nodes: threads and resources
- Edge `thread → resource`: thread is waiting for resource
- Edge `resource → thread`: thread holds resource

A cycle in this graph indicates a deadlock.

```bash
./nanlang --demo deadlock
# [DEADLOCK] Cycle detected! Thread 1 blocked on resource 0
# [DEADLOCK] T0-R1=0 T1-R0=0 (cycle expected=0,0)
```

### 31.3 DFS Cycle Detection

The `has_cycle()` function uses DFS (depth-first search) to detect cycles:

```cpp
bool has_cycle(int start_thread) const {
    bool visited[MAX_RES]{};
    int cur = start_thread;
    for (int step = 0; step < MAX_RES; ++step) {
        if (cur < 0) return false;      // end of chain
        if (visited[cur]) return true;  // cycle!
        visited[cur] = true;
        int res = waiting[cur];         // what does cur want?
        cur = holder[res];              // who holds it?
    }
    return false;
}
```

If `has_cycle()` returns true, `try_acquire()` returns false without adding the thread to the wait graph — the deadlock never actually forms.

### 31.4 Exercises — Chapter 31

**Exercise 31.1:** Trace through the deadlock demo by hand: draw the resource allocation graph at each `try_acquire()` call. Mark the cycle when it is detected.

**Exercise 31.2:** What is a "livelock"? How is it different from a deadlock? Can the DFS cycle detector detect livelocks?

---

## Chapter 32: The Kernel Scheduler

### 32.1 The TaskScheduler

The `kernel/scheduler.cpp` module implements `TaskScheduler`:

```cpp
class TaskScheduler {
    std::atomic<bool> sync_flag{false};
    std::vector<std::thread> thread_pool;
public:
    void core_sync();
    void join_workers();
};
```

`core_sync()` implements a test-and-set spinlock:
```cpp
void core_sync() {
    while (sync_flag.exchange(true, std::memory_order_acquire)) {
        __builtin_ia32_pause();
    }
    // critical section
    sync_flag.store(false, std::memory_order_release);
}
```

### 32.2 Why PAUSE?

The `__builtin_ia32_pause()` intrinsic emits the x86 `PAUSE` instruction. When used in a spin loop, it:
1. **Reduces power consumption** by hinting to the CPU it is spinning
2. **Reduces contention** on hyperthreaded cores (allows the sibling thread to use the execution units)
3. **Prevents memory order violations** on older x86 CPUs that could otherwise detect the spin-loop and re-order stores incorrectly

Without `PAUSE`, a spinlock on a hyperthreaded core wastes both hardware threads and can cause performance degradation of up to 50%.

### 32.3 Exercises — Chapter 32

**Exercise 32.1:** Build the toolchain with `make debug` and add a print statement inside `core_sync()` to trace lock acquisitions. How many times is it called during `make demo-pulse`?

---

# Part IX — Advanced Topics

---

## Chapter 33: The Branch Predictor

### 33.1 Branch Prediction in CPUs

Modern CPUs execute instructions speculatively — they predict the outcome of a branch before it is evaluated, execute instructions along the predicted path, and discard them if the prediction was wrong. A misprediction costs 15–20 clock cycles (a ~5 ns penalty on a 3 GHz CPU).

For code that runs millions of times per second, branch mispredictions are a major performance drain.

### 33.2 The 2-Bit Saturating Counter

NanLang's `BranchPredictor` uses a 2-bit saturating counter table. "Saturating" means the counter does not overflow — it stays at 3 when incremented past 3, and stays at 0 when decremented past 0.

The prediction logic:
- Counter ≥ 2 → predict Taken
- Counter < 2 → predict Not-Taken

After a Taken branch, the counter increments (up to 3). After a Not-Taken branch, it decrements (down to 0). A branch that is Taken 3 times and then Not-Taken once stays in the "Strongly Taken" state — it correctly predicted the first 3 and was wrong once.

```bash
./nanlang --demo branch --rounds 256
# [BRANCH] rounds=256 correct=170 accuracy=66.4%
```

The test pattern `(i % 3 != 0)` is Taken 2/3 of the time and Not-Taken 1/3. A 2-bit predictor eventually locks onto the dominant direction and achieves ~66% accuracy for this pattern.

### 33.3 Exercises — Chapter 33

**Exercise 33.1:** Run the branch demo with a pattern that is always Taken (modify the test to `taken = true`). What accuracy do you expect? What do you observe?

**Exercise 33.2:** Look at `BranchPredictor::hint_byte()`. What do the values `0x3E` and `0x2E` mean in x86-64 encoding? When would you use them?

---

## Chapter 34: Pipeline Reordering

### 34.1 Data Hazards

Modern CPUs execute instructions in a multi-stage pipeline. A **data hazard** occurs when one instruction depends on the result of a previous one that hasn't completed yet:

```
ADD R3, R1, R2     (cycles 1-4: result in R3 at cycle 4)
ADD R5, R3, R4     (needs R3 at cycle 2: stall 2 cycles)
XOR R6, R0, R1     (independent: no stall)
```

The CPU must stall the second ADD until R3 is available. But the XOR can execute immediately. If we reorder:

```
ADD R3, R1, R2
XOR R6, R0, R1     (moved up: fills the stall cycles)
ADD R5, R3, R4     (R3 is now ready by the time this executes)
```

The stall is hidden. Same output, fewer wasted cycles.

```bash
./nanlang --demo pipeline
# [PIPELINE] original=4 reordered=4 instructions
```

### 34.2 Exercises — Chapter 34

**Exercise 34.1:** Draw the pipeline execution timeline for the 4-instruction program in `perf_ops.cpp`. Show which cycles are stalls with and without reordering.

---

## Chapter 35: Out-of-Order Execution

### 35.1 Tomasulo's Algorithm

Out-of-order execution takes the reordering idea further: the CPU itself reorders instructions dynamically at runtime, using **reservation stations** to hold instructions whose operands are not yet ready.

```bash
./nanlang --demo ooo
# [OOO] stations issued=2 result_bus=30
```

Two instructions are issued simultaneously to the reservation station:
- `ADD 10, 20` → result 30
- `XOR 7, 3` → result 4

Since neither depends on the other, both execute in the same cycle. The `result_bus` shows the result of the last completed instruction.

### 35.2 Exercises — Chapter 35

**Exercise 35.1:** Issue three instructions to the reservation station: `ADD 1, 2`, `ADD result_of_first, 3`, and `XOR 5, 5`. Which ones can execute in parallel? Trace the execution.

---

## Chapter 36: Building a Complete Program

### 36.1 Putting It All Together

Now that you have learned every subsystem, let's build a complete signal processing program:

```nanlang
// signal_processor.nl
// A complete NanLang program that:
// 1. Reads from a sensor register
// 2. Filters the reading
// 3. Compares to a threshold
// 4. Emits an alarm signal if threshold exceeded
// 5. Logs the reading securely

const SENSOR_REG:  potential = 0x40;
const ALARM_REG:   potential = 0x44;
const THRESHOLD:   potential = 200;

fn read_sensor() -> potential {
    on Error {
        return 0;
    }
    return @SENSOR_REG;
}

fn low_pass_filter(current: potential, previous: potential) -> potential {
    // Simple average: (current + previous) / 2
    return (current + previous) / 2;
}

fn check_threshold(reading: potential) -> signal {
    if reading > THRESHOLD {
        return ^;
    }
    return _;
}

fn trigger_alarm() -> void {
    @ALARM_REG = 0xFF;
    pulse ^;
    say "ALARM: Threshold exceeded";
}

fn log_reading(device_id: str, reading: potential) -> void {
    jitter;
    say "Reading logged securely";
    purge device_id;
}

fn main() -> void {
    let device: str = "sensor-primary-001";
    let mut prev_reading: potential = 0;

    loop {
        let raw: potential       = read_sensor();
        let filtered: potential  = low_pass_filter(raw, prev_reading);
        let alarm: signal        = check_threshold(filtered);

        if alarm {
            trigger_alarm();
        }

        log_reading(device, filtered);
        prev_reading = filtered;
    }
}
```

This program demonstrates:
- All five types
- Functions with parameters and return values
- Hardware register access (`@`)
- Error handling (`on Error`)
- Timing attack mitigation (`jitter`)
- Secure memory (`str`, `purge`)
- Signal operations (`pulse`)
- Constants
- Loop with break-free event loop pattern
- Function decomposition

### 36.2 Compiling and Running

```bash
./nanlang --compile signal_processor.nl --output signal_processor.nb
./nanlang --run signal_processor.nb

# Or as a native binary:
./nanlang --build-native signal_processor.nl --output signal_processor --opt
./signal_processor
```

---

## Chapter 37: Debugging NanLang Programs

### 37.1 The Debug Build

Always develop with the debug build:
```bash
make debug
```

This enables:
- **AddressSanitizer (ASan)**: detects out-of-bounds memory access, use-after-free, memory leaks
- **UndefinedBehaviorSanitizer (UBSan)**: detects integer overflow, null pointer dereference, misaligned access
- **No optimization**: code runs exactly as written, line by line

### 37.2 Inspecting Bytecode

Use `xxd` to inspect compiled `.nb` files:
```bash
./nanlang --compile program.nl --output program.nb
xxd program.nb
```

Map each byte to the opcode table (§9.1 of README) to verify the compiler output matches your expectations.

### 37.3 Adding Debug Output

Add `say` statements to trace execution:

```nanlang
fn process(data: potential) -> potential {
    say "process: entering";
    let result: potential = data * 2;
    say "process: computed result";
    return result;
}
```

Remove them before building for production — they add overhead and generate bytecode output.

### 37.4 Common Runtime Errors

| Error Message                  | Cause                                        | Fix                                              |
|-------------------------------|----------------------------------------------|--------------------------------------------------|
| `[RT] Invalid magic.`         | File not compiled by NanLang, or corrupted   | Recompile from source                            |
| `[RT] Could not open: file`   | File not found or wrong path                 | Verify file path; check working directory        |
| `[JIT] mmap failed`           | Insufficient memory or permissions           | Check `ulimit -v`; run as non-root               |
| `[DMA] mmap failed`           | `MAP_LOCKED` not permitted                   | `ulimit -l unlimited`                            |
| `[PATCH] File not found`      | Binary path incorrect in `--patch`           | Use absolute path or check working directory     |
| `[DEADLOCK] Cycle detected!`  | Resource acquisition order conflict          | Acquire resources in consistent order            |

### 37.5 Exercises — Chapter 37

**Exercise 37.1:** Introduce an intentional bug in `hello.nl` (e.g., forget a closing brace). What error do you get? How does it compare to a C++ compiler error?

**Exercise 37.2:** Compile a program with `make debug`, then run it under Valgrind: `valgrind ./nanlang --demo pulse`. What does Valgrind report?

---

## Chapter 38: Performance Profiling

### 38.1 Using RDTSC for Micro-benchmarks

The `ClockCycleSync` timer is your primary profiling tool:

```bash
# Run the pulse demo and observe cycles-per-emit
./nanlang --demo pulse --count 1024
# [PULSE] Done. Buffer pos=1024  cycles=8192  cycles/emit=8
```

8 cycles/emit at 3 GHz = ~2.7 ns per emit. This is fast — it is close to the theoretical minimum for a cache-line store with atomics.

### 38.2 Identifying Bottlenecks

Profile each subsystem in isolation:
```bash
./nanlang --demo simd    --iterations 10000   # SIMD throughput
./nanlang --demo dma     --size 1048576        # DMA transfer speed
./nanlang --demo waitfree --count 256          # Queue throughput
./nanlang --demo branch  --rounds 10000        # Branch predictor accuracy
```

Compare results before and after optimization changes.

### 38.3 The `make fast` PGO Build

Profile-guided optimization (PGO) uses actual runtime data to guide compiler decisions:

```bash
# Step 1: Build with profiling instrumentation
make fast

# Step 2: Run representative workloads to collect profile data
./nanlang --demo all

# Step 3: Rebuild with profile data (future Make target)
```

PGO typically improves performance by 10–20% for compute-heavy code by:
- Inlining functions that are called frequently
- Unrolling loops with known iteration counts
- Ordering basic blocks to improve branch predictor accuracy

### 38.4 Exercises — Chapter 38

**Exercise 38.1:** Run `make demo-simd ITERATIONS=1000` with the release build. Then rebuild with `make debug` and run again. By what factor is the debug build slower? Why?

**Exercise 38.2:** Using `perf stat ./nanlang --demo pulse --count 10000`, identify the IPC (instructions per cycle) and cache miss rate. Research what "good" values are for these metrics.

---

# Appendices

---

## Appendix A: Quick Reference Card

### Keywords
```
fn       - function declaration
let      - variable binding
let mut  - mutable binding
const    - compile-time constant
return   - return from function
if       - conditional
else     - alternative branch
loop     - infinite loop
break    - exit loop
halt     - stop execution
say      - print string
pulse    - emit signal on hardware line
emit     - enqueue signal to async emitter
jitter   - insert random timing delay
purge    - immediately zero a memory region
on       - register event/error handler
interrupt - specify interrupt vector
void     - no-return type
```

### Types
```
potential  - uint8_t (0-255)
signal     - High(^) | Low(_) | Uncertain(?)
flow       - []signal sequence
str        - secure managed string
shadow     - raw byte buffer
```

### Operators
```
+  -  *  /     - arithmetic (on potential)
==  !=  <  >   - comparison (returns signal)
^&^            - signal AND
_?_            - uncertainty merge
>>>            - signal propagation
=              - assignment
@addr          - hardware register access
```

### CLI Quick Reference
```bash
./nanlang --compile <file.nl> [--output <file.nb>]
./nanlang --run <file.nb>
./nanlang --build-native <file.nl> [--output <n>] [--opt]
./nanlang --patch <file> --offset <hex> --bytes "<hex bytes>"
./nanlang --demo <n> [--count N] [--pattern 0xXX] [--iterations N]
./nanlang --help
./nanlang --version
```

### Make Targets
```bash
make              # release build
make debug        # debug build + sanitizers
make compile      SRC=x.nl OUT=x.nb
make run          NB=x.nb
make native       SRC=x.nl OUT=x OPT=1
make demo-all
make demo-<n>
make clean
```

---

## Appendix B: Common Error Messages

| Error                                   | Meaning                               | Solution                                   |
|-----------------------------------------|---------------------------------------|--------------------------------------------|
| `[RT] Invalid magic.`                   | Bad bytecode file                     | Recompile                                  |
| `[RT] Could not open: path`             | File not found                        | Check path                                 |
| `[FATAL] Source not found: path`        | `.nl` file not found                  | Check path                                 |
| `[ERROR] File not found: path`          | Compile target not found              | Check path                                 |
| `[ERROR] Bytecode not found: path`      | `.nb` file not found                  | Compile first                              |
| `[ERROR] Invalid arguments. Use --help` | Unknown CLI flag                      | Check `--help`                             |
| `[JIT] mmap failed`                     | Cannot allocate executable memory     | Check permissions / `ulimit`               |
| `[JIT] Platform not supported`          | JIT not available on this OS          | Use Linux                                  |
| `[DMA] mmap failed, falling back`       | MAP_LOCKED not available              | `ulimit -l unlimited` or run as root       |
| `[PATCH] Empty image, aborting.`        | Binary file is empty                  | Check target binary path                   |
| `[DEADLOCK] Cycle detected!`            | Deadlock detected, acquire blocked    | Fix resource acquisition order             |
| `[SIZE] Warning: size did not shrink!`  | Optimization pass made no progress    | Check if bytecode is already minimal       |

---

## Appendix C: Exercises Answer Key

### Chapter 1

**1.3 — Forgetting the semicolon:** The compiler (or lexer) will report a syntax error because it expects a semicolon after a statement and finds either the next keyword or the closing brace instead.

**1.4 — Same line as fn:** No. The `{` opens the function body block. Code inside the block must appear after the `{` on the same or a following line.

### Chapter 3

**3.4 — volatile and dead stores:** The compiler's data-flow analysis determines that `p[i] = 0` stores a zero to memory that is immediately freed and never read again. It classifies this as a "dead store" and removes it. `volatile` marks the access as having observable side effects, bypassing this optimization.

### Chapter 5

**5.3 — 8-bit subtraction:** `250 - 5 = 245` (normal). `5 - 10 = 251` (wraps: `-5 mod 256 = 251`). This is unsigned 8-bit wrapping arithmetic.

---

## Appendix D: Further Reading

### Systems Programming Fundamentals
- **"Computer Systems: A Programmer's Perspective"** — Bryant & O'Hallaron. Comprehensive coverage of memory hierarchy, assembly, linking, and system calls.
- **"The Linux Programming Interface"** — Kerrisk. Everything about Linux system calls, including `mmap`, signals, and process management.

### Compiler Design
- **"Engineering a Compiler"** — Cooper & Torczon. Covers lexing, parsing, IR, register allocation, and code generation in depth.
- **"Modern Compiler Implementation in ML/Java/C"** — Appel. A practical, project-oriented compiler course.

### SIMD and Performance
- **"Intel Intrinsics Guide"** — https://www.intel.com/content/www/us/en/docs/intrinsics-guide/. Reference for every AVX2 intrinsic function.
- **"Agner Fog's Optimization Manuals"** — https://agner.org/optimize/. The definitive resource for x86 CPU microarchitecture and optimization.

### Security
- **"The Art of Exploitation"** — Erickson. Low-level security, memory exploits, and mitigation techniques.
- **"Cryptography Engineering"** — Ferguson, Schneier, Kohno. Practical cryptographic engineering including timing attack defenses.

### Hardware Programming
- **ARM Cortex-M Technical Reference Manuals** — ARM developer documentation. Essential for embedded NanLang targets.
- **Intel 64 and IA-32 Architectures Software Developer Manuals** — Intel documentation. Reference for x86 instructions, memory model, and CPUID.

---

*LEARNING.md — NanLang v3.0.2 Complete Learning Guide*
*Covers all 38 chapters from beginner fundamentals to advanced internals*
