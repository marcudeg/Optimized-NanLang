/**
 * NanLang C++ Core (./bin/nanlang_cpp)
 * v4.0.0 AOT-Only Unified Bridge
 */
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include "../include/nan_binary.h"
#include "../include/nan_core.h"
#include "../include/nan_aot_compiler.h"

namespace nanlang {
void run_pulse_emitter_demo(int count, uint8_t pattern);
void run_signal_buffer_demo(int items);
} // namespace nanlang

namespace {

constexpr char MAGIC_BYTES[4] = {'N', 'A', 'N', '3'};
constexpr uint8_t PULSE_PORT = 0x10;
constexpr uint8_t EMIT_PORT  = 0x11;

enum class CompileOutputMode {
    Bytecode,
    Bin,
    Elf,
    Asm,
    Sh,
};

struct Args {
    std::unordered_map<std::string, std::string> kv;
    std::unordered_map<std::string, bool> flags;

    bool has(const std::string& k) const {
        return kv.count(k) || flags.count(k);
    }

    std::string get(const std::string& k, const std::string& def = "") const {
        auto it = kv.find(k);
        return it != kv.end() ? it->second : def;
    }

    int get_int(const std::string& k, int def = 0) const {
        auto it = kv.find(k);
        if (it == kv.end()) return def;
        try { return std::stoi(it->second, nullptr, 0); }
        catch (...) { return def; }
    }

    static Args parse(int argc, char** argv) {
        Args a;
        for (int i = 1; i < argc; ++i) {
            std::string tok(argv[i]);
            if (tok.rfind("--", 0) == 0) {
                std::string key = tok.substr(2);
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    a.kv[key] = argv[++i];
                } else {
                    a.flags[key] = true;
                }
                continue;
            }

            if (tok == "compile" || tok == "run" || tok == "demo") {
                a.kv[tok] = (i + 1 < argc) ? argv[++i] : "true";
                continue;
            }
            if (tok == "help" || tok == "-h") {
                a.flags["help"] = true;
                continue;
            }
            if (tok == "version" || tok == "-v") {
                a.flags["version"] = true;
                continue;
            }
        }
        return a;
    }
};

static std::string trim(std::string s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

static void trim_inplace(std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    if (b == 0 && e == s.size()) return;
    s = s.substr(b, e - b);
}

[[maybe_unused]] static std::string strip_line_comment(const std::string& in) {
    bool in_string = false;
    bool escaped = false;
    for (size_t i = 0; i + 1 < in.size(); ++i) {
        char c = in[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (c == '\\' && in_string) {
            escaped = true;
            continue;
        }
        if (c == '"') {
            in_string = !in_string;
            continue;
        }
        if (!in_string && c == '/' && in[i + 1] == '/') {
            return in.substr(0, i);
        }
    }
    return in;
}

static void strip_line_comment_inplace(std::string& in) {
    bool in_string = false;
    bool escaped = false;
    for (size_t i = 0; i + 1 < in.size(); ++i) {
        char c = in[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (c == '\\' && in_string) {
            escaped = true;
            continue;
        }
        if (c == '"') {
            in_string = !in_string;
            continue;
        }
        if (!in_string && c == '/' && in[i + 1] == '/') {
            in.resize(i);
            return;
        }
    }
}

static void normalize_source_line(std::string& line) {
    strip_line_comment_inplace(line);
    trim_inplace(line);
}

static bool starts_with_keyword(const std::string& line, const std::string& kw) {
    if (line.rfind(kw, 0) != 0) return false;
    if (line.size() == kw.size()) return true;
    unsigned char c = static_cast<unsigned char>(line[kw.size()]);
    return std::isspace(c) || c == ';';
}

static std::string unescape_string(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] != '\\') {
            out.push_back(raw[i]);
            continue;
        }
        if (i + 1 >= raw.size()) {
            out.push_back('\\');
            break;
        }
        char n = raw[++i];
        switch (n) {
            case 'n': out.push_back('\n'); break;
            case 't': out.push_back('\t'); break;
            case 'r': out.push_back('\r'); break;
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '0': out.push_back('\0'); break;
            default:
                out.push_back('\\');
                out.push_back(n);
                break;
        }
    }
    return out;
}

static bool extract_quoted_string(const std::string& line, std::string& out) {
    bool in_string = false;
    bool escaped = false;
    std::string raw;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (!in_string) {
            if (c == '"') {
                in_string = true;
            }
            continue;
        }

        if (escaped) {
            raw.push_back(c);
            escaped = false;
            continue;
        }
        if (c == '\\') {
            raw.push_back(c);
            escaped = true;
            continue;
        }
        if (c == '"') {
            out = unescape_string(raw);
            return true;
        }
        raw.push_back(c);
    }
    return false;
}

[[maybe_unused]] static bool parse_signal_token(const std::string& token, uint8_t& out) {
    if (token == "^") {
        out = 0x01;
        return true;
    }
    if (token == "_") {
        out = 0x00;
        return true;
    }
    if (token == "?") {
        out = 0xFF;
        return true;
    }
    if (token == "_>_") {
        out = 0x10;
        return true;
    }
    if (token == "_>>_") {
        out = 0x11;
        return true;
    }
    return false;
}

static bool parse_signal_token(std::string_view token, uint8_t& out) {
    if (token == "^") {
        out = 0x01;
        return true;
    }
    if (token == "_") {
        out = 0x00;
        return true;
    }
    if (token == "?") {
        out = 0xFF;
        return true;
    }
    if (token == "_>_") {
        out = 0x10;
        return true;
    }
    if (token == "_>>_") {
        out = 0x11;
        return true;
    }
    return false;
}

static bool parse_pulse_or_emit(const std::string& line, bool& is_pulse, uint8_t& signal) {
    std::string_view sv(line);
    const bool pulse = sv.rfind("pulse", 0) == 0;
    const bool emit = sv.rfind("emit", 0) == 0;
    if (!pulse && !emit) return false;

    const size_t kw_len = pulse ? 5u : 4u;
    if (sv.size() <= kw_len) return false;
    unsigned char delim = static_cast<unsigned char>(sv[kw_len]);
    if (!std::isspace(delim)) return false;

    size_t p = kw_len;
    while (p < sv.size() && std::isspace(static_cast<unsigned char>(sv[p]))) ++p;
    const size_t tok_begin = p;
    while (p < sv.size() && !std::isspace(static_cast<unsigned char>(sv[p])) && sv[p] != ';') ++p;
    if (p <= tok_begin) return false;
    std::string_view token = sv.substr(tok_begin, p - tok_begin);

    if (!parse_signal_token(token, signal)) return false;
    is_pulse = pulse;
    return true;
}

static bool emit_store_sequence(std::vector<uint8_t>& out, uint8_t value, uint8_t addr) {
    const uint8_t load = nanlang::NanOpcode::LOAD;
    const uint8_t store = nanlang::NanOpcode::STORE;
    out.push_back(load);
    out.push_back(value);
    out.push_back(store);
    out.push_back(addr);
    return true;
}

static bool parse_int_token(const std::string& token, long long& value) {
    try {
        size_t consumed = 0;
        value = std::stoll(token, &consumed, 0);
        return consumed == token.size();
    } catch (...) {
        return false;
    }
}

static std::string quote_for_log(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    out.push_back('"');
    for (char c : text) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    out.push_back('"');
    return out;
}

