# SPECIFICATION.md — NanLang Language Specification

> **NanLang v3.0.2 Specification** | Normative reference for language behaviour, bytecode format, and toolchain contracts.
> Notation: `MUST` = required, `SHOULD` = recommended, `MAY` = permitted.

---

## 1. Source File Format

- Source files MUST use the `.nl` extension.
- Encoding: UTF-8. The compiler MUST reject files with invalid UTF-8 sequences.
- Line endings: `\n` (LF) or `\r\n` (CRLF). Both MUST be accepted.
- Maximum line length: unlimited (no enforced limit).
- Maximum file size: limited only by available memory.

---

## 2. Lexical Structure

### 2.1 Comments

```
line-comment  ::= "//" (any character except newline)* newline
block-comment ::= "/*" (any character)* "*/"
```

Comments MUST be stripped before tokenisation. Block comments MUST NOT nest.

### 2.2 Keywords

The following identifiers are reserved and MUST NOT be used as user-defined names:

```
fn       let      const    return   if       else
loop     break    halt     say      pulse    emit
jitter   purge    on       interrupt
void     potential signal  flow     str      shadow
Error
```

### 2.3 Identifiers

```
identifier ::= (letter | "_") (letter | digit | "_")*
letter     ::= [a-zA-Z]
digit      ::= [0-9]
```

Identifiers are case-sensitive. Maximum length: 255 characters.

### 2.4 Integer Literals

```
decimal ::= [0-9]+
hex     ::= "0x" [0-9a-fA-F]+
binary  ::= "0b" [01]+
```

Integer literals are evaluated at compile time. Values MUST fit in `uint8_t` (0–255) when used as `potential` literals. Overflow at the literal level is a compile error.

### 2.5 Signal Literals

| Token | Meaning         | Internal Value |
|-------|-----------------|----------------|
| `^`   | High            | 0x01           |
| `_`   | Low             | 0x00           |
| `?`   | Uncertain       | 0xFF           |
| `_>_` | Falling edge    | 0x10           |
| `_>>_`| Double falling  | 0x11           |

### 2.6 String Literals

```
string-literal ::= '"' (utf8-char | escape-sequence)* '"'
escape-sequence ::= '\' ('n' | 't' | 'r' | '"' | '\' | '0')
```

String literals MUST be UTF-8. Maximum length: 2^32 − 1 bytes (4 GB).

---

## 3. Type System

### 3.1 Types

| Type        | Width     | Representation  | Notes                         |
|-------------|-----------|-----------------|-------------------------------|
| `potential` | 8 bits    | `uint8_t`       | Unsigned, wraps modulo 256    |
| `signal`    | 8 bits    | `enum class`    | Values: High, Low, Uncertain  |
| `flow`      | variable  | `vector<Signal>`| Ordered signal sequence       |
| `str`       | variable  | `ManagedString` | Volatile-wiped on destruction |
| `shadow`    | variable  | `vector<uint8_t>`| Raw byte buffer              |
| `void`      | 0 bits    | N/A             | Function return only          |

### 3.2 Type Rules

- A `potential` MUST NOT be implicitly converted to a `signal` or vice versa.
- A `signal` MUST NOT be used in arithmetic expressions.
- `void` MUST NOT appear as a variable type — only as a function return type.
- All variables MUST be initialised at declaration. Uninitialized declarations are a compile error.
- A `let` binding is immutable by default. Reassignment requires `let mut`.
- A `const` binding is evaluated at compile time and inlined at every use site.

### 3.3 Comparison Result Types

All comparison operators (`==`, `!=`, `<`, `>`) on `potential` operands MUST produce a `signal` result:
- `High` (`^`) when the comparison is true.
- `Low` (`_`) when the comparison is false.

---

## 4. Expressions

### 4.1 Arithmetic Operators

Defined only for `potential` operands. Result type is `potential`.

