#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static bool flag_enabled(const char* name) {
    const char* v = getenv(name);
    if (!v || !*v) return false;
    return strcmp(v, "0") != 0;
}

static bool output_is_fresh(const char* src_path, const char* out_path) {
    struct stat src_st;
    struct stat out_st;
    if (stat(src_path, &src_st) != 0) return false;
    if (stat(out_path, &out_st) != 0) return false;
    if (out_st.st_size <= 0) return false;

    const int64_t src_sec = (int64_t)src_st.st_mtime;
    const int64_t out_sec = (int64_t)out_st.st_mtime;
    if (out_sec > src_sec) return true;
    if (out_sec < src_sec) return false;
    return true;
}

static const char* resolve_nanlang_cpp(void) {
    const char* env = getenv("NANLANG_CPP");
    if (env && *env) return env;
    if (access("./bin/nanlang_cpp", X_OK) == 0) return "./bin/nanlang_cpp";
    return "nanlang_cpp";
}

static int decode_wait_status(int status) {
    if (status < 0) return 1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

static int delegate_to_nanlang_cpp(int argc, char** argv) {
    const char* cpp_bin = resolve_nanlang_cpp();
    pid_t pid = fork();
    if (pid < 0) return 1;
    if (pid == 0) {
        char** child_argv = (char**)calloc((size_t)argc + 1, sizeof(char*));
        if (!child_argv) _exit(127);

        child_argv[0] = (char*)cpp_bin;
        for (int i = 1; i < argc; ++i) {
            child_argv[i] = argv[i];
        }
        child_argv[argc] = NULL;
        execvp(cpp_bin, child_argv);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return 1;
    return decode_wait_status(status);
}

static const char* parse_output_path(int argc, char** argv) {
    const char* out = "output.elf";
    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            out = argv[++i];
            continue;
        }
        if (strncmp(argv[i], "--output=", 9) == 0) {
            out = argv[i] + 9;
        }
    }
    return out;
}

int main(int argc, char** argv) {
    if (argc >= 3 && strcmp(argv[1], "--compile") == 0) {
        const char* src = argv[2];
        const char* out = parse_output_path(argc, argv);
        if (flag_enabled("NANLANG_FAST_CACHE") && output_is_fresh(src, out)) {
            return 0;
        }
    }

    return delegate_to_nanlang_cpp(argc, argv);
}