static std::vector<std::string> tokenize_ui_directive(const std::string& line, bool& ok) {
    ok = true;
    std::vector<std::string> tokens;
    std::string current;
    bool in_string = false;
    bool escaped = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];

        if (in_string) {
            if (escaped) {
                switch (c) {
                    case 'n': current.push_back('\n'); break;
                    case 'r': current.push_back('\r'); break;
                    case 't': current.push_back('\t'); break;
                    case '"': current.push_back('"'); break;
                    case '\\': current.push_back('\\'); break;
                    default:
                        current.push_back('\\');
                        current.push_back(c);
                        break;
                }
                escaped = false;
                continue;
            }

            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (c == '"') {
                tokens.push_back(current);
                current.clear();
                in_string = false;
                continue;
            }
            current.push_back(c);
            continue;
        }

        if (c == '"') {
            in_string = true;
            continue;
        }
        if (c == ';' || std::isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') {
            break;
        }
        current.push_back(c);
    }

    if (in_string) {
        ok = false;
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

struct NativeProgram {
    std::vector<std::string> writes;
    std::vector<std::string> warning_lines;
    bool has_halt = false;
    size_t emitted_ops = 0;
};

static bool handle_ui_directive(const std::string& line, int line_no, NativeProgram& program) {
    bool ok = true;
    const std::vector<std::string> tokens = tokenize_ui_directive(line, ok);
    if (!ok) {
        program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": unterminated quoted string in ui directive: " + line);
        return true;
    }

    if (tokens.size() < 2 || tokens[0] != "ui") {
        return false;
    }

    auto emit_lines = [&](std::vector<std::string> lines) {
        for (const auto& entry : lines) {
            program.writes.push_back(entry + "\n");
        }
        program.emitted_ops += lines.size();
    };

    auto emit_signal_pair = [&](bool high) {
        if (high) {
            emit_lines({"[pulse] HIGH", "[emit] HIGH"});
        } else {
            emit_lines({"[pulse] LOW", "[emit] LOW"});
        }
    };

    const std::string& group = tokens[1];
    const std::string action = tokens.size() > 2 ? tokens[2] : "";

    if (group == "app") {
        if (action == "begin") {
            emit_lines({"nan_graphics: app begin"});
            emit_signal_pair(true);
            return true;
        }
        if (action == "end") {
            emit_lines({"nan_graphics: app end"});
            emit_signal_pair(false);
            return true;
        }
    }

    if (group == "window") {
        if (action == "create") {
            if (tokens.size() < 6) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": malformed ui window create directive: " + line);
                return true;
            }

            long long window_id = 0;
            long long width = 0;
            long long height = 0;
            if (!parse_int_token(tokens[3], window_id) ||
                !parse_int_token(tokens[4], width) ||
                !parse_int_token(tokens[5], height)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": invalid numeric value in ui window create directive: " + line);
                return true;
            }

            std::ostringstream msg;
            msg << "ui.window.create id=" << window_id
                << " size=" << width << "x" << height;
            emit_lines({msg.str()});
            emit_signal_pair(width > 0 && height > 0);
            return true;
        }

        if (action == "title") {
            if (tokens.size() < 5) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": malformed ui window title directive: " + line);
                return true;
            }

            long long window_id = 0;
            if (!parse_int_token(tokens[3], window_id)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": invalid window id in ui window title directive: " + line);
                return true;
            }

            std::ostringstream msg;
            msg << "ui.window.title id=" << window_id
                << " title=" << quote_for_log(tokens[4]);
            emit_lines({msg.str()});
            return true;
        }
    }

    if (group == "layout") {
        if (action == "vertical" || action == "horizontal") {
            if (tokens.size() < 5) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": malformed ui layout directive: " + line);
                return true;
            }

            long long window_id = 0;
            long long spacing = 0;
            if (!parse_int_token(tokens[3], window_id) || !parse_int_token(tokens[4], spacing)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": invalid numeric value in ui layout directive: " + line);
                return true;
            }

            std::ostringstream msg;
            msg << "ui.layout." << action
                << " window=" << window_id
                << " spacing=" << spacing;
            emit_lines({msg.str()});
            return true;
        }
    }

    if (group == "label" || group == "button") {
        if (tokens.size() < 5) {
            program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                            ": malformed ui " + group + " directive: " + line);
            return true;
        }

        long long window_id = 0;
        long long widget_id = 0;
        if (!parse_int_token(tokens[2], window_id) || !parse_int_token(tokens[3], widget_id)) {
            program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                            ": invalid numeric value in ui " + group + " directive: " + line);
            return true;
        }

        std::ostringstream msg;
        msg << "ui." << group
            << " window=" << window_id
            << " widget=" << widget_id
            << " text=" << quote_for_log(tokens[4]);
        emit_lines({msg.str()});
        return true;
    }

    if (group == "slider") {
        if (tokens.size() < 7) {
            program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                            ": malformed ui slider directive: " + line);
            return true;
        }

        long long window_id = 0;
        long long widget_id = 0;
        long long min_v = 0;
        long long max_v = 0;
        long long current = 0;
        if (!parse_int_token(tokens[2], window_id) ||
            !parse_int_token(tokens[3], widget_id) ||
            !parse_int_token(tokens[4], min_v) ||
            !parse_int_token(tokens[5], max_v) ||
            !parse_int_token(tokens[6], current)) {
            program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                            ": invalid numeric value in ui slider directive: " + line);
            return true;
        }

        std::ostringstream msg;
        msg << "ui.slider window=" << window_id
            << " widget=" << widget_id
            << " range=" << min_v << ".." << max_v
            << " value=" << current;
        emit_lines({msg.str()});
        return true;
    }

    if (group == "chart") {
        if (action == "begin") {
            if (tokens.size() < 6) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": malformed ui chart begin directive: " + line);
                return true;
            }

            long long window_id = 0;
            long long chart_id = 0;
            long long samples = 0;
            if (!parse_int_token(tokens[3], window_id) ||
                !parse_int_token(tokens[4], chart_id) ||
                !parse_int_token(tokens[5], samples)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": invalid numeric value in ui chart begin directive: " + line);
                return true;
            }

            std::ostringstream msg;
            msg << "ui.chart.begin window=" << window_id
                << " chart=" << chart_id
                << " samples=" << samples;
            emit_lines({msg.str()});
            emit_signal_pair(samples > 0);
            return true;
        }

        if (action == "point") {
            if (tokens.size() < 6) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": malformed ui chart point directive: " + line);
                return true;
            }

            long long chart_id = 0;
            long long index = 0;
            long long value = 0;
            if (!parse_int_token(tokens[3], chart_id) ||
                !parse_int_token(tokens[4], index) ||
                !parse_int_token(tokens[5], value)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": invalid numeric value in ui chart point directive: " + line);
                return true;
            }

            std::ostringstream msg;
            msg << "ui.chart.point chart=" << chart_id
                << " index=" << index
                << " value=" << value;
            emit_lines({msg.str()});
            return true;
        }

        if (action == "commit") {
            if (tokens.size() < 4) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": malformed ui chart commit directive: " + line);
                return true;
            }

            long long chart_id = 0;
            if (!parse_int_token(tokens[3], chart_id)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": invalid chart id in ui chart commit directive: " + line);
                return true;
            }

            std::ostringstream msg;
            msg << "ui.chart.commit chart=" << chart_id;
            emit_lines({msg.str()});
            return true;
        }
    }

    if (group == "frame" && action == "present") {
        if (tokens.size() < 4) {
            program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                            ": malformed ui frame present directive: " + line);
            return true;
        }

        long long window_id = 0;
        if (!parse_int_token(tokens[3], window_id)) {
            program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                            ": invalid window id in ui frame present directive: " + line);
            return true;
        }

        std::ostringstream msg;
        msg << "ui.frame.present window=" << window_id;
        emit_lines({msg.str()});
        emit_signal_pair(true);
        return true;
    }

    program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                    ": unsupported ui directive ignored: " + line);
    return true;
}

static bool file_exists_nonempty(const std::string& path) {
    struct stat st {};
    if (stat(path.c_str(), &st) != 0) return false;
    return st.st_size > 0;
}

static int64_t stat_mtime_nsec(const struct stat& st);

static bool output_is_fresh(const std::string& src_path, const std::string& out_path) {
    struct stat src_st {};
    struct stat out_st {};
    if (stat(src_path.c_str(), &src_st) != 0) return false;
    if (stat(out_path.c_str(), &out_st) != 0) return false;
    if (out_st.st_size <= 0) return false;

    const int64_t src_sec = static_cast<int64_t>(src_st.st_mtime);
    const int64_t out_sec = static_cast<int64_t>(out_st.st_mtime);
    if (out_sec > src_sec) return true;
    if (out_sec < src_sec) return false;
    return stat_mtime_nsec(out_st) >= stat_mtime_nsec(src_st);
}

static int64_t stat_mtime_nsec(const struct stat& st) {
#if defined(__APPLE__)
    return static_cast<int64_t>(st.st_mtimespec.tv_nsec);
#elif defined(__linux__)
    return static_cast<int64_t>(st.st_mtim.tv_nsec);
#else
    return 0;
#endif
}

struct SourceFingerprint {
    uint64_t size = 0;
    int64_t mtime_sec = 0;
    int64_t mtime_nsec = 0;
};

static bool read_source_fingerprint(const std::string& path, SourceFingerprint& fp) {
    struct stat st {};
    if (stat(path.c_str(), &st) != 0) return false;
    fp.size = static_cast<uint64_t>(st.st_size);
    fp.mtime_sec = static_cast<int64_t>(st.st_mtime);
    fp.mtime_nsec = stat_mtime_nsec(st);
    return true;
}

static bool compile_cache_enabled() {
    const char* env = std::getenv("NANLANG_DISABLE_CACHE");
    if (!env || !*env) return true;
    return std::strcmp(env, "0") == 0;
}

static bool cache_log_enabled() {
    const char* env = std::getenv("NANLANG_CACHE_LOG");
    if (!env || !*env) return false;
    return std::strcmp(env, "0") != 0;
}

static bool fast_cache_mode_enabled() {
    const char* env = std::getenv("NANLANG_FAST_CACHE");
    if (!env || !*env) return false;
    return std::strcmp(env, "0") != 0;
}

static uint64_t fnv1a64_bytes(uint64_t h, const uint8_t* data, size_t len) {
    constexpr uint64_t kPrime = 1099511628211ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint64_t>(data[i]);
        h *= kPrime;
    }
    return h;
}

static bool hash_file_fnv1a64(const std::string& path, uint64_t& out_hash) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;

    uint64_t h = 1469598103934665603ULL;
    uint8_t buf[16384];
    while (in.good()) {
        in.read(reinterpret_cast<char*>(buf), sizeof(buf));
        const std::streamsize got = in.gcount();
        if (got > 0) {
            h = fnv1a64_bytes(h, buf, static_cast<size_t>(got));
        }
    }
    if (!in.eof() && in.fail()) return false;
    out_hash = h;
    return true;
}

static std::string to_hex_u64(uint64_t v) {
    static const char* hex = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 15; i >= 0; --i) {
        out[static_cast<size_t>(i)] = hex[v & 0x0F];
        v >>= 4;
    }
    return out;
}

static std::string cache_meta_path(const std::string& out_path) {
    return out_path + ".nancache";
}

struct CacheMetaBinary {
    uint32_t magic = 0;
    int32_t mode = 0;
    uint64_t size = 0;
    int64_t mtime_sec = 0;
    int64_t mtime_nsec = 0;
    uint64_t hash = 0;
};

constexpr uint32_t kCacheMetaMagic = 0x4E41434D; // "NACM"

static bool parse_cache_meta_v2(const std::string& line,
                                int& mode,
                                SourceFingerprint& fp,
                                std::string& hash_hex) {
    // Format: v2 <mode> <size> <mtime_sec> <mtime_nsec> <hash_hex>
    std::istringstream iss(line);
    std::string tag;
    if (!(iss >> tag >> mode >> fp.size >> fp.mtime_sec >> fp.mtime_nsec >> hash_hex)) {
        return false;
    }
    return tag == "v2";
}

static bool cache_hit_native(const std::string& src_path,
                             const std::string& out_path,
                             CompileOutputMode mode) {
    if (!file_exists_nonempty(out_path)) return false;

    SourceFingerprint src_fp;
    if (!read_source_fingerprint(src_path, src_fp)) return false;

    const std::string meta_path = cache_meta_path(out_path);
    std::ifstream meta(meta_path, std::ios::binary);
    if (!meta.is_open()) return false;

    CacheMetaBinary bin_meta;
    if (meta.read(reinterpret_cast<char*>(&bin_meta), sizeof(bin_meta))) {
        if (bin_meta.magic == kCacheMetaMagic) {
            return bin_meta.mode == static_cast<int32_t>(mode) &&
                   bin_meta.size == src_fp.size &&
                   bin_meta.mtime_sec == src_fp.mtime_sec &&
                   bin_meta.mtime_nsec == src_fp.mtime_nsec;
        }
    }

    // Fallback parser for old text-based cache metadata.
    meta.clear();
    meta.seekg(0, std::ios::beg);
    std::string line;
    if (!std::getline(meta, line)) return false;

    int meta_mode = -1;
    SourceFingerprint meta_fp;
    std::string meta_hash;
    if (parse_cache_meta_v2(line, meta_mode, meta_fp, meta_hash)) {
        return meta_mode == static_cast<int>(mode) &&
               meta_fp.size == src_fp.size &&
               meta_fp.mtime_sec == src_fp.mtime_sec &&
               meta_fp.mtime_nsec == src_fp.mtime_nsec;
    }

    // Backward compatibility with old format: "<mode>:<hash>"
    const size_t sep = line.find(':');
    if (sep == std::string::npos) return false;
    try {
        const int legacy_mode = std::stoi(line.substr(0, sep));
        if (legacy_mode != static_cast<int>(mode)) return false;
    } catch (...) {
        return false;
    }

    // Legacy format has no cheap fingerprint; verify with hash once.
    uint64_t hash = 0;
    if (!hash_file_fnv1a64(src_path, hash)) return false;
    return line.substr(sep + 1) == to_hex_u64(hash);
}

