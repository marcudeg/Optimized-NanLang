#include "../include/nan_binary.h"
#include <iostream>

namespace nanlang {

void run_binary_patch_demo(const std::string& path,
                            size_t offset,
                            const std::vector<uint8_t>& patch_bytes) {
    BinaryPatcher patcher(path);
    if (patcher.image.empty()) {
        std::cerr << "[PATCH] Empty image, aborting.\n"; return;
    }
    bool ok = patcher.patch(offset, patch_bytes);
    std::cout << "[PATCH] offset=0x" << std::hex << offset
              << " status=" << (ok ? "OK" : "FAIL") << std::dec << "\n";
    if (ok) {
        std::string out = path + ".patched";
        patcher.save(out);
        std::cout << "[PATCH] Saved: " << out << "\n";
    }
}

void run_obfuscation_demo(const std::vector<uint8_t>& data, uint64_t cpu_id) {
    auto enc = hw_key_xor(data, cpu_id);
    auto dec = hw_key_xor(enc, cpu_id);

    std::cout << "[OBFUS] Original[0]=" << (int)data[0]
              << " Encrypted[0]="       << (int)enc[0]
              << " Decrypted[0]="       << (int)dec[0]
              << (dec == data ? "  [MATCH]" : "  [MISMATCH]") << "\n";
}

void run_compression_demo(const std::vector<uint8_t>& data) {
    auto compressed   = rle_compress(data);
    auto decompressed = rle_decompress(compressed);

    std::cout << "[COMPRESS] original=" << data.size()
              << " compressed=" << compressed.size()
              << " restored=" << decompressed.size()
              << (decompressed == data ? " [OK]" : " [FAIL]") << "\n";
}

void run_selfheal_demo(uint8_t nibble) {
    uint8_t encoded  = hamming_encode(nibble);
    // Flip a single bit.
    uint8_t corrupted = encoded ^ 0x08;
    uint8_t corrected  = hamming_decode_correct(corrupted);

    std::cout << "[HAMMING] nibble=0x" << std::hex << (int)nibble
              << " encoded=0x"  << (int)encoded
              << " corrupted=0x"<< (int)corrupted
              << " corrected=0x"<< (int)corrected
              << (corrected == nibble ? " [HEALED]" : " [FAIL]")
              << std::dec << "\n";
}

} // namespace nanlang
