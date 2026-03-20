#include "../include/nan_cli_lib.h"

#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Parsed {
    std::vector<std::string> positional;
    std::unordered_map<std::string, std::string> kv;
    std::unordered_map<std::string, bool> flags;
};

Parsed parse_tokens(const std::vector<std::string>& args) {
    Parsed p;
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& tok = args[i];
        if (tok.rfind("--", 0) == 0) {
            std::string key = tok.substr(2);
            size_t eq = key.find('=');
            if (eq != std::string::npos) {
                p.kv[key.substr(0, eq)] = key.substr(eq + 1);
            } else if (i + 1 < args.size() && args[i + 1].rfind("-", 0) != 0) {
                p.kv[key] = args[++i];
            } else {
                p.flags[key] = true;
            }
            continue;
        }
        if (tok.size() == 2 && tok[0] == '-') {
            std::string key(1, tok[1]);
            if (i + 1 < args.size() && args[i + 1].rfind("-", 0) != 0) {
                p.kv[key] = args[++i];
            } else {
                p.flags[key] = true;
            }
            continue;
        }
        p.positional.push_back(tok);
    }
    return p;
}

std::string make_temp_preprocessed_file() {
    char tmpl[] = "/tmp/nanlang_preprocessed_XXXXXX.nl";
    int fd = mkstemps(tmpl, 3);
    if (fd < 0) {
        throw std::runtime_error("could not create temporary preprocessed file");
    }
    close(fd);
    return std::string(tmpl);
}

int find_compile_source_index(const std::vector<std::string>& args) {
    bool skip_next = false;
    for (size_t i = 0; i < args.size(); ++i) {
        if (skip_next) {
            skip_next = false;
            continue;
        }
        const std::string& a = args[i];
        if (a == "--output" || a == "-o") {
            skip_next = true;
            continue;
        }
        if (a.rfind("--output=", 0) == 0) continue;
        if (!a.empty() && a[0] == '-') continue;
        return static_cast<int>(i);
    }
    return -1;
}

void print_help() {
    std::cout << nancli::full_banner() << "\n\n"
              << "Usage:\n"
              << "  nanlang <command> [args]\n\n"
              << "Commands:\n"
              << "  help                         Show this help\n"
              << "  version                      Print version banner\n"
              << "  pkg install -r req.txt       Resolve package requirements\n"
              << "  build --src app.nl           Compile source with optional package resolution\n"
              << "  compile <src.nl> --output o  Preprocess + native compile (--elf default)\n"
              << "  run <file.elf|script>        Execute native artifact\n"
              << "  ui-render <scene.nl>         Open the NanLang GTK renderer\n"
              << "  demo <name> [flags]          Run C++ demo mode\n";
}

int run_pkg(std::vector<std::string> args) {
    if (!args.empty() && args[0] == "install") {
        args.erase(args.begin());
    }
    Parsed p = parse_tokens(args);

    std::string req = p.kv.count("r") ? p.kv["r"] : "";
    if (req.empty()) {
        throw std::runtime_error("missing required flag: -r <req.txt>");
    }
    std::string conf = p.kv.count("config") ? p.kv["config"] : nancli::config_path();

    nancli::BuildOptions opts;
    opts.req_path = req;
    opts.conf_path = conf;
    opts.dry_run = p.flags.count("dry-run") > 0;
    opts.verbose = p.flags.count("verbose") > 0;
    nancli::run_build(opts);
    return 0;
}

int run_build_cmd(const std::vector<std::string>& args) {
    Parsed p = parse_tokens(args);
    std::string src = p.kv.count("src") ? p.kv["src"] : "";
    std::string out = p.kv.count("output") ? p.kv["output"] : "out.elf";
    std::string req = p.kv.count("r") ? p.kv["r"] : "";
    if (src.empty()) throw std::runtime_error("missing required flag: --src <file.nl>");

    if (!req.empty()) {
        run_pkg({"-r", req, "--dry-run"});
    }

    std::string temp = make_temp_preprocessed_file();
    try {
        nancli::preprocess_source_file(src, temp);
        std::vector<std::string> cmd = {nancli::resolve_cpp_command_path(), "--compile", temp, "--elf", "--output", out};
        int rc = nancli::run_command(cmd, true);
        fs::remove(temp);
        return rc;
    } catch (...) {
        fs::remove(temp);
        throw;
    }
}

int run_compile_cmd(const std::vector<std::string>& args) {
    int src_idx = find_compile_source_index(args);
    if (src_idx < 0) {
        throw std::runtime_error("missing source file for compile (expected: nanlang compile <src.nl> --output <out.elf>)");
    }

    std::string temp = make_temp_preprocessed_file();
    try {
        nancli::preprocess_source_file(args[static_cast<size_t>(src_idx)], temp);
        std::vector<std::string> patched = args;
        patched[static_cast<size_t>(src_idx)] = temp;

        std::vector<std::string> cmd;
        cmd.push_back(nancli::resolve_cpp_command_path());
        cmd.push_back("compile");
        cmd.insert(cmd.end(), patched.begin(), patched.end());
        int rc = nancli::run_command(cmd, true);
        fs::remove(temp);
        return rc;
    } catch (...) {
        fs::remove(temp);
        throw;
    }
}

int delegate_cpp(const std::string& verb, const std::vector<std::string>& args) {
    std::vector<std::string> cmd;
    cmd.push_back(nancli::resolve_cpp_command_path());
    if (verb == "ui-render") {
        cmd.push_back("--ui-render");
    } else {
        cmd.push_back(verb);
    }
    cmd.insert(cmd.end(), args.begin(), args.end());
    return nancli::run_command(cmd, true);
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            print_help();
            return 0;
        }

        std::string cmd = argv[1];
        std::vector<std::string> args;
        for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);

        if (cmd == "help" || cmd == "-h" || cmd == "--help") {
            print_help();
            return 0;
        }
        if (cmd == "version" || cmd == "-v" || cmd == "--version") {
            std::cout << nancli::full_banner() << "\n";
            return 0;
        }
        if (cmd == "pkg") return run_pkg(args);
        if (cmd == "build") return run_build_cmd(args);
        if (cmd == "compile") return run_compile_cmd(args);
        if (cmd == "run" || cmd == "demo" || cmd == "ui-render") return delegate_cpp(cmd, args);

        print_help();
        throw std::runtime_error("unknown command \"" + cmd + "\"");
    } catch (const std::exception& e) {
        std::cerr << "nanlang: " << e.what() << "\n";
        return 1;
    }
}