static void write_native_cache_meta(const std::string& src_path,
                                    const std::string& out_path,
                                    CompileOutputMode mode) {
    SourceFingerprint fp;
    if (!read_source_fingerprint(src_path, fp)) return;
    uint64_t hash = 0;
    if (!hash_file_fnv1a64(src_path, hash)) return;
    std::ofstream meta(cache_meta_path(out_path), std::ios::binary | std::ios::trunc);
    if (!meta.is_open()) return;

    CacheMetaBinary bin_meta;
    bin_meta.magic = kCacheMetaMagic;
    bin_meta.mode = static_cast<int32_t>(mode);
    bin_meta.size = fp.size;
    bin_meta.mtime_sec = fp.mtime_sec;
    bin_meta.mtime_nsec = fp.mtime_nsec;
    bin_meta.hash = hash;
    meta.write(reinterpret_cast<const char*>(&bin_meta), sizeof(bin_meta));
}

static bool has_nan3_magic(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    char magic[4] = {};
    if (!in.read(magic, sizeof(magic))) return false;
    return std::memcmp(magic, MAGIC_BYTES, sizeof(MAGIC_BYTES)) == 0;
}

static int decode_wait_status(int status) {
    if (status < 0) return 1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

static int run_native_file(const std::string& path) {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[RT] fork failed while running: " << path << "\n";
        return 1;
    }
    if (pid == 0) {
        execl(path.c_str(), path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        std::cerr << "[RT] waitpid failed while running: " << path << "\n";
        return 1;
    }
    return decode_wait_status(status);
}

static std::string signal_name(uint8_t v);

struct NativePayload {
    std::vector<uint8_t> machine_code;
    std::string assembly;
};

static void append_u16_le(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

static void append_u32_le(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

static void append_u64_le(std::vector<uint8_t>& out, uint64_t v) {
    append_u32_le(out, static_cast<uint32_t>(v & 0xFFFFFFFFULL));
    append_u32_le(out, static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFULL));
}

static void append_bytes(std::vector<uint8_t>& out, std::initializer_list<uint8_t> data) {
    out.insert(out.end(), data.begin(), data.end());
}

static void emit_mov_reg_imm32(std::vector<uint8_t>& code, uint8_t reg_opcode, uint32_t imm) {
    append_bytes(code, {0x48, 0xC7, reg_opcode});
    append_u32_le(code, imm);
}

static size_t emit_lea_rsi_rip_placeholder(std::vector<uint8_t>& code) {
    append_bytes(code, {0x48, 0x8D, 0x35});
    const size_t disp_offset = code.size();
    append_u32_le(code, 0);
    return disp_offset;
}

static void patch_i32_le(std::vector<uint8_t>& code, size_t offset, int32_t value) {
    code[offset + 0] = static_cast<uint8_t>(value & 0xFF);
    code[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    code[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    code[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

static void emit_syscall(std::vector<uint8_t>& code) {
    append_bytes(code, {0x0F, 0x05});
}

static std::string to_hex_byte(uint8_t b) {
    static const char* kHex = "0123456789ABCDEF";
    std::string s = "0x00";
    s[2] = kHex[(b >> 4) & 0x0F];
    s[3] = kHex[b & 0x0F];
    return s;
}

static std::string as_db_line(const std::string& label, const std::string& text) {
    std::ostringstream line;
    line << label << ": db ";
    if (text.empty()) {
        line << "0x00";
        return line.str();
    }
    for (size_t i = 0; i < text.size(); ++i) {
        if (i != 0) line << ", ";
        line << to_hex_byte(static_cast<uint8_t>(text[i]));
    }
    return line.str();
}

static bool parse_native_program(const std::string& src_path, NativeProgram& program) {
    std::ifstream src(src_path);
    if (!src.is_open()) {
        std::cerr << "[ERROR] Source not found: " << src_path << "\n";
        return false;
    }

    std::string raw_line;
    int line_no = 0;
    bool warned_if = false;
    bool warned_loop = false;
    bool warned_return = false;

    while (std::getline(src, raw_line)) {
        ++line_no;
        std::string line = raw_line;
        normalize_source_line(line);
        if (line.empty()) continue;

        if (line == "{" || line == "}" || line == "};") continue;
        if (!line.empty() && line.front() == '}') continue;

        if (starts_with_keyword(line, "if") || starts_with_keyword(line, "else")) {
            if (!warned_if) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": conditional flow is currently linearized by the native emitter.");
                warned_if = true;
            }
            continue;
        }
        if (starts_with_keyword(line, "loop") || starts_with_keyword(line, "break")) {
            if (!warned_loop) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": loop flow is currently linearized by the native emitter.");
                warned_loop = true;
            }
            continue;
        }
        if (starts_with_keyword(line, "return")) {
            if (!warned_return) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": return statements are ignored in native emitter mode.");
                warned_return = true;
            }
            continue;
        }

        if (starts_with_keyword(line, "fn") ||
            starts_with_keyword(line, "let") ||
            starts_with_keyword(line, "const") ||
            starts_with_keyword(line, "on") ||
            starts_with_keyword(line, "interrupt") ||
            starts_with_keyword(line, "purge") ||
            starts_with_keyword(line, "jitter")) {
            continue;
        }

        if (starts_with_keyword(line, "say")) {
            std::string text;
            if (!extract_quoted_string(line, text)) {
                std::cerr << "[ERROR] line " << line_no << ": malformed say string literal\n";
                return false;
            }
            program.writes.push_back(text + "\n");
            program.emitted_ops += 1;
            continue;
        }

        if (starts_with_keyword(line, "ui")) {
            if (!handle_ui_directive(line, line_no, program)) {
                program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                                ": unsupported ui directive ignored: " + line);
            }
            continue;
        }

        if (starts_with_keyword(line, "pulse") || starts_with_keyword(line, "emit")) {
            bool is_pulse = false;
            uint8_t signal = 0;
            if (!parse_pulse_or_emit(line, is_pulse, signal)) {
                std::cerr << "[ERROR] line " << line_no << ": malformed pulse/emit statement\n";
                return false;
            }
            std::ostringstream msg;
            msg << "[" << (is_pulse ? "pulse" : "emit") << "] " << signal_name(signal) << "\n";
            program.writes.push_back(msg.str());
            program.emitted_ops += 2;
            continue;
        }

        if (starts_with_keyword(line, "halt")) {
            program.has_halt = true;
            program.emitted_ops += 1;
            continue;
        }

        if (!line.empty() && line.front() == '@') {
            std::ostringstream msg;
            msg << "[mmio] " << line << "\n";
            program.writes.push_back(msg.str());
            program.emitted_ops += 1;
            continue;
        }

        if (starts_with_keyword(line, "@macro") || starts_with_keyword(line, "NAN3") ||
            starts_with_keyword(line, "@nan_magic")) {
            continue;
        }

        program.warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": unsupported statement ignored: " + line);
    }

    if (!program.has_halt || program.emitted_ops == 0) {
        program.has_halt = true;
        program.emitted_ops += 1;
    }

    return true;
}

static NativePayload emit_native_payload(const NativeProgram& program) {
    NativePayload payload;
    std::vector<uint8_t>& code = payload.machine_code;
    std::vector<uint8_t> rodata;
    std::vector<size_t> disp_patch_offsets;
    std::vector<size_t> rodata_offsets;

    std::ostringstream asm_text;
    asm_text << "global _start\n";
    asm_text << "section .text\n";
    asm_text << "_start:\n";

    for (size_t i = 0; i < program.writes.size(); ++i) {
        const std::string label = "msg_" + std::to_string(i);
        const std::string& text = program.writes[i];
        const size_t ro_off = rodata.size();
        rodata_offsets.push_back(ro_off);
        rodata.insert(rodata.end(), text.begin(), text.end());

        emit_mov_reg_imm32(code, 0xC0, 1); // mov rax, 1 (sys_write)
        emit_mov_reg_imm32(code, 0xC7, 1); // mov rdi, 1 (stdout)
        const size_t disp_offset = emit_lea_rsi_rip_placeholder(code);
        disp_patch_offsets.push_back(disp_offset);
        emit_mov_reg_imm32(code, 0xC2, static_cast<uint32_t>(text.size())); // mov rdx, len
        emit_syscall(code);

        asm_text << "    mov rax, 1\n";
        asm_text << "    mov rdi, 1\n";
        asm_text << "    lea rsi, [rel " << label << "]\n";
        asm_text << "    mov rdx, " << text.size() << "\n";
        asm_text << "    syscall\n";
    }

    emit_mov_reg_imm32(code, 0xC0, 60); // mov rax, 60 (sys_exit)
    append_bytes(code, {0x48, 0x31, 0xFF}); // xor rdi, rdi
    emit_syscall(code);

    asm_text << "    mov rax, 60\n";
    asm_text << "    xor rdi, rdi\n";
    asm_text << "    syscall\n";
    asm_text << "section .rodata\n";

    const size_t text_size = code.size();
    for (size_t i = 0; i < disp_patch_offsets.size(); ++i) {
        const int64_t target = static_cast<int64_t>(text_size + rodata_offsets[i]);
        const int64_t next_ip = static_cast<int64_t>(disp_patch_offsets[i] + 4);
        const int64_t delta = target - next_ip;
        patch_i32_le(code, disp_patch_offsets[i], static_cast<int32_t>(delta));
        asm_text << "    " << as_db_line("msg_" + std::to_string(i), program.writes[i]) << "\n";
    }

    code.insert(code.end(), rodata.begin(), rodata.end());
    payload.assembly = asm_text.str();
    return payload;
}

static std::vector<uint8_t> build_elf_image(const std::vector<uint8_t>& machine_code) {
    std::vector<uint8_t> out;
    const uint64_t image_base = 0x400000ULL;
    const uint64_t ehdr_size = 64;
    const uint64_t phdr_size = 56;
    const uint64_t code_offset = ehdr_size + phdr_size;
    const uint64_t file_size = code_offset + machine_code.size();

    out.reserve(static_cast<size_t>(file_size));

    // ELF64 header.
    out.push_back(0x7F);
    out.push_back('E');
    out.push_back('L');
    out.push_back('F');
    out.push_back(2); // ELFCLASS64
    out.push_back(1); // ELFDATA2LSB
    out.push_back(1); // EV_CURRENT
    out.push_back(0); // System V ABI
    for (int i = 0; i < 8; ++i) out.push_back(0);
    append_u16_le(out, 2);             // ET_EXEC
    append_u16_le(out, 0x3E);          // EM_X86_64
    append_u32_le(out, 1);             // EV_CURRENT
    append_u64_le(out, image_base + code_offset); // entry
    append_u64_le(out, ehdr_size);     // e_phoff
    append_u64_le(out, 0);             // e_shoff
    append_u32_le(out, 0);             // flags
    append_u16_le(out, static_cast<uint16_t>(ehdr_size));
    append_u16_le(out, static_cast<uint16_t>(phdr_size));
    append_u16_le(out, 1);             // phnum
    append_u16_le(out, 0);             // shentsize
    append_u16_le(out, 0);             // shnum
    append_u16_le(out, 0);             // shstrndx

    // Single PT_LOAD header.
    append_u32_le(out, 1);             // PT_LOAD
    append_u32_le(out, 5);             // PF_R | PF_X
    append_u64_le(out, 0);             // p_offset
    append_u64_le(out, image_base);    // p_vaddr
    append_u64_le(out, image_base);    // p_paddr
    append_u64_le(out, file_size);     // p_filesz
    append_u64_le(out, file_size);     // p_memsz
    append_u64_le(out, 0x1000);        // p_align

    out.insert(out.end(), machine_code.begin(), machine_code.end());
    return out;
}

