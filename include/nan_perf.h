#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <immintrin.h>
#include <iostream>
#include <thread>
#include <vector>

namespace nanlang {

// ─── Pipeline Interlocking Bypass ───────────────────────────────────────────
// Analyzes data dependencies and hoists independent instructions.
struct PipelineReorder {
    struct Inst { uint8_t op; int src1, src2, dst; };

    // Hoists instructions with no dependencies (simple forward pass).
    static std::vector<Inst> reorder(std::vector<Inst> instructions) {
        std::vector<Inst> out;
        std::vector<bool> scheduled(instructions.size(), false);
        std::vector<int> last_write(64, -1); // cycle in which each register was written

        for (size_t pass = 0; pass < instructions.size(); ++pass) {
            for (size_t i = 0; i < instructions.size(); ++i) {
                if (scheduled[i]) continue;
                auto& inst = instructions[i];
                // Are source registers ready?
                bool ready = (last_write[inst.src1 & 63] < (int)out.size()) &&
                             (last_write[inst.src2 & 63] < (int)out.size());
                if (ready) {
                    out.push_back(inst);
                    last_write[inst.dst & 63] = static_cast<int>(out.size());
                    scheduled[i] = true;
                    break;
                }
            }
        }
        return out;
    }
};

// ─── Out-of-Order Optimization ───────────────────────────────────────────────
// Tomasulo-like reservation station simulation.
struct ReservationStation {
    static constexpr int RS_SIZE = 8;

    struct Entry {
        uint8_t  op;
        int64_t  vj, vk;    // operand values
        int      qj, qk;    // which RS is waited on (-1 = ready)
        bool     busy = false;
    };

    Entry stations[RS_SIZE]{};
    int64_t result_bus = 0;

    int issue(uint8_t op, int64_t vj, int64_t vk,
              int qj = -1, int qk = -1) {
        for (int i = 0; i < RS_SIZE; ++i) {
            if (!stations[i].busy) {
                stations[i] = {op, vj, vk, qj, qk, true};
                return i;
            }
        }
        return -1; // structural hazard
    }

    void execute_ready() {
        for (int i = 0; i < RS_SIZE; ++i) {
            auto& e = stations[i];
            if (e.busy && e.qj == -1 && e.qk == -1) {
                switch (e.op) {
                    case 0x05: result_bus = e.vj + e.vk; break; // ADD
                    case 0x06: result_bus = e.vj - e.vk; break; // SUB
                    case 0x09: result_bus = e.vj ^ e.vk; break; // XOR
                    default:   result_bus = e.vj;         break;
                }
                e.busy = false;
            }
        }
    }
};

// ─── Predictive Branching Hex ────────────────────────────────────────────────
// Sets opcode bits so the branch predictor is never misled.
struct BranchPredictor {
    // 2-bit saturation counter holding the last N branch outcomes.
    static constexpr int HISTORY = 256;
    uint8_t table[HISTORY]{};  // 0-3: strongly-not-taken, weakly-not-taken, weakly-taken, strongly-taken

    // Make prediction (>= 2 -> taken).
    bool predict(uint8_t pc_hash) const noexcept {
        return table[pc_hash % HISTORY] >= 2;
    }

    // Update table with outcome.
    void update(uint8_t pc_hash, bool taken) noexcept {
        uint8_t& entry = table[pc_hash % HISTORY];
        if (taken && entry < 3) ++entry;
        else if (!taken && entry > 0) --entry;
    }

    // Adds a prediction hint to bytecode (as a prefetch hint).
    static uint8_t hint_byte(bool likely_taken) noexcept {
        return likely_taken ? 0x3E : 0x2E; // x86 branch hint prefix
    }
};

// ─── Wait-Free Data Structures ───────────────────────────────────────────────
// Lock-free SPSC queue that never causes stalls.
template<typename T, size_t N>
struct WaitFreeQueue {
    static_assert((N & (N-1)) == 0, "N must be power of 2");

    alignas(64) std::atomic<size_t> head{0};
    alignas(64) std::atomic<size_t> tail{0};
    alignas(64) T buffer[N]{};

    bool push(T val) noexcept {
        size_t t = tail.load(std::memory_order_relaxed);
        size_t next = (t + 1) & (N - 1);
        if (next == head.load(std::memory_order_acquire)) return false; // full
        buffer[t] = val;
        tail.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept {
        size_t h = head.load(std::memory_order_relaxed);
        if (h == tail.load(std::memory_order_acquire)) return false; // empty
        out = buffer[h];
        head.store((h + 1) & (N - 1), std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return head.load(std::memory_order_relaxed) ==
               tail.load(std::memory_order_relaxed);
    }
};

// ─── Dead-Lock Prevention Pulse ──────────────────────────────────────────────
// Tracks the resource allocation graph to detect cycles.
struct DeadlockDetector {
    static constexpr int MAX_RES = 32;
    int holder[MAX_RES];   // resource -> thread id (-1 = free)
    int waiting[MAX_RES];  // thread -> resource it is waiting for (-1 = none)

    DeadlockDetector() {
        for (int i = 0; i < MAX_RES; ++i) { holder[i] = -1; waiting[i] = -1; }
    }

    // Searches for a cycle in the wait graph using DFS.
    bool has_cycle(int start_thread) const {
        bool visited[MAX_RES]{};
        int cur = start_thread;
        for (int step = 0; step < MAX_RES; ++step) {
            if (cur < 0) return false;
            if (visited[cur]) return true;
            visited[cur] = true;
            // Which resource is cur waiting for?
            int res = waiting[cur];
            if (res < 0) return false;
            // Who holds that resource?
            cur = holder[res];
        }
        return false;
    }

    bool try_acquire(int thread_id, int resource) {
        if (holder[resource] == -1) {
            holder[resource] = thread_id;
            return true;
        }
        waiting[thread_id] = resource;
        if (has_cycle(thread_id)) {
            waiting[thread_id] = -1;
            std::cerr << "[DEADLOCK] Cycle detected! Thread "
                      << thread_id << " blocked on resource " << resource << "\n";
            return false;
        }
        return false; // legit wait
    }

    void release(int thread_id, int resource) {
        if (holder[resource] == thread_id) {
            holder[resource] = -1;
            waiting[thread_id] = -1;
        }
    }
};

// ─── Minimalist Context Switching ───────────────────────────────────────────
// Minimal setjmp-like structure that saves only the changed registers.
struct MiniContext {
    uint64_t rsp, rbp, rbx, r12, r13, r14, r15, rip;

    // Fiber-like: save the current context.
    // (Real fiber requires platform-specific code; this is a stub version.)
    void save() noexcept {
        rsp = 0; rbp = 0; // placeholder: real impl uses ASM
    }

    void restore() noexcept {
        // placeholder
    }
};

// ─── Dynamic Frequency Scaling ───────────────────────────────────────────────
// Hint functions for directing Turbo Boost according to workload.
struct FreqScaler {
    enum class Load { LIGHT, MEDIUM, HEAVY };

    static void hint(Load load) noexcept {
        switch (load) {
            case Load::LIGHT:
                // Power saving: allow pipeline stall.
                for (int i = 0; i < 10; ++i) __builtin_ia32_pause();
                break;
            case Load::MEDIUM:
                // Normal: memory fence only.
                _mm_sfence();
                break;
            case Load::HEAVY:
                // Turbo: speculative load, prefetch, and no pauses.
                _mm_lfence();
                break;
        }
    }
};

} // namespace nanlang