| Operator | Operation        | Overflow Behaviour        |
|----------|------------------|---------------------------|
| `+`      | Addition         | Wraps modulo 256 (defined)|
| `-`      | Subtraction      | Wraps modulo 256 (defined)|
| `*`      | Multiplication   | Lower 8 bits of result    |
| `/`      | Integer division | Division by zero: compile error if constant; runtime: `on Error` |

### 4.2 Signal Operators

Defined only for `signal` operands. Result type is `signal`.

| Operator | Name              | Truth Table                           |
|----------|-------------------|---------------------------------------|
| `^&^`    | Signal AND        | High only if both inputs are High     |
| `_?_`    | Uncertainty merge | Uncertain if either input is Uncertain|
| `>>>`    | Propagation       | Propagates left signal through right  |

### 4.3 Hardware Register Operator

```
@expr          — read from memory-mapped address expr
@expr = value  — write value to memory-mapped address expr
```

The address expression MUST evaluate to a `potential` (8-bit address) or a compile-time integer literal. The read operation returns a `potential`. The write operation is a statement, not an expression.

### 4.4 Operator Precedence (high to low)

```
1.  *  /
2.  +  -
3.  <  >  ==  !=
4.  ^&^  _?_  >>>
5.  =  @=
```

---

## 5. Statements

### 5.1 Variable Declaration

```
let-decl  ::= "let" ["mut"] identifier ":" type "=" expr ";"
const-decl::= "const" identifier ":" type "=" const-expr ";"
```

### 5.2 Assignment

```
assignment ::= identifier "=" expr ";"
             | "@" expr "=" expr ";"
```

Assignment to a non-`mut` binding is a compile error.

### 5.3 Say Statement

```
say-stmt ::= "say" string-literal ";"
```

Prints the string followed by `\n` to stdout. Compiles to `OP_SAY` + 4-byte length + inline bytes.

### 5.4 Pulse Statement

```
pulse-stmt ::= "pulse" expr ";"
```

Emits the signal value to the `AsyncPulseEmitter` ring buffer. Operand MUST be of type `signal`.

### 5.5 Emit Statement

```
emit-stmt ::= "emit" expr ";"
```

Enqueues a signal value to the async emitter. Operand MUST be of type `signal` or `potential`.

### 5.6 Return Statement

```
return-stmt ::= "return" [expr] ";"
```

`return` without an expression is valid only in `void` functions. `return` with an expression is required in non-`void` functions on all code paths.

### 5.7 Jitter Statement

```
jitter-stmt ::= "jitter" ";"
```

Inserts a call to `security_delay()`. Introduces 0–31 randomized `PAUSE` cycles. MUST appear as a standalone statement.

### 5.8 Purge Statement

```
purge-stmt ::= "purge" identifier ";"
```

Immediately zero-wipes the storage of a `str` variable. The variable remains in scope but its content is zeroed. The operation MUST use `volatile` writes.

### 5.9 Halt Statement

```
halt-stmt ::= "halt" ";"
```

Immediately terminates execution. In the VM, emits `OP_HALT`. In native binaries, calls `_exit(0)`. MUST NOT be used as an expression.

---

## 6. Control Flow

### 6.1 Conditional

```
if-stmt ::= "if" expr block ["else" (if-stmt | block)]
```

The condition expression MUST be of type `signal`. A `High` value executes the `if` block. A `Low` value executes the `else` block (if present). An `Uncertain` value is implementation-defined (SHOULD treat as Low).

### 6.2 Loop

```
loop-stmt ::= "loop" block
```

Executes the block repeatedly until a `break` statement is reached. NanLang has no `while`, `for`, or `do-while` constructs. All iteration MUST be expressed with `loop` + `break`.

### 6.3 Break

```
break-stmt ::= "break" ";"
```

Exits the innermost enclosing `loop`. A `break` outside a `loop` is a compile error.

---

## 7. Functions

```
fn-decl    ::= "fn" identifier "(" param-list ")" "->" return-type block
param-list ::= (param ("," param)*)?
param      ::= identifier ":" type
return-type::= type | "void"
```