static bool write_binary_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return out.good();
}

static std::string base64_encode(const std::vector<uint8_t>& data) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        const uint32_t a = data[i];
        const uint32_t b = (i + 1 < data.size()) ? data[i + 1] : 0;
        const uint32_t c = (i + 2 < data.size()) ? data[i + 2] : 0;
        const uint32_t triple = (a << 16) | (b << 8) | c;

        out.push_back(table[(triple >> 18) & 0x3F]);
        out.push_back(table[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < data.size()) ? table[(triple >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < data.size()) ? table[triple & 0x3F] : '=');
    }
    return out;
}

static std::string wrap_base64_lines(const std::string& b64, size_t width) {
    if (width == 0) return b64;
    std::ostringstream out;
    for (size_t i = 0; i < b64.size(); i += width) {
        out << b64.substr(i, width) << "\n";
    }
    return out.str();
}

static bool chmod_exec(const std::string& path) {
    return chmod(path.c_str(), 0755) == 0;
}

static bool read_text_file(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

static bool source_contains_ui_directives(const std::string& path) {
    std::ifstream src(path);
    if (!src.is_open()) return false;

    std::string raw_line;
    while (std::getline(src, raw_line)) {
        std::string line = raw_line;
        normalize_source_line(line);
        if (line.empty()) continue;
        if (starts_with_keyword(line, "ui")) return true;
    }
    return false;
}

static bool write_ui_launcher_script(const std::string& src_path, const std::string& out_path) {
    std::string scene_text;
    if (!read_text_file(src_path, scene_text)) {
        std::cerr << "[ERROR] Could not read UI scene source: " << src_path << "\n";
        return false;
    }

    std::vector<uint8_t> bytes(scene_text.begin(), scene_text.end());
    const std::string b64 = wrap_base64_lines(base64_encode(bytes), 76);

    std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Could not create output file: " << out_path << "\n";
        return false;
    }

    out << "#!/usr/bin/env bash\n"
        << "set -euo pipefail\n"
        << "tmp_scene=\"$(mktemp /tmp/nanlang_ui_scene.XXXXXX.nl)\"\n"
        << "cleanup() { rm -f \"$tmp_scene\"; }\n"
        << "trap cleanup EXIT\n"
        << "base64 -d > \"$tmp_scene\" <<'NANLANG_UI_SCENE_B64'\n"
        << b64
        << "NANLANG_UI_SCENE_B64\n"
        << "if [ -x ./bin/nanlang ]; then\n"
        << "  exec ./bin/nanlang ui-render \"$tmp_scene\"\n"
        << "elif command -v nanlang >/dev/null 2>&1; then\n"
        << "  exec nanlang ui-render \"$tmp_scene\"\n"
        << "else\n"
        << "  echo 'nanlang: GUI renderer not found in PATH or ./bin' >&2\n"
        << "  exit 127\n"
        << "fi\n";

    if (!out.good()) {
        std::cerr << "[ERROR] failed while writing UI launcher: " << out_path << "\n";
        return false;
    }

    return chmod_exec(out_path);
}

[[maybe_unused]] static bool compile_native_source(
    const std::string& src_path,
    const std::string& out_path,
    CompileOutputMode mode
) {
    if (compile_cache_enabled()) {
        const bool fast_hit = fast_cache_mode_enabled() && output_is_fresh(src_path, out_path);
        const bool strict_hit = (!fast_hit) && cache_hit_native(src_path, out_path, mode);
        if (fast_hit || strict_hit) {
            if (cache_log_enabled()) {
                std::cout << "[OK] Cached compile hit: " << src_path << " -> " << out_path << "\n";
            }
            return true;
        }
    }

    NativeProgram program;
    if (!parse_native_program(src_path, program)) {
        return false;
    }
    const bool ui_scene = source_contains_ui_directives(src_path);
    NativePayload payload = emit_native_payload(program);

    bool ok = false;
    switch (mode) {
        case CompileOutputMode::Asm: {
            std::ofstream out(out_path, std::ios::binary);
            if (!out.is_open()) {
                std::cerr << "[ERROR] Could not create output file: " << out_path << "\n";
                return false;
            }
            out << payload.assembly;
            ok = out.good();
            break;
        }
        case CompileOutputMode::Bin:
            ok = write_binary_file(out_path, payload.machine_code);
            break;
        case CompileOutputMode::Elf: {
            if (ui_scene) {
                ok = write_ui_launcher_script(src_path, out_path);
            } else {
                std::vector<uint8_t> elf = build_elf_image(payload.machine_code);
                ok = write_binary_file(out_path, elf);
                if (ok) chmod_exec(out_path);
            }
            break;
        }
        case CompileOutputMode::Sh: {
            if (ui_scene) {
                ok = write_ui_launcher_script(src_path, out_path);
            } else {
                std::vector<uint8_t> elf = build_elf_image(payload.machine_code);
                const std::string b64 = wrap_base64_lines(base64_encode(elf), 76);
                std::ofstream out(out_path, std::ios::binary);
                if (!out.is_open()) {
                    std::cerr << "[ERROR] Could not create output file: " << out_path << "\n";
                    return false;
                }
                out << "#!/usr/bin/env bash\n"
                    << "set -euo pipefail\n"
                    << "tmp_file=\"$(mktemp /tmp/nanlang_exec.XXXXXX)\"\n"
                    << "cleanup() { rm -f \"$tmp_file\"; }\n"
                    << "trap cleanup EXIT\n"
                    << "base64 -d > \"$tmp_file\" <<'NANLANG_B64'\n"
                    << b64
                    << "NANLANG_B64\n"
                    << "chmod +x \"$tmp_file\"\n"
                    << "exec \"$tmp_file\" \"$@\"\n";
                ok = out.good();
                if (ok) chmod_exec(out_path);
            }
            break;
        }
        case CompileOutputMode::Bytecode:
            ok = false;
            break;
    }

    if (!ok) {
        std::cerr << "[ERROR] failed while writing output file: " << out_path << "\n";
        return false;
    }

    for (const auto& w : program.warning_lines) {
        std::cerr << w << "\n";
    }

    const char* mode_name = "bytecode";
    switch (mode) {
        case CompileOutputMode::Asm: mode_name = "asm"; break;
        case CompileOutputMode::Bin: mode_name = "bin"; break;
        case CompileOutputMode::Elf: mode_name = ui_scene ? "ui-launcher" : "elf"; break;
        case CompileOutputMode::Sh: mode_name = ui_scene ? "ui-launcher" : "sh"; break;
        case CompileOutputMode::Bytecode: break;
    }
    std::cout << "[OK] Compiled " << src_path << " -> " << out_path
              << " [mode=" << mode_name << "]"
              << " (ops=" << program.emitted_ops
              << ", warnings=" << program.warning_lines.size() << ")\n";
    if (compile_cache_enabled()) {
        write_native_cache_meta(src_path, out_path, mode);
    }
    return true;
}

[[maybe_unused]] static CompileOutputMode resolve_compile_mode(const Args& args, bool& valid, std::string& err) {
    int selected = 0;
    CompileOutputMode mode = CompileOutputMode::Elf;

    if (args.has("bin")) {
        mode = CompileOutputMode::Bin;
        selected++;
    }
    if (args.has("elf")) {
        mode = CompileOutputMode::Elf;
        selected++;
    }
    if (args.has("asm")) {
        mode = CompileOutputMode::Asm;
        selected++;
    }
    if (args.has("sh")) {
        mode = CompileOutputMode::Sh;
        selected++;
    }

    if (selected > 1) {
        valid = false;
        err = "choose only one output mode: --bin, --elf, --asm or --sh";
        return CompileOutputMode::Elf;
    }

    valid = true;
    err.clear();
    return mode;
}

