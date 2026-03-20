#include "../include/nan_arch.h"
#include <cstring>
#include <iostream>

namespace nanlang {

void run_simd_demo(int iterations) {
    alignas(32) uint8_t a[32], b[32], out[32];
    for (int i = 0; i < 32; ++i) { a[i] = (uint8_t)i; b[i] = (uint8_t)(255 - i); }

    for (int k = 0; k < iterations; ++k) {
        SIMDProcessor::xor32(a, b, out);
        SIMDProcessor::or32(a, b, out);
    }
    int active = SIMDProcessor::count_active(out, 32);
    std::cout << "[SIMD] iterations=" << iterations
              << " active_bytes=" << active << "\n";
}

void run_dma_demo(size_t size_bytes) {
    uint8_t* src = DMAPulse::map_region(size_bytes);
    uint8_t* dst = DMAPulse::map_region(size_bytes);

    memset(src, 0xAB, size_bytes);
    DMAPulse::nt_store(dst, src, size_bytes);

    int match = memcmp(src, dst, size_bytes) == 0;
    std::cout << "[DMA] size=" << size_bytes
              << " bytes  transfer=" << (match ? "OK" : "FAIL") << "\n";

    DMAPulse::unmap_region(src, size_bytes);
    DMAPulse::unmap_region(dst, size_bytes);
}

void run_zerocopy_demo(const std::vector<uint8_t>& data, size_t offset, size_t len) {
    ZeroCopySlice full(data);
    ZeroCopySlice slice = full.sub(offset, len);

    std::cout << "[ZEROCOPY] full=" << full.len
              << " slice offset=" << offset
              << " len=" << slice.len
              << " valid=" << slice.valid() << "\n";
    if (slice.valid())
        std::cout << "[ZEROCOPY] first_byte=0x" << std::hex
                  << (int)slice[0] << std::dec << "\n";
}

void run_register_map_demo() {
    RegisterMap rm;
    int r0 = rm.allocate(42);
    int r1 = rm.allocate(99);
    int r2 = rm.allocate(7);

    std::cout << "[REGS] R" << r0 << "=" << rm.read(r0)
              << " R" << r1 << "=" << rm.read(r1)
              << " R" << r2 << "=" << rm.read(r2)
              << " used=" << rm.used_count() << "\n";

    rm.free_reg(r1);
    std::cout << "[REGS] After free R" << r1 << " used=" << rm.used_count() << "\n";
}

} // namespace nanlang
