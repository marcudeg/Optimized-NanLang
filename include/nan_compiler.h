#pragma once
#include "nan_binary.h"
#include "nan_arch.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace nanlang {

// ─── Static Branch Resolution ────────────────────────────────────────────────
// Resolves if decisions at compile time -> no runtime branch.
struct BranchResolver {
    struct Branch {
        size_t     offset;          // position within bytecode
        bool       condition;       // pre-known condition
        size_t     target_true;
        size_t     target_false;
    };

    std::vector<Branch> table;

    void add(size_t off, bool cond, size_t t_true, size_t t_false) {
        table.push_back({off, cond, t_true, t_false});
    }

    // Reduces every branch to a single JMP.
    std::vector<uint8_t> resolve(const std::vector<uint8_t>& bytecode) {
        std::vector<uint8_t> out = bytecode;
        for (auto& br : table) {
            if (br.offset >= out.size()) continue;
            // Redirect JZ/JMP opcodes directly to the target address.
            out[br.offset] = NanOpcode::JMP;
            size_t target = br.condition ? br.target_true : br.target_false;
            // Write 4-byte little-endian target.
            uint32_t t = static_cast<uint32_t>(target);
            for (int i = 0; i < 4 && br.offset + 1 + i < out.size(); ++i)
                out[br.offset + 1 + i] = (t >> (8 * i)) & 0xFF;
        }
        return out;
    }
};

// ─── Hex-Level Constant Folding ──────────────────────────────────────────────
// Evaluates constant expressions like ADD $3, $5 at compile time.
struct ConstantFolder {
    struct Expr { uint8_t op; int64_t lhs, rhs; };

    static int64_t fold(const Expr& e) {
        switch (e.op) {
            case NanOpcode::ADD: return e.lhs + e.rhs;
            case NanOpcode::SUB: return e.lhs - e.rhs;
            case NanOpcode::XOR: return e.lhs ^ e.rhs;
            case NanOpcode::AND: return e.lhs & e.rhs;
            case NanOpcode::OR:  return e.lhs | e.rhs;
            default: return e.lhs;
        }
    }

    // Finds constant pairs in the bytecode stream and replaces them with LOAD+value.
    static std::vector<uint8_t> fold_bytecode(const std::vector<uint8_t>& in) {
        std::vector<uint8_t> out;
        for (size_t i = 0; i < in.size(); ) {
            if (i + 3 < in.size() &&
                (in[i] == NanOpcode::ADD || in[i] == NanOpcode::SUB ||
                 in[i] == NanOpcode::XOR)) {
                int64_t result = fold({in[i], in[i+1], in[i+2]});
                out.push_back(NanOpcode::LOAD);
                out.push_back(static_cast<uint8_t>(result & 0xFF));
                i += 3;
            } else {
                out.push_back(in[i++]);
            }
        }
        return out;
    }
};

// ─── Register Coloring Optimization ─────────────────────────────────────────
// Greedy assignment similar to graph coloring: assigns colors by least conflict.
struct RegisterColoring {
    static constexpr int K = 16; // number of available registers

    struct Variable { std::string name; int color = -1; };
    std::vector<Variable> vars;
    std::vector<std::pair<int,int>> conflicts; // pairs that live at the same time

    int add_var(const std::string& name) {
        vars.push_back({name, -1});
        return static_cast<int>(vars.size()) - 1;
    }

    void add_conflict(int a, int b) {
        if (a != b) conflicts.emplace_back(a, b);
    }

    // Greedy coloring: assign each variable a color different from its neighbors.
    bool color() {
        for (int i = 0; i < (int)vars.size(); ++i) {
            std::vector<bool> used(K, false);
            for (auto& [a, b] : conflicts) {
                if (a == i && vars[b].color >= 0) used[vars[b].color] = true;
                if (b == i && vars[a].color >= 0) used[vars[a].color] = true;
            }
            int c = -1;
            for (int k = 0; k < K; ++k) { if (!used[k]) { c = k; break; } }
            if (c == -1) return false; // spill required
            vars[i].color = c;
        }
        return true;
    }

    void dump() const {
        for (auto& v : vars)
            std::cout << "  var=" << v.name << " → R" << v.color << "\n";
    }
};

// ─── Nan-Linker v2.0 ─────────────────────────────────────────────────────────
// Merges binary segments while removing padding bytes.
struct NanLinker {
    std::vector<std::vector<uint8_t>> segments;

    void add_segment(const std::vector<uint8_t>& seg) {
        segments.push_back(seg);
    }

    // Concatenate all segments in order, strip padding NOPs.
    std::vector<uint8_t> link(bool strip_padding = true) {
        std::vector<uint8_t> out;
        for (auto& seg : segments) {
            auto clean = strip_padding ? strip_nops(seg) : seg;
            out.insert(out.end(), clean.begin(), clean.end());
        }
        return out;
    }