[[maybe_unused]] static bool compile_source(const std::string& src_path, const std::string& out_path) {
    std::ifstream src(src_path);
    if (!src.is_open()) {
        std::cerr << "[ERROR] Source not found: " << src_path << "\n";
        return false;
    }

    std::vector<uint8_t> bytecode;
    struct stat src_st {};
    if (stat(src_path.c_str(), &src_st) == 0 && src_st.st_size > 0) {
        bytecode.reserve(static_cast<size_t>(src_st.st_size) + 16);
    } else {
        bytecode.reserve(4096);
    }

    bytecode.insert(bytecode.end(), MAGIC_BYTES, MAGIC_BYTES + sizeof(MAGIC_BYTES));

    std::string raw_line;
    int line_no = 0;
    size_t emitted_ops = 0;
    std::vector<std::string> warning_lines;
    bool has_halt = false;
    bool warned_if = false;
    bool warned_loop = false;
    bool warned_return = false;

    while (std::getline(src, raw_line)) {
        ++line_no;
        std::string line = raw_line;
        normalize_source_line(line);
        if (line.empty()) continue;

        if (line == "{" || line == "}" || line == "};") continue;
        if (!line.empty() && line.front() == '}') continue;
        if (starts_with_keyword(line, "if") || starts_with_keyword(line, "else")) {
            if (!warned_if) {
                warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": conditional flow is currently linearized by the minimal compiler.");
                warned_if = true;
            }
            continue;
        }
        if (starts_with_keyword(line, "loop") || starts_with_keyword(line, "break")) {
            if (!warned_loop) {
                warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": loop flow is currently linearized by the minimal compiler.");
                warned_loop = true;
            }
            continue;
        }
        if (starts_with_keyword(line, "return")) {
            if (!warned_return) {
                warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": return statements are ignored in bytecode stub mode.");
                warned_return = true;
            }
            continue;
        }
        if (starts_with_keyword(line, "fn") ||
            starts_with_keyword(line, "let") ||
            starts_with_keyword(line, "const") ||
            starts_with_keyword(line, "on") ||
            starts_with_keyword(line, "interrupt") ||
            starts_with_keyword(line, "purge") ||
            starts_with_keyword(line, "jitter")) {
            continue;
        }

        if (starts_with_keyword(line, "say")) {
            std::string text;
            if (!extract_quoted_string(line, text)) {
                std::cerr << "[ERROR] line " << line_no << ": malformed say string literal\n";
                return false;
            }
            const uint8_t op = nanlang::NanOpcode::SAY;
            const uint32_t len = static_cast<uint32_t>(text.size());
            bytecode.push_back(op);
            append_u32_le(bytecode, len);
            bytecode.insert(bytecode.end(), text.begin(), text.end());
            emitted_ops += 1;
            continue;
        }

        if (starts_with_keyword(line, "halt")) {
            const uint8_t op = nanlang::NanOpcode::HALT;
            bytecode.push_back(op);
            emitted_ops += 1;
            has_halt = true;
            continue;
        }

        if (starts_with_keyword(line, "pulse") || starts_with_keyword(line, "emit")) {
            bool is_pulse = false;
            uint8_t signal = 0;
            if (!parse_pulse_or_emit(line, is_pulse, signal)) {
                std::cerr << "[ERROR] line " << line_no << ": malformed pulse/emit statement\n";
                return false;
            }
            const uint8_t addr = is_pulse ? PULSE_PORT : EMIT_PORT;
            emit_store_sequence(bytecode, signal, addr);
            emitted_ops += 2; // LOAD + STORE
            continue;
        }

        if (!line.empty() && line.front() == '@') {
            size_t eq = line.find('=');
            if (eq == std::string::npos) {
                warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": malformed MMIO write ignored: " + line);
                continue;
            }
            std::string addr_tok = trim(line.substr(1, eq - 1));
            std::string rhs = trim(line.substr(eq + 1));
            if (!rhs.empty() && rhs.back() == ';') rhs.pop_back();
            rhs = trim(rhs);
            try {
                unsigned long addr = std::stoul(addr_tok, nullptr, 0);
                unsigned long value = std::stoul(rhs, nullptr, 0);
                if (addr > 0xFF || value > 0xFF) {
                    std::cerr << "[ERROR] line " << line_no
                              << ": MMIO address/value must fit in uint8_t\n";
                    return false;
                }
                emit_store_sequence(bytecode,
                                    static_cast<uint8_t>(value),
                                    static_cast<uint8_t>(addr));
                emitted_ops += 2;
            } catch (...) {
                warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                        ": unsupported MMIO expression ignored: " + line);
            }
            continue;
        }

        if (starts_with_keyword(line, "@macro") || starts_with_keyword(line, "NAN3") ||
            starts_with_keyword(line, "@nan_magic")) {
            continue;
        }

        warning_lines.push_back("[WARN] line " + std::to_string(line_no) +
                                ": unsupported statement ignored: " + line);
    }

    if (emitted_ops == 0 || !has_halt) {
        const uint8_t halt = nanlang::NanOpcode::HALT;
        bytecode.push_back(halt);
        emitted_ops += 1;
    }

    std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Could not create output file: " << out_path << "\n";
        return false;
    }
    out.write(reinterpret_cast<const char*>(bytecode.data()), static_cast<std::streamsize>(bytecode.size()));
    if (!out.good()) {
        std::cerr << "[ERROR] failed while finalizing output file: " << out_path << "\n";
        return false;
    }

    for (const auto& w : warning_lines) {
        std::cerr << w << "\n";
    }
    std::cout << "[OK] Compiled " << src_path << " -> " << out_path
              << " (ops=" << emitted_ops << ", warnings=" << warning_lines.size() << ")\n";
    return true;
}

static inline uint32_t read_u32_le_ptr(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

static std::string signal_name(uint8_t v) {
    switch (v) {
        case 0x00: return "LOW";
        case 0x01: return "HIGH";
        case 0xFF: return "UNCERTAIN";
        case 0x10: return "FALLING";
        case 0x11: return "DOUBLE_FALLING";
        default: {
            static const char* hex = "0123456789abcdef";
            std::string out = "0x00";
            out[2] = hex[(v >> 4) & 0x0F];
            out[3] = hex[v & 0x0F];
            return out;
        }
    }
}

[[maybe_unused]] static bool run_bytecode(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        std::cerr << "[ERROR] Bytecode not found: " << path << "\n";
        return false;
    }

    const std::streamsize fsize = in.tellg();
    if (fsize < 4) {
        std::cerr << "[ERROR] Bytecode is truncated: " << path << "\n";
        return false;
    }

    std::vector<uint8_t> bytes(static_cast<size_t>(fsize));
    in.seekg(0, std::ios::beg);
    if (!in.read(reinterpret_cast<char*>(bytes.data()), fsize)) {
        std::cerr << "[ERROR] Bytecode read failed: " << path << "\n";
        return false;
    }

    if (std::memcmp(bytes.data(), MAGIC_BYTES, sizeof(MAGIC_BYTES)) != 0) {
        std::cerr << "[RT] Invalid magic. Expected NAN3.\n";
        return false;
    }

    uint8_t acc = 0;
    const uint8_t* const base = bytes.data();
    const uint8_t* p = base + sizeof(MAGIC_BYTES);
    const uint8_t* const end = base + bytes.size();
    bool halted = false;

    while (p < end) {
        const uint8_t op = *p++;
        const size_t op_off = static_cast<size_t>((p - base) - 1);

        switch (op) {
            case nanlang::NanOpcode::HALT:
                halted = true;
                break;

            case nanlang::NanOpcode::SAY: {
                if (end - p < 4) {
                    std::cerr << "[RT] Truncated SAY length at offset " << op_off << "\n";
                    return false;
                }
                const uint32_t len = read_u32_le_ptr(p);
                p += 4;
                if (static_cast<size_t>(end - p) < len) {
                    std::cerr << "[RT] Truncated SAY payload at offset "
                              << static_cast<size_t>(p - base) << "\n";
                    return false;
                }
                std::cout.write(reinterpret_cast<const char*>(p), static_cast<std::streamsize>(len));
                std::cout.put('\n');
                p += len;
                break;
            }

            case nanlang::NanOpcode::LOAD: {
                if (p >= end) {
                    std::cerr << "[RT] Truncated LOAD immediate at offset " << op_off << "\n";
                    return false;
                }
                acc = *p++;
                break;
            }

            case nanlang::NanOpcode::STORE: {
                if (p >= end) {
                    std::cerr << "[RT] Truncated STORE address at offset " << op_off << "\n";
                    return false;
                }
                const uint8_t addr = *p++;
                if (addr == PULSE_PORT) {
                    std::cout << "[pulse] " << signal_name(acc) << "\n";
                } else if (addr == EMIT_PORT) {
                    std::cout << "[emit] " << signal_name(acc) << "\n";
                }
                break;
            }

            default:
                std::cerr << "[RT] Unsupported opcode 0x" << std::hex << std::setw(2)
                          << std::setfill('0') << static_cast<int>(op) << std::dec
                          << " at offset " << op_off << "\n";
                return false;
        }

        if (halted) break;
    }

    if (!halted) {
        std::cerr << "[RT] Program reached EOF without HALT.\n";
    }
    return true;
}

struct UiWidget {
    enum class Kind {
        Label,
        Button,
        Slider,
    };

    Kind kind = Kind::Label;
    int window_id = 0;
    int widget_id = 0;
    std::string text;
    int min_value = 0;
    int max_value = 100;
    int current_value = 0;
};

struct UiChart {
    bool present = false;
    int window_id = 0;
    int chart_id = 0;
    int samples = 0;
    std::vector<int> values;
};

struct UiScene {
    bool has_magic = false;
    bool has_ui = false;
    bool app_begin = false;
    bool app_end = false;
    bool horizontal_layout = false;
    int layout_spacing = 12;
    int window_id = 1;
    int width = 1280;
    int height = 720;
    std::string title = "NanLang UI";
    std::vector<UiWidget> widgets;
    UiChart chart;
    std::vector<std::string> notes;
    
    // Interaction state
    int dragging_slider_id = -1;
    int hovered_widget_id = -1;
    int clicked_button_id = -1;
    double last_mouse_x = -1.0;
    double last_mouse_y = -1.0;
    GtkWidget* drawing_area = nullptr;
};

static std::string widget_kind_name(UiWidget::Kind kind) {
    switch (kind) {
        case UiWidget::Kind::Label: return "Label";
        case UiWidget::Kind::Button: return "Button";
        case UiWidget::Kind::Slider: return "Slider";
    }
    return "Widget";
}

