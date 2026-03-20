# GLOSSARY.md — NanLang Terms and Definitions

> NanLang v3.0.2 | All terms defined in the context of the NanLang language, toolchain, and execution environment.
> Terms are sorted alphabetically within each section.

---

## A

**Accumulator**
The implicit destination register for arithmetic and comparison operations in the NanLang VM. Not directly addressable in source; updated by `LOAD`, `ADD`, `SUB`, `XOR`, etc.

**AddressSanitizer (ASan)**
A compiler-instrumented runtime memory error detector. Enabled in the `make debug` build. Catches out-of-bounds accesses, use-after-free, and memory leaks with minimal false positive rate.

**Alignment**
The requirement that a data structure's start address be a multiple of a specific value. NanLang uses `alignas(64)` for cache-line alignment and `alignas(32)` for AVX2 SIMD buffers. Misaligned SIMD access is legal but may incur a performance penalty on older CPUs.

**ARM64 / AArch64**
The 64-bit ARM instruction set architecture. NanLang's `CrossPlatformEmitter` can emit ARM64 machine code from NanLang IR. ARM64 uses fixed-width 32-bit instructions in little-endian byte order.

**AsyncPulseEmitter**
A lock-free, cache-aligned ring buffer that accepts `emit()` and `pulse` signal writes without blocking. Backed by a 1024-byte (`CACHE_LINE * 16`) array. Write position is an `std::atomic<size_t>` updated with `memory_order_relaxed`.

**AVX2**
Advanced Vector Extensions 2. An x86-64 SIMD extension operating on 256-bit (32-byte) `__m256i` registers. Enables `SIMDProcessor::xor32()` and `or32()` to process 32 bytes in a single instruction. Requires Haswell (Intel, 2013) or Ryzen (AMD, 2017) or newer.

---

## B

**Baseline (SizeMinimizer)**
The smallest compressed bytecode size seen in any previous compilation run. `SizeMinimizer::minimize()` warns if a new run does not produce output smaller than or equal to the baseline.

**Binary Image**
The complete in-memory copy of a binary file, loaded as a `std::vector<uint8_t>` by `BinaryPatcher`. Modifications are applied in memory; the original file is never written.

**BinaryPatcher**
A struct in `nan_binary.h` that reads a binary file into memory, applies byte-level patches at specified offsets or by search-and-replace, and writes the result to a new file with the `.patched` suffix.

**Block**
A sequence of statements enclosed in `{ }`. Defines a scope. All `let` bindings declared in a block are destroyed when the block closes. Destructor-based cleanup (e.g., `str` zeroing) runs at block exit.

**BranchPredictor**
A 256-entry table of 2-bit saturating counters in `nan_perf.h`. Each counter tracks the recent history of one branch (identified by an 8-bit PC hash). Prediction: counter ≥ 2 → Taken; < 2 → Not-Taken. Used in `make demo-branch`.

**Branch Resolution (Static)**
Compile-time elimination of conditional branches whose conditions are compile-time constants. `BranchResolver::resolve()` replaces `JZ`/`JMP` instructions with unconditional `JMP` to the statically chosen target.

**Bytecode**
The binary encoding of a NanLang program, stored in `.nb` files. Begins with magic `0x214E414E`. Consists of a sequence of opcodes and their operands, interpreted by `NanRuntime`.

---

## C

**`cache_buf`**
The 1024-byte `alignas(64)` ring buffer inside `AsyncPulseEmitter`. Holds the last N signal bytes written before the next `flush()`.

**Cache Line**
The smallest unit transferred between CPU cache levels. On x86-64, 64 bytes. NanLang places frequently accessed hot variables on separate cache lines using `alignas(64)` to prevent false sharing.

**CacheAligned\<T\>**
A template wrapper in `nan_arch.h` that places `T` on a 64-byte boundary using `alignas(64)`. Prevents false sharing when multiple threads access different fields of the same object.

**Calibrate**
The `ClockCycleSync::calibrate()` operation. Reads the CPU's Time Stamp Counter (via `RDTSCP`) and stores the value as a baseline for subsequent `elapsed()` calls.

**CALL (opcode 0x0E)**
Reserved opcode for subroutine calls. Encoding defined in Nan-ISA; VM execution reserved for a future version.

**ClockCycleSync**
A struct in `nan_pulse.h` wrapping `RDTSCP`. Provides `calibrate()`, `elapsed()`, and the static `wait_cycles()` for sub-microsecond spin-wait delays.

