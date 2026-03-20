# INTERNALS.md — NanLang Toolchain Internals

> A guided tour of the NanLang source tree: how each module works, why it was built the way it was, and how the pieces fit together.

---

## Source Tree Overview

```
NanLang/
├── main.cpp                  CLI entry point, NanCompiler, NanRuntime
├── Makefile                  Build system
├── build.sh                  Direct build script (no Make)
├── hello.nl                  Example source file
├── include/
│   ├── nan_core.h            Core types: Potential, Signal, ManagedString, Value
│   ├── nan_pulse.h           AsyncPulseEmitter, ClockCycleSync, SignalBuffer
│   ├── nan_arch.h            SIMD, DMA, ZeroCopy, RegisterMap, HugePages
│   ├── nan_binary.h          Opcodes, patching, RLE, Hamming, ELF, obfuscation
│   ├── nan_compiler.h        JIT, Linker, Coloring, ConstantFold, CrossPlatform
│   └── nan_perf.h            Pipeline, OoO, BranchPredictor, WaitFree, Deadlock
├── src/
│   ├── engine.cpp            ExecutionEngine: registers, linear pool, security delay
│   ├── pulse_emitter.cpp     Demo wrappers: run_pulse_emitter_demo, run_signal_buffer_demo
│   ├── arch_ops.cpp          Demo wrappers: SIMD, DMA, ZeroCopy, RegisterMap
│   ├── binary_ops.cpp        Demo wrappers: patching, obfuscation, RLE, Hamming
│   ├── compiler_ops.cpp      Demo wrappers: JIT, linker, coloring, folding, xplatform, hwfp
│   └── perf_ops.cpp          Demo wrappers: waitfree, deadlock, branch, pipeline, OoO
└── kernel/
    └── scheduler.cpp         TaskScheduler: spinlock, thread pool
```

---

## Module 1: `main.cpp` — The CLI and Compiler Front-End

### Responsibility
`main.cpp` is the toolchain's single executable entry point. It owns:
- **Argument parsing** (`Args` struct)
- **Compilation** (`NanCompiler` class)
- **VM execution** (`NanRuntime` class)
- **Demo dispatch** (`run_demo()` function)

### Argument Parsing (`Args`)

`Args::parse()` does a single left-to-right scan of `argv`. Each `--key value` pair is stored in a `std::map<std::string, std::string>`. Each `--flag` (no following value) is stored in `std::map<std::string, bool>`.

The design is intentionally simple — no getopt, no third-party library. For a toolchain with fewer than 20 flags, a hand-rolled scanner is easier to audit and has zero dependencies.

```cpp
// The full parse logic in ~15 lines:
for (int i = 1; i < argc; ++i) {
    std::string tok(argv[i]);
    if (tok starts with "--") {
        key = tok.substr(2);
        if (next arg exists and doesn't start with '-')
            kv[key] = argv[++i];
        else
            flags[key] = true;
    }
}
```

### NanCompiler — `generate_bytecode()`

A single-pass lexical scanner. For each line of the source file:
1. If the line contains `"say "`, extract the quoted string with `extract_string()`.
2. Write `OP_SAY` (1 byte) + length (4 bytes, little-endian) + string bytes.

The magic `0x214E414E` is written first (4 bytes). The output file is opened in binary mode.

This is not a parser — there is no AST. It is a line-pattern scanner sufficient for the `say` subset of NanLang. Expanding to full parsing would require tokenisation, an expression parser, and a code generator — future work.

### NanCompiler — `build_native()`

1. Open the source file, scan for `say "..."` lines.
2. Write a C++ stub to `nan_temp__.cpp` with `std::cout <<` for each string.
3. `std::system()` invokes `g++` on the stub.
4. On success, `std::remove()` deletes the temp file.

Using the system C++ compiler as a backend gives NanLang native binaries access to the full optimization pipeline (`-O3 -march=native -flto`) without reimplementing it.

### NanRuntime — `run()`