static bool parse_ui_scene(const std::string& src_path, UiScene& scene) {
    std::ifstream src(src_path);
    if (!src.is_open()) {
        std::cerr << "[ERROR] UI scene not found: " << src_path << "\n";
        return false;
    }

    std::string raw_line;
    int line_no = 0;
    bool saw_ui = false;

    while (std::getline(src, raw_line)) {
        ++line_no;
        std::string line = raw_line;
        normalize_source_line(line);
        if (line.empty()) continue;

        if (line == "{" || line == "}" || line == "};") continue;
        if (!line.empty() && line.front() == '}') continue;

        if (starts_with_keyword(line, "NAN3") || starts_with_keyword(line, "@nan_magic")) {
            scene.has_magic = true;
            continue;
        }

        if (!starts_with_keyword(line, "ui")) {
            continue;
        }

        bool ok = true;
        const std::vector<std::string> tokens = tokenize_ui_directive(line, ok);
        if (!ok || tokens.size() < 2) {
            scene.notes.push_back("line " + std::to_string(line_no) + ": malformed ui directive");
            continue;
        }

        saw_ui = true;
        scene.has_ui = true;

        const std::string& group = tokens[1];
        const std::string action = tokens.size() > 2 ? tokens[2] : "";

        auto parse_i = [&](size_t idx, long long& out) -> bool {
            return idx < tokens.size() && parse_int_token(tokens[idx], out);
        };

        if (group == "app") {
            if (action == "begin") {
                scene.app_begin = true;
                scene.notes.push_back("app begin");
            } else if (action == "end") {
                scene.app_end = true;
                scene.notes.push_back("app end");
            }
            continue;
        }

        if (group == "window") {
            if (action == "create") {
                if (tokens.size() < 6) {
                    scene.notes.push_back("line " + std::to_string(line_no) + ": window create missing args");
                    continue;
                }
                long long window_id = 0;
                long long width = 0;
                long long height = 0;
                if (parse_i(3, window_id) && parse_i(4, width) && parse_i(5, height)) {
                    scene.window_id = static_cast<int>(window_id);
                    scene.width = std::max(320, static_cast<int>(width));
                    scene.height = std::max(240, static_cast<int>(height));
                }
            } else if (action == "title") {
                if (tokens.size() < 5) {
                    scene.notes.push_back("line " + std::to_string(line_no) + ": window title missing args");
                    continue;
                }
                scene.title = tokens[4];
            }
            continue;
        }

        if (group == "layout") {
            if (action == "vertical" || action == "horizontal") {
                if (tokens.size() < 5) {
                    scene.notes.push_back("line " + std::to_string(line_no) + ": layout missing args");
                    continue;
                }
                long long window_id = 0;
                long long spacing = 0;
                if (parse_i(3, window_id) && parse_i(4, spacing)) {
                    scene.horizontal_layout = (action == "horizontal");
                    scene.layout_spacing = static_cast<int>(spacing);
                }
            }
            continue;
        }

        if (group == "label" || group == "button") {
            if (tokens.size() < 5) {
                scene.notes.push_back("line " + std::to_string(line_no) + ": " + group + " missing args");
                continue;
            }
            long long window_id = 0;
            long long widget_id = 0;
            if (!parse_i(2, window_id) || !parse_i(3, widget_id)) {
                scene.notes.push_back("line " + std::to_string(line_no) + ": invalid " + group + " id");
                continue;
            }
            UiWidget widget;
            widget.kind = (group == "label") ? UiWidget::Kind::Label : UiWidget::Kind::Button;
            widget.window_id = static_cast<int>(window_id);
            widget.widget_id = static_cast<int>(widget_id);
            widget.text = tokens[4];
            scene.widgets.push_back(widget);
            continue;
        }

        if (group == "slider") {
            if (tokens.size() < 7) {
                scene.notes.push_back("line " + std::to_string(line_no) + ": slider missing args");
                continue;
            }
            long long window_id = 0;
            long long widget_id = 0;
            long long min_v = 0;
            long long max_v = 0;
            long long current = 0;
            if (!parse_i(2, window_id) || !parse_i(3, widget_id) ||
                !parse_i(4, min_v) || !parse_i(5, max_v) || !parse_i(6, current)) {
                scene.notes.push_back("line " + std::to_string(line_no) + ": invalid slider args");
                continue;
            }
            UiWidget widget;
            widget.kind = UiWidget::Kind::Slider;
            widget.window_id = static_cast<int>(window_id);
            widget.widget_id = static_cast<int>(widget_id);
            widget.min_value = static_cast<int>(min_v);
            widget.max_value = static_cast<int>(max_v);
            widget.current_value = static_cast<int>(current);
            scene.widgets.push_back(widget);
            continue;
        }

        if (group == "chart") {
            if (action == "begin") {
                if (tokens.size() < 6) {
                    scene.notes.push_back("line " + std::to_string(line_no) + ": chart begin missing args");
                    continue;
                }
                long long window_id = 0;
                long long chart_id = 0;
                long long samples = 0;
                if (parse_i(3, window_id) && parse_i(4, chart_id) && parse_i(5, samples)) {
                    scene.chart.present = true;
                    scene.chart.window_id = static_cast<int>(window_id);
                    scene.chart.chart_id = static_cast<int>(chart_id);
                    scene.chart.samples = static_cast<int>(samples);
                    scene.chart.values.clear();
                    scene.chart.values.reserve(std::max(0, static_cast<int>(samples)));
                }
            } else if (action == "point") {
                if (tokens.size() < 6) {
                    scene.notes.push_back("line " + std::to_string(line_no) + ": chart point missing args");
                    continue;
                }
                long long chart_id = 0;
                long long index = 0;
                long long value = 0;
                if (parse_i(3, chart_id) && parse_i(4, index) && parse_i(5, value)) {
                    if (!scene.chart.present) {
                        scene.chart.present = true;
                        scene.chart.chart_id = static_cast<int>(chart_id);
                    }
                    const size_t idx = static_cast<size_t>(std::max<long long>(0, index));
                    if (scene.chart.values.size() <= idx) {
                        scene.chart.values.resize(idx + 1, 0);
                    }
                    scene.chart.values[idx] = static_cast<int>(value);
                }
            } else if (action == "commit") {
                scene.chart.present = true;
            }
            continue;
        }

        if (group == "frame" && action == "present") {
            scene.notes.push_back("frame present");
            continue;
        }

        scene.notes.push_back("line " + std::to_string(line_no) + ": unsupported ui directive");
    }

    return saw_ui;
}

struct Rgba {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 1.0;
};

constexpr double kPi = 3.14159265358979323846;

static void set_source_rgba(cairo_t* cr, const Rgba& c) {
    cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a);
}

static void draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
    const double x2 = x + w;
    const double y2 = y + h;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x2 - r, y + r, r, -kPi / 2.0, 0.0);
    cairo_arc(cr, x2 - r, y2 - r, r, 0.0, kPi / 2.0);
    cairo_arc(cr, x + r, y2 - r, r, kPi / 2.0, kPi);
    cairo_arc(cr, x + r, y + r, r, kPi, 3.0 * kPi / 2.0);
    cairo_close_path(cr);
}

static void fill_card(cairo_t* cr, double x, double y, double w, double h, double radius, const Rgba& fill, const Rgba& border) {
    cairo_save(cr);
    draw_rounded_rect(cr, x, y, w, h, radius);
    set_source_rgba(cr, fill);
    cairo_fill_preserve(cr);
    set_source_rgba(cr, border);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    cairo_restore(cr);
}

static void draw_text(cairo_t* cr,
                      const std::string& text,
                      double x,
                      double y,
                      const std::string& font_desc,
                      const Rgba& color,
                      int width = -1,
                      PangoAlignment align = PANGO_ALIGN_LEFT) {
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text.c_str(), -1);
    if (width > 0) {
        pango_layout_set_width(layout, width * PANGO_SCALE);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_alignment(layout, align);
    }
    PangoFontDescription* desc = pango_font_description_from_string(font_desc.c_str());
    pango_layout_set_font_description(layout, desc);
    cairo_save(cr);
    set_source_rgba(cr, color);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);
    pango_font_description_free(desc);
    g_object_unref(layout);
}

static void draw_text_centered(cairo_t* cr,
                               const std::string& text,
                               double center_x,
                               double center_y,
                               const std::string& font_desc,
                               const Rgba& color,
                               int max_width = -1) {
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text.c_str(), -1);
    if (max_width > 0) {
        pango_layout_set_width(layout, max_width * PANGO_SCALE);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    }
    PangoFontDescription* desc = pango_font_description_from_string(font_desc.c_str());
    pango_layout_set_font_description(layout, desc);

    int text_w = 0;
    int text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    cairo_save(cr);
    set_source_rgba(cr, color);
    cairo_move_to(cr, center_x - text_w / 2.0, center_y - text_h / 2.0);
    pango_cairo_show_layout(cr, layout);
    cairo_restore(cr);

    pango_font_description_free(desc);
    g_object_unref(layout);
}

static void draw_chip(cairo_t* cr, double x, double y, double w, double h, const std::string& text, const Rgba& fill, const Rgba& fg) {
    fill_card(cr, x, y, w, h, 999.0, fill, fill);
    draw_text_centered(cr, text, x + w / 2.0, y + h / 2.0 + 1.0, "Sans Bold 11", fg);
}