**Compile-Time Constant**
A value fully known to the compiler before the program runs. Used in `const` declarations and as operands to `ConstantFolder`. Examples: integer literals, signal literals, and arithmetic on other constants.

**ConstantFolder**
A struct in `nan_compiler.h` implementing compile-time evaluation of bytecode arithmetic. Scans for `[ADD|SUB|XOR] [imm1] [imm2]` triples and replaces them with `[LOAD] [result]`, saving 1 byte per folded instruction.

**CPUID**
An x86 instruction (`CPUID`, leaf 1) that returns processor identification data in registers EAX–EDX. `HWFingerprint::get_cpu_id()` uses the EBX and EDX values as a 64-bit hardware-bound identifier. CPUID is serializing — it flushes the instruction pipeline.

**CrossPlatformEmitter**
A struct in `nan_compiler.h` that translates `IRInstruction` vectors to raw machine bytes for either `Arch::X86_64` or `Arch::ARM64`. Used by `make demo-xplatform`.

---

## D

**Dead Store**
A memory write whose value is never subsequently read before the storage is reclaimed. Compilers typically eliminate dead stores as an optimization. `volatile` prevents this elimination, which is why `ManagedString`'s destructor uses `volatile char*` for its zero-wipe.

**DeadlockDetector**
A struct in `nan_perf.h` that maintains a resource allocation graph and runs a DFS cycle detection on every `try_acquire()` call. If a cycle is detected, the acquire is rejected and the deadlock is prevented from forming.

**DFS (Depth-First Search)**
The algorithm used by `DeadlockDetector::has_cycle()` to detect cycles in the wait-for graph. O(V) where V = `MAX_RES` = 32. Follows the chain: thread → waiting-for resource → held-by thread → ...

**DMA (Direct Memory Access)**
A technique for transferring data between memory regions without routing through CPU registers. `DMAPulse` simulates DMA using `mmap(MAP_LOCKED)` and AVX2 non-temporal stores (`_mm256_stream_si256`).

**DMAPulse**
A struct in `nan_arch.h` that allocates locked memory pages and performs non-temporal bulk transfers. Falls back to `malloc` on non-Linux platforms.

---

## E

**ELF (Executable and Linkable Format)**
The binary format for Linux executables. `write_minimal_elf()` in `nan_binary.h` generates a 64-byte ELF header + 56-byte program header (LOAD segment) + raw machine code. The smallest valid Linux executable.

**EmbeddedResource**
A named binary blob appended to a binary image by `embed_resources()`. Stored in a catalog format: `[count:4][name_len:2][name][data_len:4][data]...`.

**`emit` (keyword)**
A NanLang statement that enqueues a signal or potential value to the `AsyncPulseEmitter`. Non-blocking; returns before the signal is consumed.

**Entry Point**
The virtual address at which execution begins in a native ELF binary. Set to `0x400000 + 64 + entry_offset` by `write_minimal_elf()`, placing the first instruction immediately after the ELF + program headers.

**ExecutionEngine**
The class in `src/engine.cpp` that manages the VM's register map and linear memory pool. Pre-allocates 8 MB at construction. Provides `update_state()` and `security_delay()`.

---

## F

**False Sharing**
A performance degradation caused when two threads access different variables that happen to share the same cache line. Modifying either variable invalidates the entire cache line on the other CPU core. NanLang prevents false sharing by placing `head` and `tail` in `WaitFreeQueue` on separate 64-byte-aligned cache lines.

**Fingerprint (Hardware)**
An 8-byte identifier derived from the CPU's CPUID output, embedded at the end of a binary by `HWFingerprint::embed()`. Used to bind a binary to a specific processor and detect unauthorized redistribution.

**`flow` (type)**
An ordered sequence of `signal` values. Maps to `std::vector<Signal>`. Used to model parallel bus transactions, digital waveforms, and multi-bit data streams.

**`flush()` (AsyncPulseEmitter)**
Resets the emitter's write position to 0 and zeroes `cache_buf`. Called after a transmission is complete to prepare for the next burst.

**Folding**
See *Constant Folding*.

**FreqScaler**
A struct in `nan_perf.h` that emits x86 hints (`PAUSE`, `SFENCE`, `LFENCE`) to guide the CPU's dynamic frequency scaling. `LIGHT` load: multiple `PAUSE`s. `MEDIUM`: `SFENCE`. `HEAVY`: `LFENCE` for maximum frequency.

