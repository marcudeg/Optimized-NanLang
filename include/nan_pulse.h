#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <vector>

namespace nanlang {

// ─── Async Pulse Emitter ────────────────────────────────────────────────────
// Asynchronous writer that pushes signals directly into cache (L1/L2) without waiting.
struct AsyncPulseEmitter {
    static constexpr size_t CACHE_LINE = 64;

    alignas(CACHE_LINE) uint8_t cache_buf[CACHE_LINE * 16]{};
    std::atomic<size_t> write_pos{0};

    // Non-blocking emit: writes to cache-line with atomic position update.
    inline void emit(uint8_t signal) noexcept {
        size_t pos = write_pos.fetch_add(1, std::memory_order_relaxed)
                     % sizeof(cache_buf);
        cache_buf[pos] = signal;
        // Trigger cache line flush (store fence).
        _mm_sfence();
    }

    // Flushes and resets the entire buffer.
    inline void flush() noexcept {
        write_pos.store(0, std::memory_order_release);
        memset(cache_buf, 0, sizeof(cache_buf));
    }

    // Returns the current write position.
    inline size_t pos() const noexcept {
        return write_pos.load(std::memory_order_acquire);
    }
};

// ─── Clock-Cycle Synchronizer ───────────────────────────────────────────────
// Maps each tick to the CPU clock frequency using RDTSC.
struct ClockCycleSync {
    uint64_t baseline = 0;

    inline void calibrate() noexcept {
        unsigned aux;
        baseline = __rdtscp(&aux);
    }

    // Returns elapsed cycle count (for nanosecond calibration).
    inline uint64_t elapsed() const noexcept {
        unsigned aux;
        return __rdtscp(&aux) - baseline;
    }

    // Spin-waits for the specified number of cycles.
    static inline void wait_cycles(uint64_t cycles) noexcept {
        unsigned aux;
        uint64_t start = __rdtscp(&aux);
        while ((__rdtscp(&aux) - start) < cycles) {
            __builtin_ia32_pause();
        }
    }
};

// ─── Signal Buffer Overclocking ─────────────────────────────────────────────
// Places buffers in the processor's L1 hot-path segment.
struct SignalBuffer {
    static constexpr size_t BUF_SIZE = 512;
    alignas(64) uint8_t data[BUF_SIZE]{};
    size_t head = 0, tail = 0;

    inline bool push(uint8_t v) noexcept {
        if ((tail + 1) % BUF_SIZE == head) return false; // full
        data[tail] = v;
        tail = (tail + 1) % BUF_SIZE;
        return true;
    }

    inline bool pop(uint8_t& out) noexcept {
        if (head == tail) return false; // empty
        out = data[head];
        head = (head + 1) % BUF_SIZE;
        return true;
    }

    inline bool empty() const noexcept { return head == tail; }
    inline size_t size()  const noexcept {
        return (tail - head + BUF_SIZE) % BUF_SIZE;
    }
};

// ─── Pulse Flow Analyzer ────────────────────────────────────────────────────
// Computes pulse-level statistics, transitions, and streaks from signal streams.
struct PulseFlowStats {
    size_t samples = 0;
    size_t high_count = 0;
    size_t low_count = 0;
    size_t uncertain_count = 0;
    size_t rising_edges = 0;
    size_t falling_edges = 0;
    size_t toggles = 0;
    size_t longest_high_streak = 0;
    size_t longest_low_streak = 0;
};

struct PulseFlowAnalyzer {
    enum class LogicLevel : uint8_t { Low = 0, High = 1, Uncertain = 2 };

    PulseFlowStats stats{};

    bool have_prev = false;
    LogicLevel prev = LogicLevel::Uncertain;
    size_t current_high_streak = 0;
    size_t current_low_streak = 0;

    static inline LogicLevel classify(uint8_t signal) noexcept {
        if (signal == 0xFF) return LogicLevel::Uncertain;
        // Pulse semantics: LSB encodes the active line, keeping raw byte intact.
        return (signal & 0x01) ? LogicLevel::High : LogicLevel::Low;
    }

    inline void reset() noexcept {
        stats = {};
        have_prev = false;
        prev = LogicLevel::Uncertain;
        current_high_streak = 0;
        current_low_streak = 0;
    }

    inline void ingest(const std::vector<uint8_t>& data) noexcept {
        ingest(data.data(), data.size());
    }

    inline void ingest(const uint8_t* data, size_t len) noexcept {
        if (!data || len == 0) return;
        stats.samples += len;
        count_levels(data, len);
        count_transitions_and_streaks(data, len);
    }

    inline PulseFlowStats snapshot() const noexcept { return stats; }

private:
    inline void count_levels_scalar(const uint8_t* data, size_t len) noexcept {
        for (size_t i = 0; i < len; ++i) {
            LogicLevel level = classify(data[i]);
            if (level == LogicLevel::High) {
                ++stats.high_count;
            } else if (level == LogicLevel::Low) {
                ++stats.low_count;
            } else {
                ++stats.uncertain_count;
            }
        }
    }

    inline void count_levels(const uint8_t* data, size_t len) noexcept {
#ifdef __AVX2__
        size_t i = 0;
        const __m256i ones      = _mm256_set1_epi8(0x01);
        const __m256i uncertain = _mm256_set1_epi8(static_cast<char>(0xFF));
        for (; i + 31 < len; i += 32) {
            __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
            __m256i lsb = _mm256_and_si256(v, ones);

            int high_mask      = _mm256_movemask_epi8(_mm256_cmpeq_epi8(lsb, ones));
            int uncertain_mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, uncertain));
            int high_count     = __builtin_popcount(static_cast<unsigned>(high_mask));
            int uncertain_cnt  = __builtin_popcount(static_cast<unsigned>(uncertain_mask));

            // Uncertain signals should not also count as high or low.
            high_count -= uncertain_cnt;

            stats.high_count += static_cast<size_t>(high_count > 0 ? high_count : 0);
            stats.uncertain_count += static_cast<size_t>(uncertain_cnt);
        }

        // Remainder
        count_levels_scalar(data + i, len - i);
#else
        count_levels_scalar(data, len);
#endif
        stats.low_count = stats.samples - stats.high_count - stats.uncertain_count;
    }

    inline void count_transitions_and_streaks(const uint8_t* data, size_t len) noexcept {
        for (size_t i = 0; i < len; ++i) {
            LogicLevel level = classify(data[i]);

            if (level == LogicLevel::High) {
                ++current_high_streak;
                current_low_streak = 0;
                if (current_high_streak > stats.longest_high_streak)
                    stats.longest_high_streak = current_high_streak;
            } else if (level == LogicLevel::Low) {
                ++current_low_streak;
                current_high_streak = 0;
                if (current_low_streak > stats.longest_low_streak)
                    stats.longest_low_streak = current_low_streak;
            } else {
                // Uncertain values break deterministic streaks.
                current_high_streak = 0;
                current_low_streak = 0;
            }

            if (have_prev &&
                prev != LogicLevel::Uncertain &&
                level != LogicLevel::Uncertain &&
                prev != level) {
                ++stats.toggles;
                if (prev == LogicLevel::Low && level == LogicLevel::High) {
                    ++stats.rising_edges;
                } else if (prev == LogicLevel::High && level == LogicLevel::Low) {
                    ++stats.falling_edges;
                }
            }

            prev = level;
            have_prev = true;
        }
    }
};

} // namespace nanlang