static void draw_widget_card(cairo_t* cr,
                             const UiWidget& widget,
                             double x,
                             double y,
                             double w,
                             double h,
                             const UiScene& scene) {
    const bool button = widget.kind == UiWidget::Kind::Button;
    const bool slider = widget.kind == UiWidget::Kind::Slider;
    const bool is_dragging = scene.dragging_slider_id == widget.widget_id;
    const bool is_clicked = scene.clicked_button_id == widget.widget_id;

    Rgba fill = button ? Rgba{0.13, 0.45, 0.92, 0.95} : Rgba{0.95, 0.97, 1.00, 0.96};
    Rgba border = button ? Rgba{0.17, 0.50, 0.98, 0.90} : Rgba{0.83, 0.88, 0.94, 1.00};
    
    // Visual feedback for interaction
    if (is_dragging) {
        fill = Rgba{0.20, 0.58, 1.00, 0.98};
        border = Rgba{0.30, 0.68, 1.00, 1.00};
    } else if (is_clicked) {
        fill = Rgba{0.08, 0.35, 0.82, 0.98};
        border = Rgba{0.12, 0.45, 0.95, 1.00};
    }
    
    fill_card(cr, x, y, w, h, 18.0, fill, border);

    const Rgba title_color = button ? Rgba{1.0, 1.0, 1.0, 1.0} : Rgba{0.08, 0.12, 0.18, 1.0};
    const Rgba body_color = button ? Rgba{0.90, 0.96, 1.0, 0.90} : Rgba{0.27, 0.34, 0.45, 1.0};

    draw_text(cr, widget_kind_name(widget.kind), x + 18.0, y + 16.0, "Sans Bold 11", body_color);
    draw_text(cr, "Widget #" + std::to_string(widget.widget_id), x + w - 102.0, y + 16.0, "Sans 10", body_color);

    if (widget.kind == UiWidget::Kind::Label || widget.kind == UiWidget::Kind::Button) {
        draw_text(cr, widget.text, x + 18.0, y + 48.0, button ? "Sans Bold 17" : "Sans Bold 16", title_color, static_cast<int>(w - 36.0));
        draw_text(cr, "window " + std::to_string(widget.window_id), x + 18.0, y + h - 26.0, "Sans 10", body_color);
    } else if (slider) {
        draw_text(cr, "Range: " + std::to_string(widget.min_value) + ".." + std::to_string(widget.max_value),
                  x + 18.0, y + 44.0, "Sans 12", title_color);
        draw_text(cr, "Current: " + std::to_string(widget.current_value),
                  x + 18.0, y + 66.0, "Sans Bold 16", title_color);

        const double track_x = x + 18.0;
        const double track_y = y + h - 28.0;
        const double track_w = w - 36.0;
        cairo_set_line_width(cr, 8.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        set_source_rgba(cr, Rgba{0.82, 0.86, 0.92, 1.0});
        cairo_move_to(cr, track_x, track_y);
        cairo_line_to(cr, track_x + track_w, track_y);
        cairo_stroke(cr);

        const double span = std::max(1, widget.max_value - widget.min_value);
        const double ratio = std::clamp((widget.current_value - widget.min_value) / span, 0.0, 1.0);
        const double knob_x = track_x + ratio * track_w;
        
        // Highlight knob when dragging
        if (is_dragging) {
            set_source_rgba(cr, Rgba{0.30, 0.68, 1.00, 1.0});
            cairo_arc(cr, knob_x, track_y, 11.0, 0.0, 2.0 * kPi);
            cairo_fill(cr);
        } else {
            set_source_rgba(cr, Rgba{0.15, 0.52, 0.94, 1.0});
            cairo_arc(cr, knob_x, track_y, 9.0, 0.0, 2.0 * kPi);
            cairo_fill(cr);
        }
    }
}

static void draw_chart_panel(cairo_t* cr,
                             const UiScene& scene,
                             double x,
                             double y,
                             double w,
                             double h) {
    fill_card(cr, x, y, w, h, 24.0, Rgba{0.07, 0.10, 0.18, 0.92}, Rgba{0.18, 0.24, 0.37, 1.0});
    draw_text(cr, "Chart " + std::to_string(scene.chart.chart_id), x + 20.0, y + 16.0, "Sans Bold 16", Rgba{1.0, 1.0, 1.0, 1.0});
    draw_text(cr, std::to_string(scene.chart.values.size()) + " samples", x + w - 130.0, y + 18.0, "Sans 11", Rgba{0.72, 0.79, 0.90, 1.0});

    const double pad_left = 24.0;
    const double pad_top = 56.0;
    const double pad_right = 24.0;
    const double pad_bottom = 28.0;
    const double cw = std::max(1.0, w - pad_left - pad_right);
    const double ch = std::max(1.0, h - pad_top - pad_bottom);
    const double gx = x + pad_left;
    const double gy = y + pad_top;

    cairo_save(cr);
    cairo_set_line_width(cr, 1.0);
    set_source_rgba(cr, Rgba{0.22, 0.28, 0.40, 0.55});
    for (int i = 0; i <= 4; ++i) {
        const double yy = gy + (ch / 4.0) * i;
        cairo_move_to(cr, gx, yy);
        cairo_line_to(cr, gx + cw, yy);
    }
    cairo_stroke(cr);
    cairo_restore(cr);

    if (scene.chart.values.empty()) {
        draw_text_centered(cr, "No chart data", x + w / 2.0, y + h / 2.0, "Sans Bold 18", Rgba{0.78, 0.82, 0.90, 0.90});
        return;
    }

    auto minmax = std::minmax_element(scene.chart.values.begin(), scene.chart.values.end());
    double min_v = static_cast<double>(*minmax.first);
    double max_v = static_cast<double>(*minmax.second);
    if (std::fabs(max_v - min_v) < 0.0001) {
        min_v -= 1.0;
        max_v += 1.0;
    }
    if (min_v > 0.0) min_v = 0.0;
    if (max_v < 1.0) max_v = 1.0;

    const size_t count = scene.chart.values.size();
    const double step = (count > 1) ? (cw / static_cast<double>(count - 1)) : cw;

    cairo_save(cr);
    cairo_set_line_width(cr, 2.4);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    set_source_rgba(cr, Rgba{0.15, 0.78, 0.87, 1.0});

    for (size_t i = 0; i < count; ++i) {
        const double value = static_cast<double>(scene.chart.values[i]);
        const double norm = (value - min_v) / (max_v - min_v);
        const double px = gx + (count > 1 ? step * static_cast<double>(i) : cw / 2.0);
        const double py = gy + ch - (norm * ch);
        if (i == 0) {
            cairo_move_to(cr, px, py);
        } else {
            cairo_line_to(cr, px, py);
        }
    }
    cairo_stroke(cr);

    for (size_t i = 0; i < count; ++i) {
        const double value = static_cast<double>(scene.chart.values[i]);
        const double norm = (value - min_v) / (max_v - min_v);
        const double px = gx + (count > 1 ? step * static_cast<double>(i) : cw / 2.0);
        const double py = gy + ch - (norm * ch);
        set_source_rgba(cr, Rgba{0.10, 0.62, 0.88, 1.0});
        cairo_arc(cr, px, py, 4.5, 0.0, 2.0 * kPi);
        cairo_fill(cr);

        if (count <= 12) {
            draw_text_centered(cr,
                               std::to_string(scene.chart.values[i]),
                               px,
                               py - 14.0,
                               "Sans 10",
                               Rgba{0.86, 0.93, 0.98, 1.0});
        }
    }
    cairo_restore(cr);

    draw_text(cr,
              "window " + std::to_string(scene.chart.window_id) +
                  "  •  range " + std::to_string(static_cast<int>(min_v)) + ".." + std::to_string(static_cast<int>(max_v)),
              x + 20.0,
              y + h - 22.0,
              "Sans 11",
              Rgba{0.70, 0.77, 0.88, 1.0});
}

static void render_ui_scene_to_cairo(cairo_t* cr, int width, int height, const UiScene& scene) {
    cairo_save(cr);
    cairo_pattern_t* bg = cairo_pattern_create_linear(0, 0, width, height);
    cairo_pattern_add_color_stop_rgb(bg, 0.0, 0.05, 0.08, 0.15);
    cairo_pattern_add_color_stop_rgb(bg, 0.55, 0.07, 0.14, 0.22);
    cairo_pattern_add_color_stop_rgb(bg, 1.0, 0.08, 0.20, 0.25);
    cairo_set_source(cr, bg);
    cairo_paint(cr);
    cairo_pattern_destroy(bg);
    cairo_restore(cr);

    // Atmosphere blobs.
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.10, 0.45, 0.85, 0.12);
    cairo_arc(cr, width * 0.12, height * 0.12, std::min(width, height) * 0.20, 0, 2 * kPi);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, 0.17, 0.78, 0.73, 0.10);
    cairo_arc(cr, width * 0.86, height * 0.20, std::min(width, height) * 0.15, 0, 2 * kPi);
    cairo_fill(cr);
    cairo_restore(cr);

    const double margin = 24.0;
    const double gap = std::max(8.0, static_cast<double>(scene.layout_spacing));

    fill_card(cr, margin, margin, width - 2 * margin, 110.0, 28.0, Rgba{0.96, 0.98, 1.0, 0.96}, Rgba{0.84, 0.88, 0.94, 1.0});
    draw_text(cr, "NanLang Graphics Toolkit", margin + 24.0, margin + 18.0, "Sans Bold 22", Rgba{0.08, 0.12, 0.18, 1.0});
    draw_text(cr, scene.title, margin + 24.0, margin + 52.0, "Sans Bold 31", Rgba{0.06, 0.22, 0.41, 1.0}, static_cast<int>(width * 0.62));
    draw_text(cr,
              "Rendered from NanLang `ui` directives. Close the window to exit.",
              margin + 24.0,
              margin + 90.0,
              "Sans 11",
              Rgba{0.32, 0.39, 0.51, 1.0});

    draw_chip(cr,
              width - margin - 230.0,
              margin + 18.0,
              90.0,
              24.0,
              scene.has_magic ? "NAN3" : "NO MAGIC",
              scene.has_magic ? Rgba{0.12, 0.61, 0.42, 0.95} : Rgba{0.73, 0.26, 0.27, 0.95},
              Rgba{1.0, 1.0, 1.0, 1.0});
    draw_chip(cr,
              width - margin - 130.0,
              margin + 18.0,
              118.0,
              24.0,
              scene.horizontal_layout ? "HORIZONTAL" : "VERTICAL",
              Rgba{0.11, 0.34, 0.73, 0.95},
              Rgba{1.0, 1.0, 1.0, 1.0});

    double y = margin + 126.0;
    const bool horizontal = scene.horizontal_layout;
    const size_t widget_count = scene.widgets.size();
    const double usable_w = width - 2 * margin;
    const double min_card_w = horizontal ? 270.0 : usable_w;
    size_t cols = 1;
    if (horizontal && widget_count > 0) {
        cols = std::max<size_t>(1, static_cast<size_t>((usable_w + gap) / (min_card_w + gap)));
        cols = std::min(cols, widget_count);
        cols = std::max<size_t>(1, cols);
    }
    const size_t rows = widget_count == 0 ? 0 : (widget_count + cols - 1) / cols;
    const double card_w = horizontal
        ? (usable_w - gap * (cols - 1)) / static_cast<double>(cols)
        : usable_w;
    const double card_h = horizontal ? 154.0 : 92.0;

    for (size_t i = 0; i < widget_count; ++i) {
        const size_t row = i / cols;
        const size_t col = i % cols;
        const double card_x = margin + static_cast<double>(col) * (card_w + gap);
        const double card_y = y + static_cast<double>(row) * (card_h + gap);
        draw_widget_card(cr, scene.widgets[i], card_x, card_y, card_w, card_h, scene);
    }

    const double chart_y = y + static_cast<double>(rows) * (card_h + gap) + 8.0;
    const double chart_h = std::max(220.0, height - chart_y - margin);
    draw_chart_panel(cr, scene, margin, chart_y, width - 2 * margin, std::min(chart_h, 300.0));

    std::string footer = "Widgets: " + std::to_string(scene.widgets.size());
    if (scene.chart.present) {
        footer += "  |  Chart points: " + std::to_string(scene.chart.values.size());
    }
    draw_text(cr,
              footer,
              margin + 8.0,
              height - 18.0,
              "Sans 10",
              Rgba{0.74, 0.80, 0.90, 0.90});
}