---

## G

**Greedy Coloring**
The register allocation algorithm used by `RegisterColoring`. Assigns each variable the lowest-numbered register not used by any of its conflicting neighbors. O(V + E). May fail (return `false`) if K = 16 registers are insufficient for the conflict graph.

---

## H

**`halt` (keyword)**
Immediately terminates execution. Maps to `OP_HALT` (0x01) in bytecode, to `_exit(0)` in native binaries. Used when continuing execution would be more dangerous than stopping.

**Hamming(7,4)**
An error-correcting code that encodes 4 data bits into 7 bits by adding 3 parity bits. Implemented by `hamming_encode()` and `hamming_decode_correct()` in `nan_binary.h`. Detects all 2-bit errors, corrects any 1-bit error. Encoded form is 75% larger than the original.

**Hardware Fingerprinting**
See *Fingerprint (Hardware)*.

**`halt` (opcode 0x01)**
See `OP_HALT`.

**`HugePageAlloc`**
A struct in `nan_arch.h` that allocates 2 MB transparent huge pages via `mmap(MAP_HUGETLB)`. Reduces TLB pressure for large buffers. Falls back to regular `mmap` if huge pages are unavailable.

**`hw_key_xor()`**
A function in `nan_binary.h` that XORs each byte of a vector with a byte from an 8-byte CPU-ID key (`key[i % 8]`). Symmetric: applying twice with the same key recovers the original. Used for binary obfuscation, not cryptographic security.

**`HWFingerprint`**
A struct in `nan_compiler.h` providing `get_cpu_id()`, `embed()`, and `validate()`. Used in `make demo-hwfp`.

---

## I

**IR (Intermediate Representation)**
The `CrossPlatformEmitter::IRInstruction` struct: a pair of `{uint8_t op, int64_t operand}`. Abstracts over target architecture encoding. The same IR vector produces different machine bytes for x86-64 vs ARM64.

---

## J

**`jitter` (keyword)**
A statement that inserts a call to `security_delay()`. Randomises execution timing to defeat timing side-channel attacks. The delay is 0–31 `PAUSE` instruction cycles, non-deterministically chosen via `rand() % 32`.

**JIT (Just-In-Time Compilation)**
Generating and executing native machine code at runtime. `JITPulseGen::compile()` allocates a RWX page, writes machine bytes, applies W^X via `mprotect`, and returns a callable function pointer. Linux-only in v3.0.2.

**`JITPulseGen`**
A struct in `nan_compiler.h` providing `make_ret_stub()`, `make_zero_func()`, and `compile()`. Used in `make demo-jit`.

---

## L

**Linear Memory Pool**
The 8 MB `std::vector<uint8_t>` pre-allocated by `ExecutionEngine` at construction. All runtime data is drawn from this pool; no `malloc` calls occur during signal processing. Eliminates heap fragmentation and timing non-determinism.

**Lock-Free**
A concurrency guarantee stronger than blocking: at least one thread always makes progress. `WaitFreeQueue` is also wait-free (stronger still: every thread makes progress in bounded steps).

**`LOAD` (opcode 0x03)**
Loads an 8-bit immediate value into the accumulator.

---

## M

**Magic Number**
The 4-byte sequence `0x21 0x4E 0x41 0x4E` (ASCII `!NAN`) at the start of every `.nb` bytecode file. Validated by `NanRuntime::run()` before execution.

**`ManagedString`**
The C++ backing of NanLang's `str` type. Stores text in a `std::string`. Destructor performs a `volatile` zero-wipe of all bytes. The `active` flag indicates whether the string has been purged.

**`MAP_HUGETLB`**
A Linux `mmap` flag requesting 2 MB transparent huge pages. Used by `HugePageAlloc`. Requires pre-allocated huge pages (`/proc/sys/vm/nr_hugepages`). Falls back silently if unavailable.

**`MAP_LOCKED`**
A Linux `mmap` flag requesting that the mapped pages be pinned in physical RAM (not swapped out). Used by `DMAPulse::map_region()`. Requires `CAP_IPC_LOCK` or sufficient `RLIMIT_MEMLOCK`.

**`memory_order_acquire`**
A C++ `std::atomic` memory ordering. Ensures that no memory operations in the current thread can be reordered before the load. Used in `WaitFreeQueue::pop()` to ensure the consumer sees all stores made before the producer's `memory_order_release` store.

