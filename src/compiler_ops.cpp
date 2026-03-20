#include "../include/nan_compiler.h"
#include <iostream>

namespace nanlang {

void run_jit_demo() {
    auto stub = JITPulseGen::make_zero_func(); // xor eax,eax; ret
    auto fn   = JITPulseGen::compile(stub);

    if (fn) {
        fn(); // execute
        std::cout << "[JIT] Stub executed successfully.\n";
    } else {
        std::cout << "[JIT] Compilation skipped (platform/perms).\n";
    }
}

void run_linker_demo(int num_segments, bool strip) {
    NanLinker linker;
    for (int i = 0; i < num_segments; ++i) {
        std::vector<uint8_t> seg;
        for (int j = 0; j < 8; ++j)
            seg.push_back((j % 3 == 0) ? static_cast<uint8_t>(NanOpcode::NOP)
                                        : static_cast<uint8_t>(j + 1));
        linker.add_segment(seg);
    }
    auto linked = linker.link(strip);
    std::cout << "[LINKER] segments=" << num_segments
              << " total_raw=" << linker.total_size()
              << " linked=" << linked.size()
              << " strip_nops=" << strip << "\n";
}

void run_regcolor_demo() {
    RegisterColoring rc;
    int a = rc.add_var("x"); int b = rc.add_var("y");
    int c = rc.add_var("z"); int d = rc.add_var("w");
    rc.add_conflict(a, b); rc.add_conflict(b, c);
    rc.add_conflict(a, c); rc.add_conflict(c, d);

    bool ok = rc.color();
    std::cout << "[COLOR] success=" << ok << "\n";
    if (ok) rc.dump();
}

void run_constant_fold_demo() {
    std::vector<uint8_t> code = {
        NanOpcode::ADD, 10, 20,   // → LOAD 30
        NanOpcode::SAY, 'H', 'i', // should pass
        NanOpcode::XOR, 0xFF, 0x0F // → LOAD 0xF0
    };
    auto folded = ConstantFolder::fold_bytecode(code);
    std::cout << "[FOLD] original=" << code.size()
              << " folded=" << folded.size() << "\n";
}

void run_crossplatform_demo() {
    using IR = CrossPlatformEmitter::IRInstruction;
    std::vector<IR> ir = {
        {NanOpcode::LOAD, 42},
        {NanOpcode::ADD,  8},
        {NanOpcode::RET,  0}
    };
    auto x86 = CrossPlatformEmitter::emit(ir, CrossPlatformEmitter::Arch::X86_64);
    auto arm  = CrossPlatformEmitter::emit(ir, CrossPlatformEmitter::Arch::ARM64);

    std::cout << "[XPLATFORM] x86_64=" << x86.size()
              << " bytes  arm64=" << arm.size() << " bytes\n";
}

void run_hwfp_demo() {
    uint64_t cpu_id = HWFingerprint::get_cpu_id();
    std::cout << "[HWFP] CPU-ID=0x" << std::hex << cpu_id << std::dec;

    std::vector<uint8_t> binary = {0xDE, 0xAD, 0xBE, 0xEF};
    HWFingerprint::embed(binary);
    std::cout << "  binary_with_fp=" << binary.size() << " bytes";

    bool valid = HWFingerprint::validate(cpu_id);
    std::cout << "  validate=" << (valid ? "OK" : "FAIL") << "\n";
}

} // namespace nanlang
