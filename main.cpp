/**
 * NanLang Integrated Toolchain v4.0.0
 */
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "include/nan_core.h"
#include "include/nan_pulse.h"
#include "include/nan_binary.h"
#include "include/nan_arch.h"
#include "include/nan_compiler.h"
#include "include/nan_perf.h"

namespace nanlang {
    void run_pulse_emitter_demo(int count, uint8_t pattern);
    void run_signal_buffer_demo(int items);
    void run_simd_demo(int iterations);
    void run_dma_demo(size_t size_bytes);
    void run_zerocopy_demo(const std::vector<uint8_t>& data, size_t offset, size_t len);
    void run_register_map_demo();
    void run_binary_patch_demo(const std::string& path, size_t offset,
                                const std::vector<uint8_t>& patch_bytes);
    void run_obfuscation_demo(const std::vector<uint8_t>& data, uint64_t cpu_id);
    void run_compression_demo(const std::vector<uint8_t>& data);
    void run_selfheal_demo(uint8_t nibble);
    void run_jit_demo();
    void run_linker_demo(int num_segments, bool strip);
    void run_regcolor_demo();
    void run_constant_fold_demo();
    void run_crossplatform_demo();
    void run_hwfp_demo();
    void run_waitfree_demo(int count);
    void run_deadlock_demo();
    void run_branch_predictor_demo(int rounds);
    void run_pipeline_reorder_demo();
    void run_ooo_demo();
}

struct Args {
    std::unordered_map<std::string, std::string> kv;
    std::unordered_map<std::string, bool>        flags;

    static Args parse(int argc, char** argv) {
        Args a;
        for (int i = 1; i < argc; ++i) {
            std::string tok(argv[i]);
            if (tok.size() > 2 && tok[0] == '-' && tok[1] == '-') {
                std::string key = tok.substr(2);
                if (i + 1 < argc && argv[i+1][0] != '-') {
                    a.kv[key] = argv[++i];
                } else {
                    a.flags[key] = true;
                }
            }
        }
        return a;
    }

    bool has(const std::string& k) const { return kv.count(k) || flags.count(k); }
    std::string get(const std::string& k, const std::string& def = "") const {
        auto it = kv.find(k); return it != kv.end() ? it->second : def;
    }
    int get_int(const std::string& k, int def = 0) const {
        auto it = kv.find(k);
        if (it == kv.end()) return def;
        try { return std::stoi(it->second, nullptr, 0); } catch (...) { return def; }
    }
    uint64_t get_u64(const std::string& k, uint64_t def = 0) const {
        auto it = kv.find(k);
        if (it == kv.end()) return def;
        try { return std::stoull(it->second, nullptr, 0); } catch (...) { return def; }
    }
    size_t get_size(const std::string& k, size_t def = 0) const {
        return static_cast<size_t>(get_u64(k, def));
    }
    bool flag(const std::string& k) const { return flags.count(k) > 0; }
};

static void print_help() {
    std::cout << R"(
NanLang Toolchain v4.0.0  — Fully Parameterized CLI
==================================================

COMPILE & RUN
  --compile <file.nl>   [--output <out.nb>]     Generate bytecode
  --run     <file.nb>                            Run in VM
  --build-native <file.nl> [--output <n>] [--opt]  Native binary

PATCH
  --patch <file> --offset <hex> --bytes "DE AD BE EF"

DEMO  (--demo <name> [parameters])
  pulse    --count N --pattern 0xXX
  sigbuf   --count N
  simd     --iterations N
  dma      --size N
  zerocopy --offset N --len N
  regmap
  binary   --cpu-key 0xXX
  compress
  selfheal --nibble 0xX
  jit
  linker   --segments N [--strip-nops]
  color
  fold
  xplatform
  hwfp
  waitfree --count N
  deadlock
  branch   --rounds N
  pipeline
  ooo
  all      (run all demos)

  --help | --version
)";
}