- Functions MUST declare a return type.
- Non-`void` functions MUST return a value on every execution path. Missing return paths are a compile error.
- Functions MAY be called before they are declared (forward reference).
- Recursion is permitted. Stack depth is limited by the system stack size.
- There is no function overloading. Each function name MUST be unique within its scope.

### 7.1 Error Handler

```
error-handler ::= "on" "Error" block
```

MUST appear as the first statement in a function body if present. Executes if a hardware fault or runtime error occurs during the function. The handler MUST either `return` a value or `halt`.

---

## 8. Event Handlers

```
interrupt-handler ::= "on" "interrupt" "(" integer-literal ")" block
```

Registers a handler for the specified interrupt vector. The integer literal is the vector number (0–255 for x86; 16–255 for ARM Cortex-M user interrupts). Multiple handlers for the same vector MUST NOT be registered; the compiler MUST emit an error.

---

## 9. Bytecode Format

### 9.1 File Structure

```
Offset   Size   Field
0x00     4      Magic: 0x21 0x4E 0x41 0x4E  (little-endian 0x214E414E)
0x04     N      Instruction stream (variable length)
```

The magic MUST be validated before execution. An invalid magic MUST produce the error `[RT] Invalid magic.` and abort.

### 9.2 Opcode Encoding

| Opcode | Hex  | Operand Bytes | Operand Format                   |
|--------|------|---------------|----------------------------------|
| NOP    | 0x00 | 0             | none                             |
| HALT   | 0x01 | 0             | none                             |
| SAY    | 0x02 | 4 + N         | u32 len (LE), then len UTF-8 bytes|
| LOAD   | 0x03 | 1             | u8 immediate value               |
| STORE  | 0x04 | 1             | u8 address                       |
| ADD    | 0x05 | 2             | u8 operand-a, u8 operand-b       |
| SUB    | 0x06 | 2             | u8 operand-a, u8 operand-b       |
| JMP    | 0x07 | 4             | u32 target offset (LE)           |
| JZ     | 0x08 | 4             | u32 target offset (LE), jump if accumulator == 0 |
| XOR    | 0x09 | 2             | u8 operand-a, u8 operand-b       |
| OR     | 0x0A | 2             | u8 operand-a, u8 operand-b       |
| AND    | 0x0B | 2             | u8 operand-a, u8 operand-b       |
| SHL    | 0x0C | 2             | u8 value, u8 shift amount        |
| SHR    | 0x0D | 2             | u8 value, u8 shift amount        |
| CALL   | 0x0E | 4             | u32 target offset (LE)           |
| RET    | 0x0F | 0             | none                             |

All multi-byte operands are little-endian. The VM MUST NOT execute unknown opcodes; encountering an unknown opcode MUST trigger the `on Error` handler or halt.

### 9.3 VM Execution Model

1. Read 4-byte magic. Validate.
2. Read 1-byte opcode.
3. Dispatch to handler. Execute.
4. Repeat from step 2 until EOF or `HALT`.

The VM is a single-threaded, stack-less interpreter. There is no explicit call stack in the current version — `CALL` and `RET` are reserved for future use.

---

## 10. Compilation Guarantees

The compiler MUST guarantee the following properties of its output:

1. **Magic validity:** Every `.nb` file MUST begin with `0x21 0x4E 0x41 0x4E`.
2. **Opcode correctness:** Every opcode in the output MUST be in the range `0x00`–`0x0F`.
3. **SAY encoding:** The 4-byte length field in a `SAY` instruction MUST equal the number of bytes that follow it.
4. **Determinism:** Given the same source file, the compiler MUST produce the same bytecode output on every invocation (no non-deterministic passes).
5. **Optimization non-interference:** Optimization passes (NOP strip, constant fold, RLE) MUST NOT alter the observable output of a program — only its size and the number of instructions executed.

---