```
1. Open file in binary mode.
2. Read 4 bytes → validate magic.
3. Loop: read 1-byte opcode.
   - OP_SAY: read 4-byte length, read N bytes, write to stdout.
   - (future opcodes here)
4. Loop ends at EOF.
```

The VM is intentionally minimal — no call stack, no register file, no arithmetic unit in the current implementation. These are scaffolded in `ExecutionEngine` but not yet wired into the dispatch loop.

---

## Module 2: `include/nan_core.h` — Foundation Types

### `Potential`
A `uint8_t` typedef. 8-bit unsigned, range 0–255, wraps on overflow. Named after electrical potential — the quantized representation of a voltage level.

### `Signal`
An `enum class Signal : uint8_t` with values `Low = 0` and `High = 1`. `enum class` prevents implicit conversion to integer — you cannot accidentally write `Signal + 1`. The `?` (uncertain) state is represented as `0xFF` at the opcode level; enum class support for it is reserved.

### `SignalPacket`
Wraps `__m256i` — a 256-bit AVX2 register holding 32 signal bytes simultaneously. `merge()` is `_mm256_or_si256`: a single-cycle bitwise OR of 256 bits. This struct is the bridge between the NanLang signal model and AVX2 vectorised hardware.

### `ManagedString`
The security-critical type. Key implementation detail: the destructor's zero-wipe uses `volatile char*`, not `char*`. The difference:

```cpp
// Without volatile — compiler MAY eliminate as dead store:
char* p = &content[0];
for (size_t i = 0; i < content.size(); ++i) p[i] = 0;

// With volatile — compiler MUST NOT eliminate:
volatile char* p = &content[0];
for (size_t i = 0; i < content.size(); ++i) p[i] = 0;
```

The `active` flag is a reminder mechanism for future introspection tooling — it tracks whether the string has been zeroed.

### `Value` variant
`std::variant<Potential, Signal, std::vector<Signal>, ManagedString>` unifies all NanLang value types in a single type-safe container. The `ExecutionEngine` register file stores `Value` objects, allowing a register to hold any NanLang type.

---

## Module 3: `include/nan_pulse.h` — Timing and Signal Buffering

### `AsyncPulseEmitter`

The most performance-critical struct in the standard library. Design goals:
1. **Non-blocking:** `emit()` must never wait for a consumer.
2. **Cache-friendly:** all writes go to a cache-aligned local buffer.
3. **Ordered:** writes must be globally visible in program order.

```cpp
alignas(CACHE_LINE) uint8_t cache_buf[CACHE_LINE * 16]{};  // 1024 bytes, cache-aligned
std::atomic<size_t> write_pos{0};                           // separate cache line
```

`emit()` implementation:
```cpp
size_t pos = write_pos.fetch_add(1, memory_order_relaxed) % sizeof(cache_buf);
cache_buf[pos] = signal;
_mm_sfence();   // store fence: make write globally visible
```

Why `memory_order_relaxed` on `fetch_add`? The atomic increment only needs to give us a unique position — it does not need to synchronise with any other thread's reads. The `_mm_sfence()` provides the ordering guarantee for the actual data write.

### `ClockCycleSync`

Uses `__rdtscp()` (serializing TSC read) rather than `__rdtsc()`. The `P` variant ensures the TSC read is not speculatively executed before the preceding instructions complete.

```cpp
uint64_t calibrate() { baseline = __rdtscp(&aux); }
uint64_t elapsed()   { return __rdtscp(&aux) - baseline; }
```

`wait_cycles()` is a precision spin-wait:
```cpp
uint64_t start = __rdtscp(&aux);
while ((__rdtscp(&aux) - start) < cycles)
    __builtin_ia32_pause();
```

At 3.6 GHz, 1 µs = 3,600 cycles. `wait_cycles(3600)` gives approximately 1 µs of delay without any OS involvement.

### `SignalBuffer`