// Event handler for mouse button press
static gboolean on_ui_button_press(GtkWidget* widget, GdkEventButton* event, gpointer user_data) {
    auto* scene = static_cast<UiScene*>(user_data);
    if (event->button != 1) return FALSE; // Only left click

    scene->last_mouse_x = event->x;
    scene->last_mouse_y = event->y;
    
    // Check if click is on a slider to start dragging
    const double margin = 24.0;
    const double gap = std::max(8.0, static_cast<double>(scene->layout_spacing));
    const double y_start = margin + 126.0;
    const bool horizontal = scene->horizontal_layout;
    const size_t widget_count = scene->widgets.size();
    const int w_width = gtk_widget_get_allocated_width(widget);
    const double usable_w = w_width - 2 * margin;
    const double min_card_w = horizontal ? 270.0 : usable_w;
    size_t cols = horizontal && widget_count > 0 ? 
        std::max<size_t>(1, static_cast<size_t>((usable_w + gap) / (min_card_w + gap))) : 1;
    cols = std::min(cols, widget_count);
    cols = std::max<size_t>(1, cols);
    const double card_w = horizontal ? (usable_w - gap * (cols - 1)) / static_cast<double>(cols) : usable_w;
    const double card_h = horizontal ? 154.0 : 92.0;

    for (size_t i = 0; i < widget_count; ++i) {
        const UiWidget& w = scene->widgets[i];
        const size_t row = i / cols;
        const size_t col = i % cols;
        const double card_x = margin + static_cast<double>(col) * (card_w + gap);
        const double card_y = y_start + static_cast<double>(row) * (card_h + gap);
        
        if (event->x >= card_x && event->x < card_x + card_w && 
            event->y >= card_y && event->y < card_y + card_h) {
            if (w.kind == UiWidget::Kind::Slider) {
                scene->dragging_slider_id = w.widget_id;
                gtk_widget_queue_draw(widget);
                return TRUE;
            } else if (w.kind == UiWidget::Kind::Button) {
                scene->clicked_button_id = w.widget_id;
                gtk_widget_queue_draw(widget);
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Event handler for mouse button release
static gboolean on_ui_button_release(GtkWidget* widget, GdkEventButton* event, gpointer user_data) {
    auto* scene = static_cast<UiScene*>(user_data);
    if (event->button != 1) return FALSE;

    if (scene->dragging_slider_id >= 0) {
        scene->dragging_slider_id = -1;
        gtk_widget_queue_draw(widget);
        return TRUE;
    }
    
    if (scene->clicked_button_id >= 0) {
        // Print button click feedback
        std::cout << "[UI] Button #" << scene->clicked_button_id << " clicked" << std::endl;
        scene->clicked_button_id = -1;
        gtk_widget_queue_draw(widget);
        return TRUE;
    }
    
    return FALSE;
}

// Event handler for mouse motion
static gboolean on_ui_motion(GtkWidget* widget, GdkEventMotion* event, gpointer user_data) {
    auto* scene = static_cast<UiScene*>(user_data);
    
    // Handle slider dragging
    if (scene->dragging_slider_id >= 0) {
        for (auto& w : scene->widgets) {
            if (w.widget_id == scene->dragging_slider_id && w.kind == UiWidget::Kind::Slider) {
                // Simple horizontal drag-based slider adjustment
                double delta = event->x - scene->last_mouse_x;
                int new_value = w.current_value + static_cast<int>(delta * 0.5);
                w.current_value = std::clamp(new_value, w.min_value, w.max_value);
                std::cout << "[UI] Slider #" << w.widget_id << " = " << w.current_value << std::endl;
                gtk_widget_queue_draw(widget);
                break;
            }
        }
    }
    
    scene->last_mouse_x = event->x;
    scene->last_mouse_y = event->y;
    return FALSE;
}

static gboolean on_ui_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
    (void)widget;
    auto* scene = static_cast<UiScene*>(user_data);
    const int width = gtk_widget_get_allocated_width(widget);
    const int height = gtk_widget_get_allocated_height(widget);
    render_ui_scene_to_cairo(cr, width, height, *scene);
    return FALSE;
}

static void on_ui_activate(GtkApplication* app, gpointer user_data) {
    auto* scene = static_cast<UiScene*>(user_data);
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), scene->title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), scene->width, scene->height);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

    GtkWidget* area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(area, TRUE);
    gtk_widget_set_vexpand(area, TRUE);
    gtk_widget_add_events(area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    
    g_signal_connect(area, "draw", G_CALLBACK(on_ui_draw), scene);
    g_signal_connect(area, "button-press-event", G_CALLBACK(on_ui_button_press), scene);
    g_signal_connect(area, "button-release-event", G_CALLBACK(on_ui_button_release), scene);
    g_signal_connect(area, "motion-notify-event", G_CALLBACK(on_ui_motion), scene);
    
    scene->drawing_area = area;
    gtk_container_add(GTK_CONTAINER(window), area);
    gtk_widget_show_all(window);
}

static bool render_ui_scene(const std::string& src_path) {
    UiScene scene;
    if (!parse_ui_scene(src_path, scene)) {
        std::cerr << "[ERROR] No ui directives found in " << src_path << "\n";
        return false;
    }

    GtkApplication* app = gtk_application_new("org.nanlang.graphics", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_ui_activate), &scene);

    int argc = 1;
    char arg0[] = "nanlang";
    char* argv[] = {arg0, nullptr};
    const int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    for (const auto& note : scene.notes) {
        if (note.rfind("line ", 0) == 0) {
            std::cerr << "[WARN] " << note << "\n";
        }
    }

    return status == 0;
}

static void print_help() {
    std::cout << R"(NanLang C++ Core v4.0.0
Usage:
  nanlang_cpp --compile <src.nl> [--output <out.elf>]
             [--bin | --elf | --asm | --sh]
  nanlang_cpp --run <file.elf|script>
  nanlang_cpp --ui-render <scene.nl>
  nanlang_cpp --demo <pulse|sigbuf> [--count N] [--pattern 0xAA]
  nanlang_cpp --help | --version

Compilation Modes (all AOT — no VM):
  --bin           Output raw binary
  --elf           Output ELF executable (default)
  --asm           Output x86_64 assembly
  --sh            Output self-extracting shell script
  --aot           Accepted for compatibility (AOT is always active)
)" << std::endl;
}

static bool parse_flag_output_mode(const char* token, CompileOutputMode& mode) {
    if (std::strcmp(token, "--bin") == 0) {
        mode = CompileOutputMode::Bin;
        return true;
    }
    if (std::strcmp(token, "--elf") == 0) {
        mode = CompileOutputMode::Elf;
        return true;
    }
    if (std::strcmp(token, "--asm") == 0) {
        mode = CompileOutputMode::Asm;
        return true;
    }
    if (std::strcmp(token, "--sh") == 0) {
        mode = CompileOutputMode::Sh;
        return true;
    }
    return false;
}

static bool fast_path_compile_run(int argc, char** argv, int& exit_code) {
    if (argc >= 3 && std::strcmp(argv[1], "--ui-render") == 0) {
        const std::string src = argv[2];
        if (src.empty()) {
            std::cerr << "[ERROR] Missing UI scene path.\n";
            exit_code = 1;
            return true;
        }
        exit_code = render_ui_scene(src) ? 0 : 1;
        return true;
    }

    if (argc >= 3 && std::strcmp(argv[1], "--compile") == 0) {
        const std::string src = argv[2];
        if (src.empty()) {
            std::cerr << "[ERROR] Missing compile source path.\n";
            exit_code = 1;
            return true;
        }

        std::string out = "output.elf";
        CompileOutputMode mode = CompileOutputMode::Elf;
        int mode_count = 0;

        for (int i = 3; i < argc; ++i) {
            CompileOutputMode parsed_mode = CompileOutputMode::Elf;
            if (parse_flag_output_mode(argv[i], parsed_mode)) {
                mode = parsed_mode;
                ++mode_count;
                continue;
            }
            // --aot is now the unconditional default in v4.0.0; accept the
            // flag silently so existing scripts don't break.
            if (std::strcmp(argv[i], "--aot") == 0) continue;
            if (std::strcmp(argv[i], "--output") == 0) {
                if (i + 1 >= argc) {
                    std::cerr << "[ERROR] Missing value for --output\n";
                    exit_code = 1;
                    return true;
                }
                out = argv[++i];
                continue;
            }
            if (std::strncmp(argv[i], "--output=", 9) == 0) {
                out = argv[i] + 9;
                continue;
            }
        }

        if (mode_count > 1) {
            std::cerr << "[ERROR] choose only one output mode: --bin, --elf, --asm or --sh\n";
            exit_code = 1;
            return true;
        }

        // v4.0.0: AOT is the only compilation path — no VM, no bytecode.
        {
            nanlang::aot::AOTCompiler aot_compiler(src);
            bool success = (mode == CompileOutputMode::Bin)
                           ? aot_compiler.compile_to_binary(out)
                           : aot_compiler.compile_to_elf(out);
            if (success) {
                exit_code = 0;
            } else {
                std::cerr << "[ERROR] AOT compilation failed\n";
                for (const auto& err : aot_compiler.get_errors()) {
                    std::cerr << "  " << err << "\n";
                }
                exit_code = 1;
            }
        }
        return true;
    }

    if (argc >= 3 && std::strcmp(argv[1], "--run") == 0) {
        const std::string target = argv[2];
        if (target.empty()) {
            std::cerr << "[ERROR] Missing executable file path.\n";
            exit_code = 1;
            return true;
        }
        if (has_nan3_magic(target)) {
            std::cerr << "[ERROR] VM runtime removed in v4.0.0 (AOT-only).\n"
                      << "        Recompile source with --elf/--bin/--sh and run native output.\n";
            exit_code = 1;
            return true;
        }
        exit_code = run_native_file(target);
        return true;
    }

    return false;
}

} // namespace

int main(int argc, char** argv) {
    int fast_exit = 0;
    if (fast_path_compile_run(argc, argv, fast_exit)) {
        return fast_exit;
    }

    if (argc < 2) {
        print_help();
        return 1;
    }

    Args args = Args::parse(argc, argv);
    if (args.has("help")) {
        print_help();
        return 0;
    }
    if (args.has("version")) {
        std::cout << "NanLang C++ Core v4.0.0\n";
        return 0;
    }

    if (args.has("ui-render")) {
        const std::string src = args.get("ui-render");
        if (src.empty() || src == "true") {
            std::cerr << "[ERROR] Missing UI scene path.\n";
            return 1;
        }
        return render_ui_scene(src) ? 0 : 1;
    }

    if (args.has("compile")) {
        const std::string src = args.get("compile");
        if (src.empty() || src == "true") {
            std::cerr << "[ERROR] Missing compile source path.\n";
            return 1;
        }
        const std::string out = args.get("output", "output.elf");

        // v4.0.0: AOT is always used — --aot flag accepted but now redundant.
        {
            nanlang::aot::AOTCompiler aot_compiler(src);
            bool success = args.has("bin")
                           ? aot_compiler.compile_to_binary(out)
                           : aot_compiler.compile_to_elf(out);
            if (success) {
                return 0;
            } else {
                std::cerr << "[ERROR] AOT compilation failed\n";
                for (const auto& err : aot_compiler.get_errors()) {
                    std::cerr << "  " << err << "\n";
                }
                return 1;
            }
        }
    }

    if (args.has("run")) {
        const std::string target = args.get("run");
        if (target.empty() || target == "true") {
            std::cerr << "[ERROR] Missing executable file path.\n";
            return 1;
        }
        if (has_nan3_magic(target)) {
            std::cerr << "[ERROR] VM runtime removed in v4.0.0 (AOT-only).\n"
                      << "        Recompile source with --elf/--bin/--sh and run native output.\n";
            return 1;
        }
        return run_native_file(target);
    }

    if (args.has("demo")) {
        const std::string demo_name = args.get("demo", "pulse");
        if (demo_name == "pulse") {
            const int count = args.get_int("count", 64);
            const uint8_t pattern = static_cast<uint8_t>(args.get_int("pattern", 0xAA) & 0xFF);
            nanlang::run_pulse_emitter_demo(count, pattern);
            return 0;
        }
        if (demo_name == "sigbuf") {
            const int count = args.get_int("count", 64);
            nanlang::run_signal_buffer_demo(count);
            return 0;
        }
        std::cerr << "[ERROR] Unknown demo: " << demo_name << "\n";
        return 1;
    }

    std::cerr << "[ERROR] Invalid arguments. Use --help.\n";
    return 1;
}