## 11. Memory Layout Guarantees

The runtime MUST guarantee:

1. `AsyncPulseEmitter::cache_buf` is aligned to 64 bytes.
2. `AsyncPulseEmitter::write_pos` is on a separate 64-byte cache line from `cache_buf`.
3. `WaitFreeQueue::head` and `WaitFreeQueue::tail` are on separate 64-byte cache lines.
4. `RegisterMap` fits within 2 cache lines (128 bytes for 16 × 8-byte entries).
5. The linear memory pool is contiguous and allocated before the main execution loop begins.

---

## 12. Security Guarantees

The runtime MUST guarantee:

1. **Volatile erasure:** When a `str` variable is destroyed (scope exit or explicit `purge`), all bytes of its storage MUST be overwritten with `0x00` using `volatile` writes that cannot be elided by the compiler.
2. **Jitter non-inlineability:** The `security_delay()` function MUST NOT be inlined by the compiler. It MUST execute a randomised number of `PAUSE` instructions in the range [0, 31].
3. **HW fingerprint append:** `HWFingerprint::embed()` MUST append exactly 8 bytes to the target buffer, encoding the CPU ID in little-endian byte order.

---

## 13. Error Conditions and Behaviour

| Condition                     | Required Behaviour                              |
|-------------------------------|------------------------------------------------|
| Invalid magic in `.nb` file   | Print `[RT] Invalid magic.` and abort          |
| File not found (`--compile`)  | Print `[ERROR] File not found: <path>` exit 1  |
| File not found (`--run`)      | Print `[ERROR] Bytecode not found: <path>` exit 1|
| Unknown `--demo` name         | Print `[ERROR] Unknown demo: <n>` exit 1      |
| Invalid CLI arguments         | Print `[ERROR] Invalid arguments. Use --help.` exit 1|
| Native build failure          | Print `[FAIL] Native build failed.` exit non-zero |
| Patch offset out of bounds    | Return false from `patch()`. No file written.  |
| JIT `mmap` failure            | Print `[JIT] mmap failed` return null function pointer |
| DMA `mmap` failure            | Print `[DMA] mmap failed, falling back to malloc` |
| Deadlock cycle detected       | Print `[DEADLOCK] Cycle detected! Thread N blocked on resource M` return false |
| Register overflow (`allocate`)| Return -1 (caller responsible for spill)       |

---

## 14. CLI Contract

The `nanlang` binary MUST support the following flag syntax:

```
--key value     key-value pair (value is next argv token if it does not start with '-')
--flag          boolean flag (no value)
```

All flags are optional unless stated. Unrecognised flags MUST cause exit with error code 1.

Required flags per mode:

| Mode           | Required              | Optional                |
|----------------|-----------------------|-------------------------|
| `--compile`    | `--compile <file.nl>` | `--output <file.nb>`    |
| `--run`        | `--run <file.nb>`     | none                    |
| `--build-native`| `--build-native <f>` | `--output <n>`, `--opt` |
| `--patch`      | `--patch <f>`, `--offset <hex>` | `--bytes "<hex bytes>"` |
| `--demo`       | `--demo <n>`          | all demo parameters     |

---

## 15. Version String

`./nanlang --version` MUST print exactly:

```
NanLang v3.0.2
```

followed by a single newline character (`\n`). No additional output.

---

## 16. Reserved for Future Use

The following features are defined in the specification but not fully implemented in v3.0.2:

- `CALL` and `RET` opcodes (bytecode encoding defined; VM execution reserved)
- `SHL` and `SHR` opcodes (encoding defined; compiler emission reserved)
- Interrupt handler registration (syntax defined; linking to vector table reserved)
- `flow` type arithmetic operators (type defined; operators reserved)
- Cross-module symbol resolution in NanLinker (API defined; address patching reserved)

Implementations MUST NOT rely on unimplemented features producing correct results.

---

*SPECIFICATION.md — NanLang v3.0.2 | Normative reference*