A simple FIFO with head/tail indices and modular arithmetic. The key design choice: no atomics. `SignalBuffer` is single-threaded. If you need thread-safe buffering, use `WaitFreeQueue` from `nan_perf.h`.

The `BUF_SIZE = 512` is a power of two — not coincidentally. The modulo `% BUF_SIZE` in push/pop compiles to a single `AND` instruction when the size is a power of two.

---

## Module 4: `include/nan_arch.h` — Hardware Abstractions

### `SIMDProcessor`

The AVX2 path uses `_mm256_loadu_si256` (unaligned load) rather than `_mm256_load_si256` (aligned load). Why? Callers do not always guarantee 32-byte alignment. On modern CPUs (Haswell+), the performance difference between aligned and unaligned loads is negligible when the data is cache-resident.

The scalar fallback is compiled when `__AVX2__` is not defined:
```cpp
#ifdef __AVX2__
    // AVX2 path: 1 instruction for 32 bytes
#else
    for (int i = 0; i < 32; ++i) out[i] = a[i] ^ b[i];  // scalar: 32 iterations
#endif
```

This pattern appears in all three `SIMDProcessor` methods. The compiler can auto-vectorize the scalar fallback with `-O3 -msse4.2` if SSE4.2 is available.

### `ZeroCopySlice`

The `sub()` method is worth examining closely:

```cpp
ZeroCopySlice sub(size_t offset, size_t count) const noexcept {
    if (offset + count > len) return {};  // out of bounds → empty slice
    return {ptr + offset, count};         // pointer arithmetic only
}
```

No allocation. No copy. The returned slice points into the middle of the original buffer. The calling code is responsible for ensuring the original buffer outlives all slices derived from it.

### `DMAPulse`

