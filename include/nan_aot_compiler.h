/**
 * NanLang Ahead-of-Time (AOT) Compiler
 * 
 * Converts NanLang source directly to optimized native code (ELF)
 * without intermediate bytecode representation.
 * 
 * Pipeline:
 *   Source.nl → Parser → IR → Register Allocator → Native Code → ELF
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace nanlang::aot {

// ─── IR (Intermediate Representation) ────────────────────────────────────────

enum class IROp {
    // Data movement
    MOV,        // move immediate/register
    LOAD,       // load from memory
    STORE,      // store to memory
    
    // Arithmetic
    ADD, SUB, MUL, DIV, MOD,
    AND, OR, XOR, NOT,
    SHL, SHR, ROT,
    
    // Control flow
    JMP,        // unconditional jump
    JEQ,        // jump if equal
    JNE,        // jump if not equal
    JLT,        // jump if less than
    JLE,        // jump if less than or equal
    JGT,        // jump if greater than
    JGE,        // jump if greater than or equal
    CALL,       // function call
    RET,        // return
    
    // System
    SYSCALL,    // system call
    PRINT,      // output text
    HALT,       // program termination
};

struct IRValue {
    enum class Kind { Immediate, Register, Memory, Label } kind;
    
    uint64_t value = 0;      // for Immediate, register ID, or memory offset
    std::string label;       // for Label
    
    static IRValue imm(uint64_t v) {
        IRValue iv;
        iv.kind = Kind::Immediate;
        iv.value = v;
        return iv;
    }
    
    static IRValue reg(uint64_t reg_id) {
        IRValue iv;
        iv.kind = Kind::Register;
        iv.value = reg_id;
        return iv;
    }
    
    static IRValue mem(uint64_t offset) {
        IRValue iv;
        iv.kind = Kind::Memory;
        iv.value = offset;
        return iv;
    }
    
    static IRValue lbl(const std::string& name) {
        IRValue iv;
        iv.kind = Kind::Label;
        iv.label = name;
        return iv;
    }
};

struct IRInstruction {
    IROp op;
    IRValue dst;
    IRValue src1;
    IRValue src2;
    std::string label;  // for CALL
    
    IRInstruction() : op(IROp::MOV) {}
    
    IRInstruction(IROp op_)
        : op(op_) {}
    
    IRInstruction(IROp op_, IRValue d)
        : op(op_), dst(d) {}
    
    IRInstruction(IROp op_, IRValue d, IRValue s1)
        : op(op_), dst(d), src1(s1) {}
    
    IRInstruction(IROp op_, IRValue d, IRValue s1, IRValue s2)
        : op(op_), dst(d), src1(s1), src2(s2) {}
};

struct IRBasicBlock {
    std::string label;
    std::vector<IRInstruction> instructions;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;
};

struct IRFunction {
    std::string name;
    std::vector<std::string> params;
    std::vector<IRBasicBlock> blocks;
    uint64_t stack_size = 0;
};

// ─── Parser ─────────────────────────────────────────────────────────────────

class Parser {
public:
    explicit Parser(const std::string& source_path);
    
    bool parse(std::vector<IRFunction>& functions);
    
    const std::vector<std::string>& get_errors() const { return errors_; }
    
private:
    std::string source_path_;
    std::vector<std::string> lines_;
    std::vector<std::string> errors_;
    size_t current_line_ = 0;
    
    bool parse_function(IRFunction& func);
    bool parse_statement(IRBasicBlock& block);
    bool parse_expression(IRBasicBlock& block);
};

// ─── Register Allocator ──────────────────────────────────────────────────────

class RegisterAllocator {
public:
    // x86_64 calling convention: RDI, RSI, RDX, RCX, R8, R9 (args)
    // Scratch: RAX, RCX, RDX, RSI, RDI, R8-R11
    // Preserved: RBX, RSP, RBP, R12-R15
    
    static constexpr uint8_t RAX = 0;
    static constexpr uint8_t RBX = 3;
    static constexpr uint8_t RCX = 1;
    static constexpr uint8_t RDX = 2;
    static constexpr uint8_t RSI = 6;
    static constexpr uint8_t RDI = 7;
    static constexpr uint8_t R8 = 8;
    static constexpr uint8_t R9 = 9;
    static constexpr uint8_t R10 = 10;
    static constexpr uint8_t R11 = 11;
    static constexpr uint8_t R12 = 12;
    static constexpr uint8_t R13 = 13;
    static constexpr uint8_t R14 = 14;
    static constexpr uint8_t R15 = 15;
    static constexpr uint8_t RSP = 4;
    static constexpr uint8_t RBP = 5;
    
    RegisterAllocator();
    
    uint8_t allocate(const std::string& var_name);
    void free(const std::string& var_name);
    void spill(const std::string& var_name);
    
    uint64_t stack_offset() const { return stack_offset_; }
    
private:
    std::unordered_map<std::string, uint8_t> var_to_reg_;
    std::unordered_map<std::string, uint64_t> var_to_stack_;
    std::vector<bool> reg_used_;
    uint64_t stack_offset_ = 0;
};

// ─── Code Generator (x86_64) ────────────────────────────────────────────────

class x86_64Generator {
public:
    std::vector<uint8_t> generate(const std::vector<IRFunction>& functions);
    
private:
    std::vector<uint8_t> code_;
    RegisterAllocator alloc_;
    
    // Emission helpers
    void emit_u8(uint8_t byte);
    void emit_u32(uint32_t dword);
    void emit_u64(uint64_t qword);
    void emit_bytes(const std::vector<uint8_t>& bytes);
    
    // x86_64 instruction generation
    void emit_mov_reg_imm64(uint8_t reg, uint64_t imm);
    void emit_add_reg_imm32(uint8_t reg, int32_t imm);
    void emit_mov_reg_reg(uint8_t dst, uint8_t src);
    void emit_call_near(int32_t offset);
    void emit_ret();
    void emit_syscall();
    
    // Function-level code generation
    void generate_function(const IRFunction& func);
    void generate_basic_block(const IRBasicBlock& block);
    void generate_instruction(const IRInstruction& instr);
    
    // RIP-relative lea helper — returns position of the disp32 field
    size_t emit_lea_rsi_rip_placeholder();

    // Label management
    std::unordered_map<std::string, size_t> label_positions_;
    std::vector<std::pair<size_t, std::string>> label_fixups_;

    // String data pool — populated during PRINT instruction generation,
    // appended after the code section and patched via RIP-relative fixups.
    std::vector<std::string> string_pool_;
    std::vector<size_t>      string_pool_offsets_;
    // Each entry: {disp32 field position in code_, index into string_pool_}
    std::vector<std::pair<size_t, size_t>> print_fixups_;
};

// ─── Main AOT Compiler ───────────────────────────────────────────────────────

class AOTCompiler {
public:
    explicit AOTCompiler(const std::string& source_path)
        : source_path_(source_path) {}
    
    /**
     * Compile source to native ELF executable
     * @param output_path Output file path
     * @return true if compilation succeeded
     */
    bool compile_to_elf(const std::string& output_path);
    
    /**
     * Compile source to raw machine code
     * @param output_path Output file path
     * @return true if compilation succeeded
     */
    bool compile_to_binary(const std::string& output_path);
    
    const std::vector<std::string>& get_errors() const { return errors_; }
    
private:
    std::string source_path_;
    std::vector<std::string> errors_;
    
    bool compile_to_native(
        const std::string& output_path,
        bool as_elf
    );
    
    static std::vector<uint8_t> build_elf_executable(const std::vector<uint8_t>& machine_code);
};

// ─── Optimization Passes (Optional) ──────────────────────────────────────────

class ConstantPropagation {
public:
    static void run(std::vector<IRBasicBlock>& blocks);
};

class DeadCodeElimination {
public:
    static void run(std::vector<IRBasicBlock>& blocks);
};

class PeepholeOptimizer {
public:
    static void run(std::vector<uint8_t>& machine_code);
};

}  // namespace nanlang::aot