class NanCompiler {
public:
    void generate_bytecode(const std::string& input, const std::string& output) {
        std::ifstream src(input);
        if (!src.is_open()) throw std::runtime_error("Source not found: " + input);
        std::ofstream bin(output, std::ios::binary);
        uint32_t header = 0x214E414E;
        bin.write(reinterpret_cast<char*>(&header), sizeof(header));
        std::string line;
        while (std::getline(src, line)) {
            if (line.find("say ") != std::string::npos) {
                uint8_t op = nanlang::OP_SAY;
                bin.write(reinterpret_cast<char*>(&op), sizeof(op));
                std::string content = extract_string(line);
                uint32_t length = static_cast<uint32_t>(content.size());
                bin.write(reinterpret_cast<char*>(&length), sizeof(length));
                bin.write(content.c_str(), length);
            }
        }
        std::cout << "[OK] Bytecode generated: " << output << "\n";
    }

    void build_native(const std::string& input, const std::string& out_name, bool optimize) {
        std::string stub = "nan_temp__.cpp";
        std::ofstream s(stub);
        s << "#include <iostream>\nint main() {\n";
        std::ifstream src(input);
        std::string line;
        while (std::getline(src, line))
            if (line.find("say ") != std::string::npos)
                s << "  std::cout << \"" << extract_string(line) << "\" << std::endl;\n";
        s << "  return 0;\n}\n"; s.close();
        std::string cmd = "g++ " + stub + " -o " + out_name + " -std=c++17";
        if (optimize) cmd += " -O3 -march=native -flto";
        std::cout << "[BUILD] " << cmd << "\n";
        if (std::system(cmd.c_str()) == 0) {
            std::cout << "[OK] Binary: ./" << out_name << "\n";
            std::remove(stub.c_str());
        } else { std::cerr << "[FAIL] Native build failed.\n"; }
    }

private:
    std::string extract_string(const std::string& line) {
        size_t start = line.find('"') + 1, end = line.rfind('"');
        if (start == std::string::npos || end == std::string::npos) return "";
        return line.substr(start, end - start);
    }
};

class NanRuntime {
public:
    void run(const std::string& path) {
        std::ifstream bin(path, std::ios::binary);
        if (!bin.is_open()) { std::cerr << "[RT] Could not open: " << path << "\n"; return; }
        uint32_t header;
        bin.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (header != 0x214E414E) { std::cerr << "[RT] Invalid magic.\n"; return; }
        uint8_t opcode;
        while (bin.read(reinterpret_cast<char*>(&opcode), sizeof(opcode))) {
            if (opcode == nanlang::OP_SAY) {
                uint32_t len;
                bin.read(reinterpret_cast<char*>(&len), sizeof(len));
                std::vector<char> buf(len);
                bin.read(buf.data(), len);
                std::cout << std::string(buf.begin(), buf.end()) << "\n";
            }
        }
    }
};

static std::vector<uint8_t> parse_hex_bytes(const std::string& s) {
    std::vector<uint8_t> out;
    std::istringstream ss(s);
    std::string tok;
    while (ss >> tok) {
        try { out.push_back(static_cast<uint8_t>(std::stoul(tok, nullptr, 16))); }
        catch (...) {}
    }
    return out;
}