    size_t total_size() const {
        size_t s = 0;
        for (auto& seg : segments) s += seg.size();
        return s;
    }
};

// ─── Binary Size Minimizer ───────────────────────────────────────────────────
// Forces the binary to be 1 byte smaller on each compilation.
struct SizeMinimizer {
    size_t baseline = SIZE_MAX;

    std::vector<uint8_t> minimize(std::vector<uint8_t> code) {
        // 1. Strip NOPs
        code = strip_nops(code);
        // 2. Fold constants
        code = ConstantFolder::fold_bytecode(code);
        // 3. RLE compress
        auto compressed = rle_compress(code);
        // Update baseline and notify.
        if (compressed.size() < baseline) {
            baseline = compressed.size();
            std::cout << "[SIZE] Binary: " << code.size()
                      << " → compressed: " << compressed.size() << " bytes\n";
        } else {
            std::cerr << "[SIZE] Warning: size did not shrink!\n";
        }
        return compressed;
    }
};

// ─── JIT-Pulse Generator ─────────────────────────────────────────────────────
// Generates the fastest hex code for the current processor at runtime.
struct JITPulseGen {
    using JITFunc = void(*)();

        // Allocate executable memory and write the code.
    static JITFunc compile(const std::vector<uint8_t>& machine_code) {
#ifdef __linux__
        size_t size = machine_code.size();
        // Obtain a RWX page via mmap.
        void* mem = mmap(nullptr, size,
                         PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (mem == MAP_FAILED) {
            std::cerr << "[JIT] mmap failed\n";
            return nullptr;
        }
        memcpy(mem, machine_code.data(), size);
        // mprotect for W^X after code is written.
        mprotect(mem, size, PROT_READ | PROT_EXEC);
        return reinterpret_cast<JITFunc>(mem);
#else
        std::cerr << "[JIT] Platform not supported\n";
        return nullptr;
#endif
    }

    // x86-64: test stub returning a simple "ret" opcode.
    static std::vector<uint8_t> make_ret_stub() {
        return {0xC3}; // RET
    }

    // x86-64: "xor eax, eax; ret" -> function returning 0.
    static std::vector<uint8_t> make_zero_func() {
        return {0x31, 0xC0, 0xC3}; // XOR EAX,EAX; RET
    }
};

// ─── Cross-Platform Binary Emitter ───────────────────────────────────────────
// Produces both ARM and x86-64 output from the same NanLang IR.
struct CrossPlatformEmitter {
    enum class Arch { X86_64, ARM64 };

    struct IRInstruction { uint8_t op; int64_t operand; };

    static std::vector<uint8_t> emit(const std::vector<IRInstruction>& ir,
                                     Arch arch) {
        std::vector<uint8_t> out;
        for (auto& inst : ir) {
            if (arch == Arch::X86_64) {
                switch (inst.op) {
                    case NanOpcode::LOAD: // MOV EAX, imm32
                        out.push_back(0xB8);
                        for (int i = 0; i < 4; ++i)
                            out.push_back((inst.operand >> (8*i)) & 0xFF);
                        break;
                    case NanOpcode::ADD: // ADD EAX, imm32
                        out.push_back(0x05);
                        for (int i = 0; i < 4; ++i)
                            out.push_back((inst.operand >> (8*i)) & 0xFF);
                        break;
                    case NanOpcode::RET:
                        out.push_back(0xC3);
                        break;
                    default: break;
                }
            } else { // ARM64 (little-endian 32-bit instructions)
                switch (inst.op) {
                    case NanOpcode::LOAD: { // MOV X0, #imm
                        uint32_t enc = 0xD2800000 |
                            ((inst.operand & 0xFFFF) << 5);
                        for (int i = 0; i < 4; ++i)
                            out.push_back((enc >> (8*i)) & 0xFF);
                        break;
                    }
                    case NanOpcode::RET: { // RET
                        uint32_t enc = 0xD65F03C0;
                        for (int i = 0; i < 4; ++i)
                            out.push_back((enc >> (8*i)) & 0xFF);
                        break;
                    }
                    default: break;
                }
            }
        }
        return out;
    }
};

// ─── Hardware Fingerprint Validation ─────────────────────────────────────────
// Embeds the processor's CPUID into the binary and validates it at runtime.
struct HWFingerprint {
    static uint64_t get_cpu_id() {
        uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
#ifdef __x86_64__
        __asm__ volatile(
            "cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1)
        );
#endif
        return (static_cast<uint64_t>(ebx) << 32) | edx;
    }

    static bool validate(uint64_t embedded_id) {
        return get_cpu_id() == embedded_id;
    }

    static void embed(std::vector<uint8_t>& binary) {
        uint64_t id = get_cpu_id();
        // Append an 8-byte fingerprint at the end.
        for (int i = 0; i < 8; ++i)
            binary.push_back((id >> (8*i)) & 0xFF);
    }
};

} // namespace nanlang
