# NanLang Issue Submission Guide

Before opening an issue, remember: **NanLang operates at the hardware level.** Vague bug reports will be closed immediately. We need raw data.

## 🔍 Required Information Checklist
Please include the following in your report:

1. **Architecture Details:** Output of `lscpu` or at least your CPU model (e.g., i5-10th Gen).
2. **Bytecode Hex Dump:** If the crash happens during execution, provide the `hexdump -C output.nb` of the failing program.
3. **Compiler Output:** The full output of `make` and any warnings/errors from `g++`.
4. **The Pulse Pattern:** The `.nl` source code that triggered the issue.

## 🚫 What NOT to Report
* Issues related to non-AVX2 supported CPUs (NanLang v3.0.2+ requires AVX2).
* Issues caused by manual binary patching of the `nanlang` executable.
* Requests for "higher-level" features (use Python for that).

## 🛠️ How to Report
1. Use a clear title: `[Core/JIT/SIMD] Short description of the bug`.
2. Describe the **Expected Potential** vs **Actual Pulse**.
3. Attach the stack trace if a `SIGSEGV` occurred.

---
*If you can't provide a hex dump, you probably shouldn't be using NanLang yet.*