**`memory_order_release`**
A C++ `std::atomic` memory ordering. Ensures that no memory operations in the current thread can be reordered after the store. Used in `WaitFreeQueue::push()` and `AsyncPulseEmitter::flush()`.

**`memory_order_relaxed`**
A C++ `std::atomic` memory ordering. Provides atomicity but no ordering guarantees. Used in `AsyncPulseEmitter::emit()` for the write position counter — ordering is provided by the subsequent `_mm_sfence()`.

**MiniContext**
A stub struct in `nan_perf.h` representing a minimal fiber context (RSP, RBP, RBX, R12–R15, RIP). `save()` and `restore()` are placeholder implementations; production fiber switching requires inline assembly.

**`mprotect`**
A Linux system call that changes the protection flags of a memory region. Used by `JITPulseGen::compile()` to transition a page from RWX (write + execute) to RX (read + execute) after JIT code is written, enforcing the W^X security policy.

---

## N

**Nan-ISA**
The NanLang Instruction Set Architecture. A 16-opcode bytecode encoding for the NanLang VM. Opcodes 0x00–0x0F. All opcodes fit in one nibble.

**NanCompiler**
The class in `main.cpp` implementing `generate_bytecode()` (source → `.nb`) and `build_native()` (source → ELF via g++).

**NanLinker**
A struct in `nan_compiler.h` that concatenates multiple bytecode segments, optionally stripping NOP bytes. Used in `make demo-linker`.

**NanRuntime**
The class in `main.cpp` implementing `run()` — the bytecode VM's dispatch loop. Validates the magic header and dispatches on each opcode byte.

**`nan_temp__.cpp`**
The temporary C++ file generated by `NanCompiler::build_native()`. Contains a `main()` function with `std::cout` statements corresponding to each `say` in the NanLang source. Deleted after successful compilation.

**`.nb` (file extension)**
NanLang Bytecode. The output of `--compile`. Consumed by `--run`. Format defined in §9 of SPECIFICATION.md.

**`.nl` (file extension)**
NanLang Source. The human-readable source language format consumed by `--compile` and `--build-native`.

**NOP (opcode 0x00)**
No-operation. Produces no effect. Used for segment alignment padding by the linker. Stripped by `strip_nops()` and `NanLinker::link()`.

**Non-Temporal Store**
An AVX2 store instruction (`_mm256_stream_si256`) that writes data directly to DRAM without loading the target cache line first. Avoids polluting L1/L2/L3 for large sequential writes. Requires `_mm_sfence()` to ensure visibility.

---

## O

**Obfuscation**
See `hw_key_xor()`. Not cryptographic encryption — XOR with a predictable key. Prevents casual static disassembly; does not prevent a determined attacker with the target hardware.

**`on Error`**
An inline error handler block at the start of a function. Executes when a hardware fault or runtime exception occurs. MUST either `return` a safe value or `halt`.

**`on interrupt`**
A top-level interrupt handler registration. Binds a NanLang block to a hardware interrupt vector number. Execution is deferred until the CPU delivers the interrupt.

**`OP_HALT` (0x01)**
The VM opcode for unconditional program termination. Emitted by the `halt` keyword.

**`OP_SAY` (0x02)**
The VM opcode for string output. Followed by a 4-byte length and the UTF-8 string bytes. Prints to stdout with a trailing newline.

**Out-of-Order (OoO)**
CPU execution where instructions execute in a different order than programmed, based on operand availability. `ReservationStation` in `nan_perf.h` simulates a Tomasulo-style OoO unit with 8 entries.

---

## P

**`PAUSE` instruction**
The x86 `PAUSE` instruction (`__builtin_ia32_pause()`). Used in spinloops to: (1) hint to the CPU that the thread is spinning, reducing power consumption; (2) improve performance on hyperthreaded cores by freeing execution resources for the sibling thread; (3) prevent memory order violations on older CPUs.

**PGO (Profile-Guided Optimization)**
A compiler optimization technique using actual runtime data to improve code generation. Enabled by `make fast` (`-fprofile-generate`). Improves branch prediction, inlining decisions, and loop unrolling. Typically yields 10–20% speedup.

**PipelineReorder**
A struct in `nan_perf.h` implementing a forward-pass instruction scheduler that hoists instructions whose source registers are ready ahead of instructions waiting on data dependencies.

**`potential` (type)**
NanLang's 8-bit unsigned integer type. Models electrical potential (voltage level quantized to 256 steps). Maps to `uint8_t`. Arithmetic wraps modulo 256.

