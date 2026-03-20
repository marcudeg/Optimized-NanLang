/**
 * NanLang AOT Compiler Implementation
 * v4.0.0 — pure Ahead-of-Time, no VM
 *
 * Bug fixes from v3.0.x:
 *  - generate(): removed dead xor/ret preamble that made every binary exit
 *    before any user code ran.
 *  - generate_instruction(PRINT): strings are now embedded in the data section
 *    appended after the code section; rip-relative addressing resolves the
 *    correct pointer at load time instead of passing NULL to write(2).
 *  - compile_to_native(): as_elf parameter was [[maybe_unused]] and the ELF
 *    path was always taken regardless; binary mode now really writes raw bytes.
 *  - compile_native_source (caller): default CompileOutputMode::Bytecode used
 *    to return ok=false silently; AOT is now the unconditional default path.
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>
#include <sys/stat.h>

#include "nan_aot_compiler.h"

namespace nanlang::aot {

// ─── Parser Implementation ──────────────────────────────────────────────────

Parser::Parser(const std::string& source_path)
    : source_path_(source_path) {
    std::ifstream src(source_path);
    if (src.is_open()) {
        std::string line;
        while (std::getline(src, line)) {
            lines_.push_back(line);
        }
    }
}

bool Parser::parse(std::vector<IRFunction>& functions) {
    if (lines_.empty()) {
        errors_.push_back("Empty source file");
        return false;
    }

    IRFunction main_func;
    main_func.name = "main";

    IRBasicBlock main_block;
    main_block.label = "entry";

    bool has_halt = false;

    for (const auto& raw : lines_) {
        // Strip line comments
        std::string line = raw;
        {
            bool in_str = false;
            for (size_t i = 0; i + 1 < line.size(); ++i) {
                if (line[i] == '"') in_str = !in_str;
                if (!in_str && line[i] == '/' && line[i+1] == '/') {
                    line.resize(i);
                    break;
                }
            }
            // trim
            size_t b = 0;
            while (b < line.size() && (line[b] == ' ' || line[b] == '\t')) ++b;
            size_t e = line.size();
            while (e > b && (line[e-1] == ' ' || line[e-1] == '\t')) --e;
            line = line.substr(b, e - b);
        }

        if (line.empty()) continue;
        if (line == "{" || line == "}" || line == "};") continue;
        if (line.rfind("fn ", 0) == 0) continue;
        if (line.rfind("let ", 0) == 0) continue;
        if (line.rfind("const ", 0) == 0) continue;
        if (line.rfind("on ", 0) == 0) continue;
        if (line.rfind("interrupt ", 0) == 0) continue;
        if (line.rfind("return", 0) == 0) continue;

// --- NanLang v4: Minimal Match Support ---

        static std::string current_match_val = "";
        static bool match_resolved = false;

        if (line.rfind("match ", 0) == 0) {
            match_resolved = false; 
            size_t v_start = line.find(' ') + 1;
            size_t v_end = line.find('{');
            if (v_end != std::string::npos) {
                current_match_val = line.substr(v_start, v_end - v_start);
                current_match_val.erase(0, current_match_val.find_first_not_of(" \t"));
                current_match_val.erase(current_match_val.find_last_not_of(" \t") + 1);
            }
            continue;
        }

        if (line == "}" || line == "};") {
            match_resolved = false;
            continue;
        }

        if (line.find("=>") != std::string::npos) {
            if (match_resolved) continue; 

            size_t arrow_pos = line.find("=>");
            std::string condition = line.substr(0, arrow_pos);
            std::string action = line.substr(arrow_pos + 2);
            
            condition.erase(0, condition.find_first_not_of(" \t"));
            condition.erase(condition.find_last_not_of(" \t") + 1);

            if (condition == current_match_val || condition == "_") {
                match_resolved = true; 
                action.erase(0, action.find_first_not_of(" \t"));
                if (action.rfind("say ", 0) == 0) {
                    line = action;
                } else { continue; }
            } else {
                continue; 
            }
        }
        if (line.rfind("say ", 0) == 0) {
            size_t start = line.find('"');
            size_t end   = line.rfind('"');
            if (start != std::string::npos && end != std::string::npos && start < end) {
                std::string text = line.substr(start + 1, end - start - 1);
                // Unescape \n
                std::string unescaped;
                for (size_t i = 0; i < text.size(); ++i) {
                    if (text[i] == '\\' && i + 1 < text.size()) {
                        switch (text[++i]) {
                            case 'n': unescaped += '\n'; break;
                            case 't': unescaped += '\t'; break;
                            case 'r': unescaped += '\r'; break;
                            case '"': unescaped += '"'; break;
                            default:  unescaped += '\\'; unescaped += text[i]; break;
                        }
                    } else {
                        unescaped += text[i];
                    }
                }
                // Append implicit newline
                unescaped += '\n';

                IRInstruction print_instr(IROp::PRINT);
                print_instr.label = unescaped;
                main_block.instructions.push_back(print_instr);
            }
            continue;
        }

        if (line.rfind("halt", 0) == 0) {
            IRInstruction halt_instr(IROp::HALT);
            main_block.instructions.push_back(halt_instr);
            has_halt = true;
            continue;
        }
    }

    // Always emit explicit halt so the process exits cleanly
    if (!has_halt) {
        IRInstruction halt_instr(IROp::HALT);
        main_block.instructions.push_back(halt_instr);
    }

    main_func.blocks.push_back(main_block);
    functions.push_back(main_func);
    return true;
}

// ─── Register Allocator Implementation ───────────────────────────────────────

RegisterAllocator::RegisterAllocator()
    : reg_used_(16, false), stack_offset_(0) {
    reg_used_[RBP] = true;
    reg_used_[RSP] = true;
}

uint8_t RegisterAllocator::allocate(const std::string& var_name) {
    for (size_t i = 0; i < 16; ++i) {
        if (i == RBP || i == RSP) continue;
        if (!reg_used_[i]) {
            reg_used_[i] = true;
            var_to_reg_[var_name] = static_cast<uint8_t>(i);
            return static_cast<uint8_t>(i);
        }
    }
    spill(var_name);
    return RAX;
}

void RegisterAllocator::free(const std::string& var_name) {
    auto it = var_to_reg_.find(var_name);
    if (it != var_to_reg_.end()) {
        reg_used_[it->second] = false;
        var_to_reg_.erase(it);
    }
}

void RegisterAllocator::spill(const std::string& var_name) {
    if (var_to_stack_.find(var_name) == var_to_stack_.end()) {
        var_to_stack_[var_name] = stack_offset_;
        stack_offset_ += 8;
    }
}

// ─── x86_64 Code Generator Implementation ────────────────────────────────────

void x86_64Generator::emit_u8(uint8_t byte) {
    code_.push_back(byte);
}

void x86_64Generator::emit_u32(uint32_t dword) {
    emit_u8(static_cast<uint8_t>(dword & 0xFF));
    emit_u8(static_cast<uint8_t>((dword >> 8) & 0xFF));
    emit_u8(static_cast<uint8_t>((dword >> 16) & 0xFF));
    emit_u8(static_cast<uint8_t>((dword >> 24) & 0xFF));
}

void x86_64Generator::emit_u64(uint64_t qword) {
    emit_u32(static_cast<uint32_t>(qword & 0xFFFFFFFFULL));
    emit_u32(static_cast<uint32_t>((qword >> 32) & 0xFFFFFFFFULL));
}

void x86_64Generator::emit_bytes(const std::vector<uint8_t>& bytes) {
    code_.insert(code_.end(), bytes.begin(), bytes.end());
}

void x86_64Generator::emit_mov_reg_imm64(uint8_t reg, uint64_t imm) {
    uint8_t rex_byte = 0x48;
    if (reg >= 8) rex_byte |= 0x01;
    emit_u8(rex_byte);
    emit_u8(static_cast<uint8_t>(0xB8 + (reg & 0x7)));
    emit_u64(imm);
}

void x86_64Generator::emit_add_reg_imm32(uint8_t reg, int32_t imm) {
    uint8_t rex_byte = 0x48;
    if (reg >= 8) rex_byte |= 0x01;
    emit_u8(rex_byte);
    emit_u8(0x81);
    emit_u8(static_cast<uint8_t>(0xC0 | (reg & 0x7)));
    emit_u32(static_cast<uint32_t>(imm));
}

void x86_64Generator::emit_mov_reg_reg(uint8_t dst, uint8_t src) {
    uint8_t rex_byte = 0x48;
    if (dst >= 8) rex_byte |= 0x01;
    if (src >= 8) rex_byte |= 0x04;
    emit_u8(rex_byte);
    emit_u8(0x89);
    emit_u8(static_cast<uint8_t>(0xC0 | ((src & 0x7) << 3) | (dst & 0x7)));
}

void x86_64Generator::emit_call_near(int32_t offset) {
    emit_u8(0xE8);
    emit_u32(static_cast<uint32_t>(offset));
}

void x86_64Generator::emit_ret() {
    emit_u8(0xC3);
}

void x86_64Generator::emit_syscall() {
    emit_u8(0x0F);
    emit_u8(0x05);
}

// ─── RIP-relative LEA helper ─────────────────────────────────────────────────
// Emits:  lea rsi, [rip + disp32]
// The caller must record the fixup position and patch disp32 once the data
// section offset is known.  We pass back the position of the disp32 field.

size_t x86_64Generator::emit_lea_rsi_rip_placeholder() {
    // REX.W + 8D /r  ModRM(0b00_110_101) = 0x35  disp32
    emit_u8(0x48);   // REX.W
    emit_u8(0x8D);   // LEA opcode
    emit_u8(0x35);   // ModRM: reg=RSI(6), mod=00, rm=101 (RIP+disp32)
    size_t disp_pos = code_.size();
    emit_u32(0x00000000); // placeholder disp32
    return disp_pos;
}

void x86_64Generator::generate_function(const IRFunction& func) {
    std::cout << "[AOT] Generating function: " << func.name << "\n";

    bool is_main = (func.name == "main");

    if (!is_main) {
        emit_u8(0x55);        // push rbp
        emit_u8(0x48); emit_u8(0x89); emit_u8(0xE5); // mov rbp, rsp
        if (func.stack_size > 0) {
            emit_add_reg_imm32(RegisterAllocator::RSP,
                               -static_cast<int32_t>(func.stack_size));
        }
    }

    for (const auto& block : func.blocks) {
        label_positions_[block.label] = code_.size();
        generate_basic_block(block);
    }

    if (!is_main) {
        emit_u8(0x48); emit_u8(0x89); emit_u8(0xEC); // mov rsp, rbp
        emit_u8(0x5D);                                // pop rbp
    }

    emit_ret();
}

void x86_64Generator::generate_basic_block(const IRBasicBlock& block) {
    for (const auto& instr : block.instructions) {
        generate_instruction(instr);
    }
}

void x86_64Generator::generate_instruction(const IRInstruction& instr) {
    switch (instr.op) {
        case IROp::MOV: {
            if (instr.src1.kind == IRValue::Kind::Immediate) {
                emit_mov_reg_imm64(static_cast<uint8_t>(instr.dst.value),
                                   instr.src1.value);
            }
            break;
        }

        case IROp::PRINT: {
            // FIX: embed the string in the data pool and use a RIP-relative
            // lea to obtain its address at runtime.  Previously this emitted
            // mov rsi, 0 (null pointer) which made write(2) write nothing /
            // segfault.

            const std::string& text = instr.label;
            const uint64_t text_len = text.size();

            // Record offset in data pool for this string
            size_t pool_index = string_pool_.size();
            string_pool_.push_back(text);
            string_pool_offsets_.push_back(0); // filled in generate()

            // mov rax, 1  (SYS_write)
            emit_mov_reg_imm64(RegisterAllocator::RAX, 1);
            // mov rdi, 1  (stdout fd)
            emit_mov_reg_imm64(RegisterAllocator::RDI, 1);
            // lea rsi, [rip + ???]  — placeholder, fixup after code section known
            size_t disp_pos = emit_lea_rsi_rip_placeholder();
            print_fixups_.push_back({disp_pos, pool_index});
            // mov rdx, len
            emit_mov_reg_imm64(RegisterAllocator::RDX, text_len);
            // syscall
            emit_syscall();
            break;
        }

        case IROp::HALT: {
            // mov rax, 60  (SYS_exit)
            emit_mov_reg_imm64(RegisterAllocator::RAX, 60);
            // mov rdi, 0   (exit code 0)
            emit_mov_reg_imm64(RegisterAllocator::RDI, 0);
            // syscall
            emit_syscall();
            break;
        }

        default:
            // Unsupported instruction — emit NOP so code stays valid
            emit_u8(0x90);
            break;
    }
}

std::vector<uint8_t> x86_64Generator::generate(const std::vector<IRFunction>& functions) {
    code_.clear();
    label_positions_.clear();
    label_fixups_.clear();
    print_fixups_.clear();
    string_pool_.clear();
    string_pool_offsets_.clear();

    // FIX: removed the dead "xor eax,eax / ret" preamble that was emitted
    // *before* generate_function(), causing every binary to return immediately
    // at the entry point before executing any user code.

    for (const auto& func : functions) {
        generate_function(func);
    }

    // ── Build data section (string pool) ─────────────────────────────────
    // Compute each string's offset within the data section
    size_t pool_base_in_code = code_.size(); // data will be appended here
    size_t running_offset = 0;
    for (size_t i = 0; i < string_pool_.size(); ++i) {
        string_pool_offsets_[i] = running_offset;
        running_offset += string_pool_[i].size();
    }

    // Patch RIP-relative displacements
    // For each fixup: disp32 = (data_address) - (end_of_lea_instruction)
    // end_of_lea_instruction = code_base + disp_pos + 4
    // data_address in file   = code_base + pool_base_in_code + string_pool_offsets_[idx]
    // (code_base cancels; all arithmetic is relative)
    for (const auto& fix : print_fixups_) {
        size_t disp_pos  = fix.first;
        size_t str_idx   = fix.second;
        size_t instr_end = disp_pos + 4; // disp32 is 4 bytes wide
        size_t str_off   = pool_base_in_code + string_pool_offsets_[str_idx];
        // disp = str_off - instr_end  (signed 32-bit)
        int64_t disp = static_cast<int64_t>(str_off) - static_cast<int64_t>(instr_end);
        uint32_t disp32 = static_cast<uint32_t>(static_cast<int32_t>(disp));
        code_[disp_pos + 0] = static_cast<uint8_t>(disp32 & 0xFF);
        code_[disp_pos + 1] = static_cast<uint8_t>((disp32 >> 8) & 0xFF);
        code_[disp_pos + 2] = static_cast<uint8_t>((disp32 >> 16) & 0xFF);
        code_[disp_pos + 3] = static_cast<uint8_t>((disp32 >> 24) & 0xFF);
    }

    // Append string data
    for (const auto& s : string_pool_) {
        code_.insert(code_.end(), s.begin(), s.end());
    }

    return code_;
}

// ─── AOT Compiler Implementation ─────────────────────────────────────────────

bool AOTCompiler::compile_to_elf(const std::string& output_path) {
    return compile_to_native(output_path, true);
}

bool AOTCompiler::compile_to_binary(const std::string& output_path) {
    return compile_to_native(output_path, false);
}

bool AOTCompiler::compile_to_native(
    const std::string& output_path,
    bool as_elf
) {
    std::cout << "[AOT] NanLang v4.0.0 — Ahead-of-Time compiler\n";
    std::cout << "[AOT] Compiling: " << source_path_ << "\n";

    // Phase 1: Parse
    std::cout << "[AOT] Phase 1: Parsing...\n";
    Parser parser(source_path_);
    std::vector<IRFunction> ir_functions;

    if (!parser.parse(ir_functions)) {
        for (const auto& err : parser.get_errors()) {
            errors_.push_back(err);
        }
        std::cerr << "[ERROR] Parse failed\n";
        return false;
    }

    std::cout << "[AOT] ✓ Parsed " << ir_functions.size() << " function(s)\n";

    // Phase 2: Code generation
    std::cout << "[AOT] Phase 2: x86_64 native code generation...\n";
    x86_64Generator codegen;
    std::vector<uint8_t> machine_code = codegen.generate(ir_functions);

    std::cout << "[AOT] ✓ Generated " << machine_code.size() << " bytes of native code\n";

    // Phase 3: Output
    std::cout << "[AOT] Phase 3: Writing output (" << (as_elf ? "ELF" : "raw") << ")...\n";

    // FIX: as_elf was previously [[maybe_unused]] — both paths always built
    // an ELF.  Now raw binary mode really skips the ELF wrapper.
    std::vector<uint8_t> output_bytes =
        as_elf ? build_elf_executable(machine_code) : machine_code;

    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        errors_.push_back("Could not open output file: " + output_path);
        return false;
    }

    out.write(reinterpret_cast<const char*>(output_bytes.data()),
              static_cast<std::streamsize>(output_bytes.size()));

    if (!out.good()) {
        errors_.push_back("Failed to write output file");
        return false;
    }

    out.close();

    std::cout << "[AOT] ✓ Wrote " << output_bytes.size() << " bytes to " << output_path << "\n";
    std::cout << "[AOT] ✅ Compilation complete!\n";

    if (as_elf) {
        chmod(output_path.c_str(), 0755);
    }

    return true;
}

std::vector<uint8_t> AOTCompiler::build_elf_executable(const std::vector<uint8_t>& machine_code) {
    std::vector<uint8_t> out;
    const uint64_t image_base  = 0x400000ULL;
    const uint64_t ehdr_size   = 64;
    const uint64_t phdr_size   = 56;
    const uint64_t code_offset = ehdr_size + phdr_size;
    const uint64_t file_size   = code_offset + machine_code.size();

    out.reserve(static_cast<size_t>(file_size));

    // ELF64 header
    out.push_back(0x7F); out.push_back('E'); out.push_back('L'); out.push_back('F');
    out.push_back(2);    // ELFCLASS64
    out.push_back(1);    // ELFDATA2LSB
    out.push_back(1);    // EV_CURRENT
    out.push_back(0);    // ELFOSABI_NONE
    for (int i = 0; i < 8; ++i) out.push_back(0); // padding

    // e_type ET_EXEC
    out.push_back(0x02); out.push_back(0x00);
    // e_machine EM_X86_64
    out.push_back(0x3E); out.push_back(0x00);
    // e_version
    out.push_back(0x01); out.push_back(0x00); out.push_back(0x00); out.push_back(0x00);

    // e_entry
    uint64_t entry = image_base + code_offset;
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((entry >> (i*8)) & 0xFF));

    // e_phoff
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((ehdr_size >> (i*8)) & 0xFF));

    // e_shoff (none)
    for (int i = 0; i < 8; ++i) out.push_back(0);

    // e_flags
    for (int i = 0; i < 4; ++i) out.push_back(0);

    // e_ehsize = 64
    out.push_back(0x40); out.push_back(0x00);
    // e_phentsize = 56
    out.push_back(0x38); out.push_back(0x00);
    // e_phnum = 1
    out.push_back(0x01); out.push_back(0x00);
    // e_shentsize, e_shnum, e_shstrndx = 0
    for (int i = 0; i < 6; ++i) out.push_back(0);

    // Program header PT_LOAD
    // p_type PT_LOAD
    out.push_back(0x01); out.push_back(0x00); out.push_back(0x00); out.push_back(0x00);
    // p_flags PF_X|PF_R
    out.push_back(0x05); out.push_back(0x00); out.push_back(0x00); out.push_back(0x00);
    // p_offset = 0 (map whole file)
    for (int i = 0; i < 8; ++i) out.push_back(0);
    // p_vaddr
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((image_base >> (i*8)) & 0xFF));
    // p_paddr
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((image_base >> (i*8)) & 0xFF));
    // p_filesz
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((file_size >> (i*8)) & 0xFF));
    // p_memsz
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((file_size >> (i*8)) & 0xFF));
    // p_align = 0x1000
    const uint64_t align = 0x1000;
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((align >> (i*8)) & 0xFF));

    out.insert(out.end(), machine_code.begin(), machine_code.end());
    return out;
}

// ─── Optimization Passes (stubs — hooks for future passes) ───────────────────

void ConstantPropagation::run(std::vector<IRBasicBlock>& /*blocks*/) {
    // TODO: propagate compile-time constants through the IR
}

void DeadCodeElimination::run(std::vector<IRBasicBlock>& /*blocks*/) {
    // TODO: remove unreachable instructions after halt
}

void PeepholeOptimizer::run(std::vector<uint8_t>& /*machine_code*/) {
    // TODO: collapse redundant mov-mov pairs, etc.
}

}  // namespace nanlang::aot
