#pragma once
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include <iostream>
#include <vector>

#ifdef __linux__
#  include <sys/mman.h>
#endif

namespace nanlang {

// ─── SIMD Vectorization (AVX/SSE) ───────────────────────────────────────────
// Processes 32 bytes of data in a single pass with one bytecode instruction.
struct SIMDProcessor {

    // XORs 32 uint8_t values in a single AVX2 vector operation.
    static void xor32(const uint8_t* __restrict__ a,
                      const uint8_t* __restrict__ b,
                      uint8_t* __restrict__ out) noexcept {
#ifdef __AVX2__
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(out),
                            _mm256_xor_si256(va, vb));
#else
        for (int i = 0; i < 32; ++i) out[i] = a[i] ^ b[i];
#endif
    }

    // ORs 32 uint8_t values (signal merge).
    static void or32(const uint8_t* a, const uint8_t* b, uint8_t* out) noexcept {
#ifdef __AVX2__
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b));
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(out),
                            _mm256_or_si256(va, vb));
#else
        for (int i = 0; i < 32; ++i) out[i] = a[i] | b[i];
#endif
    }

    // Counts non-zero bytes in the vector (similar to popcount).
    static int count_active(const uint8_t* data, size_t len) noexcept {
        int cnt = 0;
        for (size_t i = 0; i < len; ++i) cnt += (data[i] != 0);
        return cnt;
    }
};

// ─── Cache-Line Alignment (64-byte) ─────────────────────────────────────────
// Aligns all binary blocks to a 64-byte cache line.
template<typename T>
struct alignas(64) CacheAligned {
    T value{};
    CacheAligned() = default;
    explicit CacheAligned(T v) : value(v) {}
    T& get() noexcept { return value; }
    const T& get() const noexcept { return value; }
};

// Aligned memory allocation.
inline void* alloc_aligned(size_t size, size_t align = 64) {
    void* ptr = nullptr;
#ifdef _WIN32
    ptr = _aligned_malloc(size, align);
#else
    if (posix_memalign(&ptr, align, size) != 0) ptr = nullptr;
#endif
    return ptr;
}

inline void free_aligned(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// ─── Zero-Copy Bytecode ──────────────────────────────────────────────────────
// Holds only a reference (pointer+size) instead of copying data.
struct ZeroCopySlice {
    const uint8_t* ptr = nullptr;
    size_t         len = 0;

    ZeroCopySlice() = default;
    ZeroCopySlice(const uint8_t* p, size_t l) : ptr(p), len(l) {}
    ZeroCopySlice(const std::vector<uint8_t>& v)
        : ptr(v.data()), len(v.size()) {}

    uint8_t operator[](size_t i) const noexcept { return ptr[i]; }
    bool valid() const noexcept { return ptr && len > 0; }

    // Sub-slice (no copy, only pointer shift).
    ZeroCopySlice sub(size_t offset, size_t count) const noexcept {
        if (offset + count > len) return {};
        return {ptr + offset, count};
    }
};

// ─── Register Pressure Management ───────────────────────────────────────────
// Record tracking which variable is assigned to which register.
struct RegisterMap {
    static constexpr int NUM_REGS = 16; // R0-R15
    int64_t regs[NUM_REGS]{};
    bool    in_use[NUM_REGS]{};

    // Find the first free register and assign a value.
    int allocate(int64_t value) {
        for (int i = 0; i < NUM_REGS; ++i) {
            if (!in_use[i]) {
                regs[i] = value;
                in_use[i] = true;
                return i;
            }
        }
        return -1; // register overflow -> spill to RAM
    }

    void free_reg(int id) {
        if (id >= 0 && id < NUM_REGS) {
            regs[id] = 0;
            in_use[id] = false;
        }
    }

    int64_t read(int id) const noexcept {
        return (id >= 0 && id < NUM_REGS) ? regs[id] : 0;
    }

    int used_count() const noexcept {
        int c = 0;
        for (int i = 0; i < NUM_REGS; ++i) c += in_use[i];
        return c;
    }
};

// ─── DMA Pulse ───────────────────────────────────────────────────────────────
// Simulates direct source -> destination byte transfer bypassing RAM.
struct DMAPulse {
    // mmap-based: direct page mapping bypassing the OS buffer.
    static uint8_t* map_region(size_t size) {
#ifdef __linux__
        void* p = mmap(nullptr, size,
                       PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_LOCKED,
                       -1, 0);
        if (p == MAP_FAILED) {
            std::cerr << "[DMA] mmap failed, falling back to malloc\n";
            return static_cast<uint8_t*>(malloc(size));
        }
        return static_cast<uint8_t*>(p);
#else
        return static_cast<uint8_t*>(malloc(size));
#endif
    }

    static void unmap_region(uint8_t* ptr, size_t size) {
#ifdef __linux__
        munmap(ptr, size);
#else
        free(ptr);
#endif
    }

    // Non-temporal store: writes directly to RAM without polluting L1/L2.
    static void nt_store(uint8_t* dst, const uint8_t* src, size_t len) noexcept {
#ifdef __AVX2__
        size_t i = 0;
        for (; i + 32 <= len; i += 32) {
            __m256i v = _mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(src + i));
            _mm256_stream_si256(reinterpret_cast<__m256i*>(dst + i), v);
        }
        _mm_sfence();
        for (; i < len; ++i) dst[i] = src[i]; // tail
#else
        memcpy(dst, src, len);
#endif
    }
};

// ─── Page Table Optimization ────────────────────────────────────────────────
// Reduces TLB misses by allocating huge pages.
struct HugePageAlloc {
    static constexpr size_t HUGE_PAGE = 2ULL * 1024 * 1024; // 2 MB

    static uint8_t* alloc(size_t size) {
#ifdef __linux__
        // Round up to the nearest 2MB multiple.
        size = (size + HUGE_PAGE - 1) & ~(HUGE_PAGE - 1);
        void* p = mmap(nullptr, size,
                       PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                       -1, 0);
        if (p != MAP_FAILED) return static_cast<uint8_t*>(p);
        // Huge page failed -> fall back to normal mmap.
        p = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        return (p != MAP_FAILED) ? static_cast<uint8_t*>(p) : nullptr;
#else
        return static_cast<uint8_t*>(malloc(size));
#endif
    }

    static void free(uint8_t* ptr, size_t size) {
#ifdef __linux__
        size = (size + HUGE_PAGE - 1) & ~(HUGE_PAGE - 1);
        munmap(ptr, size);
#else
        ::free(ptr);
#endif
    }
};

// ─── Instruction Prefetching ─────────────────────────────────────────────────
// Proactively pulls the next hex block into L1 cache.
inline void prefetch_block(const void* ptr, size_t stride = 64) noexcept {
    const char* p = static_cast<const char*>(ptr);
    // Prefetch three cache-lines ahead.
    __builtin_prefetch(p + stride,     0, 3);
    __builtin_prefetch(p + stride * 2, 0, 2);
    __builtin_prefetch(p + stride * 3, 0, 1);
}

} // namespace nanlang