**`prefetch_block()`**
A function in `nan_arch.h` that issues `__builtin_prefetch()` calls for three consecutive cache lines ahead of the given pointer, using decreasing temporal locality hints (3, 2, 1). Reduces L1 miss penalties for sequential access patterns.

**`pulse` (keyword)**
A NanLang statement that emits a signal value to the `AsyncPulseEmitter`. The operand MUST be of type `signal`. Calls `AsyncPulseEmitter::emit()` in the runtime.

**`purge` (keyword)**
An explicit `str` memory erasure statement. Calls the equivalent of `ManagedString`'s destructor wipe immediately, before the variable goes out of scope.

---

## R

**RDTSC / RDTSCP**
x86 instructions that read the CPU's Time Stamp Counter (cycle count since last reset). `RDTSCP` is the serializing variant — it prevents out-of-order execution from repositioning the read. Used by `ClockCycleSync::calibrate()` and `elapsed()`.

**RegisterColoring**
A struct in `nan_compiler.h` implementing greedy graph coloring for register allocation. K = 16 available registers (R0–R15). Returns `false` (spill needed) when the graph cannot be colored with K colors.

**`RegisterMap`**
A struct in `nan_arch.h` managing a 16-entry `int64_t` array (R0–R15). `allocate()` finds the first free register; `free_reg()` zeroes and marks it available. Returns -1 on overflow.

**ReservationStation**
A struct in `nan_perf.h` with 8 `Entry` slots. Implements `issue()` (add an instruction to a free slot) and `execute_ready()` (run all slots whose operands `qj` and `qk` are both -1, i.e., ready).

**RET (opcode 0x0F)**
Returns from a subroutine. In JIT-compiled stubs, emits the x86 `RET` byte (0xC3) or ARM64 `RET` encoding.

**RLE (Run-Length Encoding)**
A lossless compression algorithm that replaces runs of identical bytes with `[count, value]` pairs. Implemented by `rle_compress()` and `rle_decompress()` in `nan_binary.h`. Max run length: 255.

---

## S

**SAY (opcode 0x02)**
See `OP_SAY`.

**`say` (keyword)**
A NanLang I/O statement that prints a string literal to stdout followed by a newline. Compiles to `OP_SAY` + 4-byte length + UTF-8 bytes.

**`security_delay()`**
A function in `src/engine.cpp` that executes 0–31 `PAUSE` iterations using a randomised loop bound (`rand() % 32`). The `volatile int garbage` variable prevents the compiler from optimising the loop away. Called by the `jitter` keyword.

**`shadow` (type)**
NanLang's raw byte buffer type. Maps to `std::vector<uint8_t>`. No semantic interpretation — used for encrypted data, firmware images, compressed payloads, and anything else that is "just bytes."

**`signal` (type)**
NanLang's binary logic state type. Maps to `enum class Signal : uint8_t`. Three values: `High` (^, 0x01), `Low` (_, 0x00), `Uncertain` (?, 0xFF).

**`SignalBuffer`**
A 512-byte `alignas(64)` FIFO ring buffer in `nan_pulse.h`. `push()` and `pop()` use modular index arithmetic. Head and tail are plain `size_t`s (no atomics — single-threaded use).

**`SignalPacket`**
A struct in `nan_core.h` wrapping a `__m256i` (256-bit AVX2 register). The `merge()` method performs `_mm256_or_si256` — a hardware-accelerated bitwise OR of 256 bits in a single instruction.

**`SIMDProcessor`**
A struct in `nan_arch.h` with static methods `xor32()`, `or32()`, and `count_active()`. Uses AVX2 intrinsics with scalar fallbacks.

**SizeMinimizer**
A struct in `nan_compiler.h` that chains `strip_nops()`, `ConstantFolder::fold_bytecode()`, and `rle_compress()` into a single minimisation pass. Tracks a `baseline` and warns if output size does not decrease.

**Spill**
Moving a variable from a CPU register to the linear memory pool because no register is available. `RegisterMap::allocate()` returns -1 to signal a spill is needed. Spilled variables cost 2 extra memory accesses (load + store) per use.

**`str` (type)**
NanLang's security-managed string type. Backed by `ManagedString`. UTF-8 content. Destructor performs a `volatile` zero-wipe of all bytes. Explicit wipe available via the `purge` keyword.

