#include "../include/nan_cli_lib.h"

#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <vector>

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

void print_help() {
    std::cout << nancli::full_banner() << "\n\n"
              << "USAGE (Legacy CLI - prefer ./nanlang pkg ...)\n"
              << "  npi -r <req.txt> [flags]\n\n"
              << "FLAGS\n"
              << "  -r <file>          Requirements file (required)\n"
              << "  --build            Full pipeline\n"
              << "  --dry-run          Preview\n"
              << "  --verbose          Verbose\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
        Parsed p = parse_tokens(args);

        if (p.flags.count("help") || p.flags.count("h")) {
            print_help();
            return 0;
        }
        if (p.flags.count("version")) {
            std::cout << nancli::full_banner() << "\n";
            return 0;
        }
        if (p.flags.count("origin")) {
            std::cout << "Project originality key (source-only, never compiled into binary):\n"
                      << " " << nancli::kOriginalityKey << "\n";
            return 0;
        }

        std::string req_path = p.kv.count("r") ? p.kv["r"] : "";
        if (req_path.empty()) {
            print_help();
            throw std::runtime_error("missing required flag: -r <req.txt>");
        }

        std::string conf_path = p.kv.count("config") ? p.kv["config"] : nancli::config_path();
        std::string target = p.kv.count("output") ? p.kv["output"] : "output.elf";
        bool do_build = p.flags.count("build") > 0;
        bool dry_run = p.flags.count("dry-run") > 0;
        bool verbose = p.flags.count("verbose") > 0;

        std::cout << nancli::full_banner() << "\n\n";
        nancli::ReqFile rf = nancli::parse_reqfile(req_path);
        std::cout << "Loaded " << rf.count() << " package(s) from " << rf.path << ":\n";
        for (size_t i = 0; i < rf.packages.size(); ++i) {
            const auto& pkg = rf.packages[i];
            std::string suffix = pkg.comment.empty() ? "" : "  # " + pkg.comment;
            std::cout << "  " << (i + 1) << ". " << pkg.to_string() << suffix << "\n";
        }
        std::cout << "\n";

        if (do_build) {
            nancli::BuildOptions opts;
            opts.req_path = req_path;
            opts.conf_path = conf_path;
            opts.dry_run = dry_run;
            opts.verbose = verbose;
            opts.target_name = target;
            nancli::BuildResult result = nancli::run_build(opts);
            std::cout << "\n[OK] Output: " << result.output_file << "\n";
            std::cout << "  Packages : " << result.packages << "\n";
            std::cout << "  Duration : " << result.duration_seconds << "s\n";
            return 0;
        }

        std::cout << "Run with --build to execute the full build pipeline.\n";
        std::cout << "Use ./nanlang pkg install -r req.txt for unified CLI.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "npi error: " << e.what() << "\n";
        return 1;
    }
}