static void run_demo(const std::string& name, const Args& args) {
    std::cout << "\n====== DEMO: " << name << " ======\n";
    int    count      = args.get_int("count",      64);
    int    iterations = args.get_int("iterations", 100);
    int    rounds     = args.get_int("rounds",     256);
    int    segments   = args.get_int("segments",   3);
    size_t size       = args.get_size("size",      4096);
    size_t offset     = args.get_size("offset",    0);
    size_t len        = args.get_size("len",       4);
    uint64_t cpu_key  = args.get_u64("cpu-key",    0xCAFEBABEDEADBEEFULL);
    uint8_t pattern   = static_cast<uint8_t>(args.get_int("pattern", 0xAA));
    uint8_t nibble    = static_cast<uint8_t>(args.get_int("nibble",  0x0B));
    bool strip        = args.flag("strip-nops");

    if      (name == "pulse")    nanlang::run_pulse_emitter_demo(count, pattern);
    else if (name == "sigbuf")   nanlang::run_signal_buffer_demo(count);
    else if (name == "simd")     nanlang::run_simd_demo(iterations);
    else if (name == "dma")      nanlang::run_dma_demo(size);
    else if (name == "zerocopy") {
        std::vector<uint8_t> data(32);
        for (int i=0;i<32;++i) data[i]=static_cast<uint8_t>(i*3);
        nanlang::run_zerocopy_demo(data, offset, len);
    }
    else if (name == "regmap")    nanlang::run_register_map_demo();
    else if (name == "binary") {
        std::vector<uint8_t> sample={0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80};
        nanlang::run_obfuscation_demo(sample, cpu_key);
    }
    else if (name == "compress") {
        std::vector<uint8_t> data={0xAA,0xAA,0xAA,0xBB,0xCC,0xCC,0xDD,0xDD,0xDD,0xDD};
        nanlang::run_compression_demo(data);
    }
    else if (name == "selfheal")  nanlang::run_selfheal_demo(nibble & 0x0F);
    else if (name == "jit")       nanlang::run_jit_demo();
    else if (name == "linker")    nanlang::run_linker_demo(segments, strip);
    else if (name == "color")     nanlang::run_regcolor_demo();
    else if (name == "fold")      nanlang::run_constant_fold_demo();
    else if (name == "xplatform") nanlang::run_crossplatform_demo();
    else if (name == "hwfp")      nanlang::run_hwfp_demo();
    else if (name == "waitfree")  nanlang::run_waitfree_demo(count);
    else if (name == "deadlock")  nanlang::run_deadlock_demo();
    else if (name == "branch")    nanlang::run_branch_predictor_demo(rounds);
    else if (name == "pipeline")  nanlang::run_pipeline_reorder_demo();
    else if (name == "ooo")       nanlang::run_ooo_demo();
    else if (name == "all") {
        for (auto& d : {"pulse","sigbuf","simd","dma","zerocopy","regmap",
                        "binary","compress","selfheal","jit","linker","color",
                        "fold","xplatform","hwfp","waitfree","deadlock",
                        "branch","pipeline","ooo"})
            run_demo(d, args);
    }
    else std::cerr << "[ERROR] Unknown demo: " << name << "\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { print_help(); return 0; }
    Args args = Args::parse(argc, argv);

    if (args.has("help"))    { print_help(); return 0; }
    if (args.has("version")) { std::cout << "NanLang v4.0.0\n"; return 0; }

    if (args.has("compile")) {
        std::string src = args.get("compile");
        std::string out = args.get("output", "output.nb");
        if (!std::filesystem::exists(src)) {
            std::cerr << "[ERROR] File not found: " << src << "\n"; return 1;
        }
        NanCompiler compiler;
        try { compiler.generate_bytecode(src, out); }
        catch (const std::exception& e) { std::cerr << "[FATAL] " << e.what() << "\n"; return 1; }
        return 0;
    }

    if (args.has("run")) {
        std::string nb = args.get("run");
        if (!std::filesystem::exists(nb)) {
            std::cerr << "[ERROR] Bytecode not found: " << nb << "\n"; return 1;
        }
        NanRuntime rt; rt.run(nb); return 0;
    }

    if (args.has("build-native")) {
        std::string src = args.get("build-native");
        std::string out = args.get("output", "app.elf");
        bool opt        = args.flag("opt");
        if (!std::filesystem::exists(src)) {
            std::cerr << "[ERROR] Source not found: " << src << "\n"; return 1;
        }
        NanCompiler compiler;
        compiler.build_native(src, out, opt);
        return 0;
    }

    if (args.has("patch")) {
        std::string file = args.get("patch");
        size_t off       = args.get_size("offset", 0);
        auto   bytes     = parse_hex_bytes(args.get("bytes", "90"));
        nanlang::run_binary_patch_demo(file, off, bytes);
        return 0;
    }

    if (args.has("demo")) {
        run_demo(args.get("demo"), args);
        return 0;
    }

    std::cerr << "[ERROR] Invalid arguments. Use --help.\n";
    return 1;
}