The `nt_store()` function processes 32 bytes at a time with `_mm256_stream_si256`, then handles the tail (bytes that don't fill a full 32-byte chunk) with a scalar loop:

```cpp
size_t i = 0;
for (; i + 32 <= len; i += 32) {
    __m256i v = _mm256_loadu_si256(src + i);
    _mm256_stream_si256(dst + i, v);     // non-temporal write
}
_mm_sfence();               // ensure all NT stores are globally visible
for (; i < len; ++i) dst[i] = src[i];   // tail bytes
```

The `_mm_sfence()` between the vector loop and the tail loop is essential — NT stores are weakly ordered and must be fenced before any subsequent memory access.

### `HugePageAlloc`

Huge pages require upfront reservation. If `/proc/sys/vm/nr_hugepages` is 0 (the default), `MAP_HUGETLB` fails and the allocator silently falls back to regular 4 KB pages. This fallback is intentional — the code still works, just with higher TLB pressure.

---

## Module 5: `include/nan_binary.h` — Opcodes and Binary Manipulation

### Opcode Design

The 16-opcode Nan-ISA fits in one nibble (4 bits). This has a practical consequence: two opcodes can be packed into one byte if needed (future compression idea). The current encoding is one opcode per byte, leaving the upper nibble unused (always 0).

Opcodes 0x00–0x0F were chosen based on frequency analysis of typical signal processing programs:
- `NOP` (0x00): padding, always needed
- `SAY` (0x02): primary I/O in current version
- `LOAD`/`STORE` (0x03/0x04): universal
- `ADD`/`SUB`/`XOR`/`OR`/`AND` (0x05–0x0B): arithmetic and logic
- `JMP`/`JZ` (0x07/0x08): control flow
- `SHL`/`SHR` (0x0C/0x0D): shift operations (common in signal processing)
- `CALL`/`RET` (0x0E/0x0F): subroutines

### `BinaryPatcher`

Constructor reads the entire file into `image` (a `std::vector<uint8_t>`). This is a deliberate whole-file-in-memory approach — the file is never partially read or memory-mapped. For the typical binaries NanLang patches (a few KB to a few MB), this is safe.

`find_and_patch()` uses `memcmp` for the search loop — no Boyer-Moore or KMP. For small needles (typically 2–16 bytes) and short binaries, the O(N×M) naive scan is fast enough and avoids the overhead of preprocessing the pattern.

### Hamming Implementation

```cpp
uint8_t hamming_encode(uint8_t nibble) {
    uint8_t d1=(nibble>>3)&1, d2=(nibble>>2)&1, d3=(nibble>>1)&1, d4=nibble&1;
    uint8_t p1 = d1^d2^d4, p2 = d1^d3^d4, p3 = d2^d3^d4;
    return (p1<<6)|(p2<<5)|(d1<<4)|(p3<<3)|(d2<<2)|(d3<<1)|d4;
}
```

Bit positions in the encoded byte (bit 6 = MSB):
```
6   5   4   3   2   1   0
p1  p2  d1  p3  d2  d3  d4
```

Parity checks:
- p1 covers positions {p1, d1, d2, d4} → `p1 = d1^d2^d4`
- p2 covers positions {p2, d1, d3, d4} → `p2 = d1^d3^d4`
- p3 covers positions {p3, d2, d3, d4} → `p3 = d2^d3^d4`

During decoding, syndrome = `(s1<<2)|(s2<<1)|s3` identifies the error bit position (1–7 in standard Hamming notation, mapped to 6–0 in the byte).

---

## Module 6: `include/nan_compiler.h` — Compiler Infrastructure

### `ConstantFolder`

The folding scan is a single forward pass with a 3-byte lookahead:

```cpp
for (size_t i = 0; i < in.size(); ) {
    if (i + 3 < in.size() && (in[i] == ADD || in[i] == SUB || in[i] == XOR)) {
        // fold: [op, a, b] → [LOAD, fold(op, a, b)]
        i += 3;
    } else {
        out.push_back(in[i++]);
    }
}
```

This is O(N) time, O(N) space (new output vector). The decision not to fold in-place avoids index management complexity.

### `RegisterColoring`

The conflict graph is stored as a flat vector of `(int, int)` pairs rather than an adjacency matrix or list. For the typical case of ≤ 20 variables, the flat scan is O(V×E) but faster in practice than a hash map due to cache locality.

The greedy algorithm assigns colors in variable-declaration order. This is not optimal (chromatic number minimization is NP-complete) but is O(V + E) and sufficient for the small graphs encountered in NanLang programs.

### `NanLinker`

The segment store is a `vector<vector<uint8_t>>` — segments are independent byte vectors concatenated during `link()`. The `strip_padding` flag controls whether `strip_nops()` runs on each segment before concatenation.

`total_size()` sums segment sizes without stripping — it represents the unoptimized linked binary size, useful for measuring the effect of NOP stripping.

### `JITPulseGen`

The W^X sequence:
```cpp
// 1. RWX: needed to write the code
void* mem = mmap(nullptr, size, PROT_READ|PROT_WRITE|PROT_EXEC,
                 MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
// 2. Write the machine code
memcpy(mem, machine_code.data(), size);
// 3. Remove write permission (W^X)
mprotect(mem, size, PROT_READ|PROT_EXEC);
// 4. Return callable
return reinterpret_cast<JITFunc>(mem);
```

On Linux, step 1 may be rejected if the kernel enforces strict W^X (`CONFIG_STRICT_DEVMEM` or SELinux policy). Check `/proc/sys/vm/mmap_min_addr` if mmap fails.

### `CrossPlatformEmitter`

x86-64 encoding for common IR operations:
- `LOAD imm32`: `B8 [imm32 LE]` (5 bytes, MOV EAX, imm32)
- `ADD imm32`: `05 [imm32 LE]` (5 bytes, ADD EAX, imm32)
- `RET`: `C3` (1 byte)

ARM64 encoding:
- `LOAD imm16`: `D2800000 | ((imm & 0xFFFF) << 5)` (4 bytes, MOVZ X0, #imm)
- `RET`: `D65F03C0` (4 bytes, BR LR)

ARM64 uses fixed-width 32-bit instructions in little-endian byte order. The MOVZ encoding supports immediates up to 16 bits (0–65535); larger values require MOVK instructions (not yet implemented).

---

## Module 7: `include/nan_perf.h` — Performance Subsystems

### `PipelineReorder`

The reorder algorithm:
```
output = []
scheduled = [false] * N
last_write = [-1] * 64  // last cycle register was written

for each pass (N passes total):
    for each unscheduled instruction i:
        if last_write[i.src1] < output.size() and
           last_write[i.src2] < output.size():
            schedule i; update last_write[i.dst]
            break
```

This is a simple ready-list scheduler. It always picks the first ready instruction (not necessarily the best one). A production scheduler would use priority heuristics (e.g., critical path length) to maximize ILP.

### `ReservationStation`

The `execute_ready()` scan:
```cpp
for (int i = 0; i < RS_SIZE; ++i) {
    auto& e = stations[i];
    if (e.busy && e.qj == -1 && e.qk == -1) {
        // both operands ready — execute
        result_bus = compute(e.op, e.vj, e.vk);
        e.busy = false;
    }
}
```

In a real OoO unit, execution would also broadcast the result to other waiting entries (clearing their `qj`/`qk` fields). This broadcast logic is not implemented in the simulation — it suffices for the demo.

### `BranchPredictor`

The 2-bit saturating counter update:
```cpp
uint8_t& entry = table[pc_hash % HISTORY];
if (taken && entry < 3) ++entry;    // saturate at 3
if (!taken && entry > 0) --entry;   // saturate at 0
```

The prediction is always the current counter state (`>= 2` → Taken). This is a **bimodal predictor** — one of the simplest and most studied branch prediction algorithms. Modern CPUs use more sophisticated designs (TAGE, ITTAGE, perceptron-based), but bimodal is a correct and teachable implementation.

### `WaitFreeQueue`

The power-of-two requirement (`static_assert((N & (N-1)) == 0)`) enables the critical optimization:

```cpp
// Expensive:
size_t next = (t + 1) % N;   // integer division
// Free:
size_t next = (t + 1) & (N - 1);  // single AND
```

The `alignas(64)` on head, tail, and buffer ensures each is on a separate cache line. On a 4-core system with 2 producers and 2 consumers sharing the queue, cache-line separation eliminates ~200 ns of false-sharing latency per operation.

### `DeadlockDetector`

The `has_cycle()` DFS:
```cpp
int cur = start_thread;
for (int step = 0; step < MAX_RES; ++step) {
    if (cur < 0) return false;      // followed chain to end
    if (visited[cur]) return true;  // visited twice → cycle
    visited[cur] = true;
    int res = waiting[cur];         // thread cur waits for resource res
    cur = holder[res];              // resource res is held by thread holder[res]
}
return false;
```

Maximum depth: `MAX_RES = 32` steps. For a deadlock cycle of length L, the cycle is detected after exactly L steps (when we revisit the starting node). The `MAX_RES` bound prevents infinite loops in pathological cases.

---

## Module 8: `src/engine.cpp` — Execution Engine

### Linear Memory Pool

```cpp
ExecutionEngine() {
    linear_memory.resize(8 * 1024 * 1024);  // 8 MB, zeroed
}
```

The `std::vector::resize()` call initialises all bytes to zero. This is both correct (prevents reads of uninitialised memory) and intentional — zeroed memory is a known state.

The 8 MB size is a reasonable default for embedded signal processors. Smaller systems might use 256 KB; large DSP applications might need 64 MB. The size is a compile-time decision, not a runtime parameter.

### `security_delay()`

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

Three reasons `volatile int garbage` is necessary:
1. Prevents the compiler from recognising that `garbage` is never read and eliminating the loop.
2. Forces a store to the `garbage` variable each iteration — making the loop body non-trivial.
3. `volatile` stores interact with the memory subsystem in a way that prevents the loop from being unrolled and then eliminated as dead code.

---

## Module 9: `kernel/scheduler.cpp` — Concurrency Primitive

### Spinlock Design

```cpp
void core_sync() {
    while (sync_flag.exchange(true, memory_order_acquire)) {
        __builtin_ia32_pause();
    }
    // critical section
    sync_flag.store(false, memory_order_release);
}
```

`exchange(true, acquire)` is a test-and-set: sets the flag to true and returns the previous value. If the previous value was `false` (lock was free), the thread enters the critical section. If `true` (lock was held), the thread spins.

`memory_order_acquire` on the exchange ensures that all memory reads in the critical section happen after the lock acquisition. `memory_order_release` on the store ensures all writes complete before the lock is released.

`__builtin_ia32_pause()` in the spin loop is critical for hyperthreaded CPUs. Without it, the spinning thread occupies the full execution pipeline, leaving nothing for the sibling hardware thread. With `PAUSE`, the spinning thread yields pipeline resources for ~10-40 clock cycles per PAUSE, allowing the sibling to make progress and potentially release the lock faster.

---

## How Modules Connect: A Single `say` Statement

Tracing `say "Hello";` end-to-end:

```
Source: say "Hello";
        │
        ▼ NanCompiler::generate_bytecode()
Bytecode: [02][05 00 00 00][48 65 6C 6C 6F]
          OP_SAY  len=5     "Hello"
        │
        ▼ NanRuntime::run()
Dispatch: opcode 0x02 → OP_SAY handler
Read len: 5
Read buf: [0x48, 0x65, 0x6C, 0x6C, 0x6F]
Print:    std::cout << "Hello" << "\n"
Output:   Hello
```

Total chain length from keyword to stdout: 6 functions, 0 allocations, 0 SIMD operations, 0 security-critical paths. The simplest possible instruction.

---

## Adding a New Opcode — Step-by-Step

To add a hypothetical `MUL` (multiply) opcode:

1. **`include/nan_binary.h`** — add to `NanOpcode` enum:
   ```cpp
   MUL = 0x10,   // NOTE: would expand ISA beyond 4-bit — reconsider encoding
   ```

2. **`NanCompiler::generate_bytecode()`** in `main.cpp` — add pattern detection:
   ```cpp
   if (line.find("mul ") != std::string::npos) {
       // emit MUL opcode + 2 operands
   }
   ```

3. **`NanRuntime::run()`** in `main.cpp` — add dispatch case:
   ```cpp
   case 0x10: {  // MUL
       uint8_t a, b;
       bin.read(&a, 1); bin.read(&b, 1);
       accumulator = a * b;
       break;
   }
   ```

4. **`ConstantFolder::fold()`** in `nan_compiler.h` — add folding rule:
   ```cpp
   case NanOpcode::MUL: return e.lhs * e.rhs;
   ```

5. **`CrossPlatformEmitter::emit()`** — add x86-64 and ARM64 encoding.

6. **Tests** — add a `make demo-mul` target.

7. **Documentation** — update SPECIFICATION.md §9.2 opcode table and GLOSSARY.md.

---

## Build System Internals

The Makefile uses pattern rules for compilation:
```makefile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

And a single link rule:
```makefile
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@strip --strip-debug $(TARGET) 2>/dev/null || true
```

`strip --strip-debug` removes DWARF debug info from the release binary, reducing its size by 30–50% without affecting runtime behaviour. The `2>/dev/null || true` suppresses errors on systems where `strip` is unavailable.

The `FAST_FLAGS` build uses `-fprofile-generate`, which instruments every branch and loop with counters. Running the instrumented binary writes `.gcda` files. A subsequent build with `-fprofile-use` reads the `.gcda` files and uses the real-world branch frequencies to guide optimization. The `make clean` target removes `.gcda` and `.gcno` files as part of the cleanup.

---

*INTERNALS.md — NanLang v3.0.2 | Source-level guide to all 9 modules*