**`strip_nops()`**
A function in `nan_binary.h` that removes all `0x00` (NOP) bytes from a bytecode vector. Used by `NanLinker` during segment concatenation.

**Syndrome (Hamming)**
A 3-bit value computed during Hamming decoding by XORing the received parity bits against recomputed parities. A non-zero syndrome indicates an error; its binary value identifies the bit position to flip.

---

## T

**`TaskScheduler`**
A class in `kernel/scheduler.cpp` managing a thread pool and a `std::atomic<bool>` spinlock (`sync_flag`). `core_sync()` acquires the lock, executes the critical section, and releases. Uses `PAUSE` in the spin loop.

**Timing Attack**
An attack that infers secret values by measuring how long a program takes to process different inputs. Defended against using constant-time comparison, `jitter` for randomized delays, and SIMD parallelism that processes all bytes simultaneously regardless of content.

**TLB (Translation Lookaside Buffer)**
A CPU cache for virtual-to-physical address translations. TLB misses are expensive (~100 ns). `HugePageAlloc` reduces TLB pressure by using 2 MB pages instead of 4 KB pages — 512× fewer TLB entries for the same memory footprint.

**Tomasulo's Algorithm**
A hardware algorithm for dynamic out-of-order instruction scheduling. `ReservationStation` in `nan_perf.h` is a software simulation of the key concepts: operand readiness tracking, parallel execution of independent instructions, and a result bus for forwarding computed values.

**TSC (Time Stamp Counter)**
An x86 64-bit counter incremented once per CPU clock cycle since the last reset. Read by `RDTSC` (non-serializing) and `RDTSCP` (serializing). Used by `ClockCycleSync`.

---

## U

**UBSanitizer (UBSan)**
A compiler-instrumented runtime checker for undefined behavior. Enabled in `make debug`. Detects signed integer overflow, misaligned access, null pointer dereference, and more.

---

## V

**`Value` (variant)**
The C++ `std::variant<Potential, Signal, std::vector<Signal>, ManagedString>` alias in `nan_core.h`. Allows the `ExecutionEngine`'s register map to hold any NanLang value type.

**`volatile`**
A C/C++ type qualifier indicating that a memory access has observable side effects that the compiler MUST NOT optimize away. Used in `ManagedString`'s destructor (`volatile char*`) to prevent dead-store elimination of the zero-wipe.

---

## W

**Wait-Free**
The strongest concurrency guarantee: every thread makes progress in a bounded number of steps, regardless of other threads' behaviour. `WaitFreeQueue` achieves this for SPSC use. Contrast with *lock-free* (some thread always progresses, but individual threads may starve) and *blocking* (threads may wait indefinitely).

**`WaitFreeQueue<T, N>`**
A template SPSC lock-free ring queue in `nan_perf.h`. N must be a power of two (checked by `static_assert`). `push()` and `pop()` use `memory_order_acquire`/`memory_order_release` atomics on separate cache-aligned head and tail counters.

**W^X (Write XOR Execute)**
A memory protection policy enforced by modern operating systems: a page cannot be simultaneously writable and executable. `JITPulseGen::compile()` respects this by writing code under RWX, then calling `mprotect` to downgrade to RX before execution.

---

## X

**x86-64**
The 64-bit extension of the x86 instruction set, used by all modern Intel and AMD desktop/server CPUs. `CrossPlatformEmitter` emits x86-64 machine code for `LOAD` (B8 imm32), `ADD` (05 imm32), and `RET` (C3).

**XOR**
Exclusive OR. Used in: (1) `SIMDProcessor::xor32()` for 32-byte SIMD XOR; (2) `hw_key_xor()` for bytecode obfuscation; (3) `JITPulseGen::make_zero_func()` which emits `XOR EAX, EAX; RET` (bytes 31 C0 C3).

---

## Z

**Zero-Copy**
A data processing approach that avoids copying data by working on the original buffer via a pointer/length pair. `ZeroCopySlice` in `nan_arch.h` is the NanLang implementation. `sub()` creates a sub-slice by advancing the pointer — no allocation, no copy.

**`ZeroCopySlice`**
A struct in `nan_arch.h` holding `const uint8_t* ptr` and `size_t len`. Constructed from a `std::vector<uint8_t>` without copying. `sub(offset, count)` returns a new slice pointing into the same backing buffer. `valid()` checks ptr != null && len > 0.

---

*GLOSSARY.md — NanLang v3.0.2 | ~90 terms across 26 letters*