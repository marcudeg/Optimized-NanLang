#pragma once

#include <unordered_map>
#include <string>
#include <vector>

namespace nancli {

constexpr const char* kOriginalityKey =
    "iTjz07jXdTggEDBfmYLK+xrkRgNipuDI63JvIdsQzXB4yES7J/N8wq+xpyFT0dYEGNZ6spRFyJ4=";
constexpr const char* kProjectName = "nanlang";
constexpr const char* kVersion = "4.0.0";

std::string full_banner();

struct PackageEntry {
    std::string name;
    std::string version;
    std::string comment;
    int line = 0;

    std::string to_string() const;
};

struct ReqFile {
    std::string path;
    std::vector<PackageEntry> packages;

    int count() const;
};

ReqFile parse_reqfile(const std::string& path);

struct Config {
    std::string lib_dir;
    std::string cache_dir;
    std::string registry_url;
    std::string compiler;
    std::string build_flags;
    std::string output_dir;
    std::unordered_map<std::string, std::string> raw;

    std::string get(const std::string& key) const;
    std::string dump() const;
};

std::string config_path();
Config load_config(const std::string& path);

struct BuildOptions {
    std::string req_path;
    std::string conf_path;
    bool dry_run = false;
    bool verbose = false;
    std::string target_name;
};

struct BuildResult {
    std::string output_file;
    double duration_seconds = 0.0;
    int packages = 0;
};

BuildResult run_build(const BuildOptions& opts);

std::string resolve_cpp_command_path();
std::string resolve_preprocessor_path();
std::string resolve_raku_path();
void preprocess_source_file(const std::string& source_path, const std::string& out_path);

int run_command(const std::vector<std::string>& args, bool inherit_io = true);
std::string shell_quote(const std::string& in);

} // namespace nancli
