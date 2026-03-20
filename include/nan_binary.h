#pragma once
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nanlang {

// ─── Compact Opcode Map ──────────────────────────────────────────────────────
// Nan-ISA that reduces the most-used instructions to 1 byte.
enum NanOpcode : uint8_t {
    NOP  = 0x00, HALT = 0x01, SAY  = 0x02,
    LOAD = 0x03, STORE= 0x04, ADD  = 0x05,
    SUB  = 0x06, JMP  = 0x07, JZ   = 0x08,
    XOR  = 0x09, OR   = 0x0A, AND  = 0x0B,
    SHL  = 0x0C, SHR  = 0x0D, CALL = 0x0E,
    RET  = 0x0F
};

// ─── Instruction Skip Logic ──────────────────────────────────────────────────
// Strips NOPs from the binary stream and jumps to the next real instruction.
inline std::vector<uint8_t> strip_nops(const std::vector<uint8_t>& bytecode) {
    std::vector<uint8_t> out;
    out.reserve(bytecode.size());
    for (uint8_t b : bytecode) {
        if (b != NanOpcode::NOP) out.push_back(b);
    }
    return out;
}

// ─── Atomic Bit Swapping (XOR tactics) ──────────────────────────────────────
// Data transfer at the individual bit level instead of bytes.
inline uint8_t bit_swap_xor(uint8_t a, uint8_t key) noexcept {
    return a ^ key ^ (key << 4 | key >> 4);
}

// ─── Hex-Level Compression ──────────────────────────────────────────────────
// Self-extracting mini binary using run-length encoding.
inline std::vector<uint8_t> rle_compress(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i < in.size(); ) {
        uint8_t val = in[i];
        uint8_t cnt = 1;
        while (i + cnt < in.size() && in[i + cnt] == val && cnt < 255) ++cnt;
        out.push_back(cnt);
        out.push_back(val);
        i += cnt;
    }
    return out;
}

inline std::vector<uint8_t> rle_decompress(const std::vector<uint8_t>& in) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < in.size(); i += 2) {
        uint8_t cnt = in[i], val = in[i + 1];
        for (uint8_t j = 0; j < cnt; ++j) out.push_back(val);
    }
    return out;
}

// ─── Binary Patching on the Fly ─────────────────────────────────────────────
// Applies an instant patch to the specified offset of a running binary.
struct BinaryPatcher {
    std::vector<uint8_t> image;

    explicit BinaryPatcher(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) { std::cerr << "[PATCH] File not found: " << path << "\n"; return; }
        image.assign(std::istreambuf_iterator<char>(f), {});
    }

    // Writes patch_bytes at the specified offset.
    bool patch(size_t offset, const std::vector<uint8_t>& patch_bytes) {
        if (offset + patch_bytes.size() > image.size()) return false;
        memcpy(image.data() + offset, patch_bytes.data(), patch_bytes.size());
        return true;
    }

    // Searches for a byte sequence and applies a patch if found.
    bool find_and_patch(const std::vector<uint8_t>& needle,
                        const std::vector<uint8_t>& replacement) {
        for (size_t i = 0; i + needle.size() <= image.size(); ++i) {
            if (memcmp(image.data() + i, needle.data(), needle.size()) == 0) {
                return patch(i, replacement);
            }
        }
        return false;
    }

    bool save(const std::string& path) const {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(image.data()), image.size());
        return f.good();
    }
};

// ─── Hex Obfuscation (Hardware Key) ─────────────────────────────────────────
// Encrypts/decrypts bytecode with the given CPU-ID key (XOR-based).
inline std::vector<uint8_t> hw_key_xor(const std::vector<uint8_t>& data,
                                        uint64_t cpu_id) {
    std::vector<uint8_t> out(data.size());
    const uint8_t* key = reinterpret_cast<const uint8_t*>(&cpu_id);
    for (size_t i = 0; i < data.size(); ++i)
        out[i] = data[i] ^ key[i % 8];
    return out;
}

// ─── Self-Healing Binary ─────────────────────────────────────────────────────
// Simple error correction based on Hamming(7,4).
inline uint8_t hamming_encode(uint8_t nibble) {
    uint8_t d1 = (nibble >> 3) & 1, d2 = (nibble >> 2) & 1;
    uint8_t d3 = (nibble >> 1) & 1, d4 =  nibble       & 1;
    uint8_t p1 = d1 ^ d2 ^ d4;
    uint8_t p2 = d1 ^ d3 ^ d4;
    uint8_t p3 = d2 ^ d3 ^ d4;
    return (p1 << 6) | (p2 << 5) | (d1 << 4) |
           (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

inline uint8_t hamming_decode_correct(uint8_t code) {
    uint8_t p1 = (code >> 6) & 1, p2 = (code >> 5) & 1;
    uint8_t d1 = (code >> 4) & 1, p3 = (code >> 3) & 1;
    uint8_t d2 = (code >> 2) & 1, d3 = (code >> 1) & 1, d4 = code & 1;
    uint8_t s1 = p1 ^ d1 ^ d2 ^ d4;
    uint8_t s2 = p2 ^ d1 ^ d3 ^ d4;
    uint8_t s3 = p3 ^ d2 ^ d3 ^ d4;
    uint8_t syndrome = (s1 << 2) | (s2 << 1) | s3;
    if (syndrome) code ^= (1 << (7 - syndrome)); // bit correction
    return (((code >> 4) & 1) << 3) | (((code >> 2) & 1) << 2) |
           (((code >> 1) & 1) << 1) | (code & 1);
}

// ─── Zero-Header ELF ────────────────────────────────────────────────────────
// Writes a minimal ELF64 header (sets entry point directly to the code).
inline void write_minimal_elf(const std::string& path,
                               const std::vector<uint8_t>& code,
                               uint64_t entry_offset = 0) {
    std::ofstream f(path, std::ios::binary);
    // ELF64 header: 64 bytes
    uint8_t hdr[64] = {};
    // Magic
    hdr[0]=0x7f; hdr[1]='E'; hdr[2]='L'; hdr[3]='F';
    hdr[4]=2; hdr[5]=1; hdr[6]=1; // 64-bit, LE, ELF v1
    hdr[16]=2; hdr[17]=0;           // ET_EXEC
    hdr[18]=0x3e; hdr[19]=0;        // x86-64
    hdr[20]=1;                       // ELF version
    // Entry point = 0x400000 + 64 (after header) + entry_offset
    uint64_t ep = 0x400000 + 64 + entry_offset;
    memcpy(hdr + 24, &ep, 8);
    hdr[40] = 64; // phoff
    hdr[52] = 64; hdr[54] = 1; // ehsize, phentsize, phnum
    f.write(reinterpret_cast<char*>(hdr), 64);
    // Minimal program header (LOAD segment)
    uint8_t phdr[56] = {};
    phdr[0]=1;                          // PT_LOAD
    phdr[4]=5;                          // PF_R | PF_X
    uint64_t offset=64, vaddr=0x400000+64, filesz=code.size(), memsz=code.size();
    memcpy(phdr+ 8, &offset, 8);
    memcpy(phdr+16, &vaddr,  8);
    memcpy(phdr+24, &vaddr,  8);
    memcpy(phdr+32, &filesz, 8);
    memcpy(phdr+40, &memsz,  8);
    f.write(reinterpret_cast<char*>(phdr), 56);
    f.write(reinterpret_cast<const char*>(code.data()), code.size());
}

// ─── Static Binary Embedding ─────────────────────────────────────────────────
// Embeds dependencies at the end of the binary and appends a catalog table.
struct EmbeddedResource {
    std::string name;
    std::vector<uint8_t> data;
};

inline void embed_resources(std::vector<uint8_t>& binary,
                             const std::vector<EmbeddedResource>& resources) {
    // Catalog: [count:4][name_len:2][name][data_len:4][data]...
    uint32_t count = static_cast<uint32_t>(resources.size());
    auto push32 = [&](uint32_t v) {
        binary.push_back(v & 0xFF); binary.push_back((v >> 8) & 0xFF);
        binary.push_back((v>>16) & 0xFF); binary.push_back((v>>24) & 0xFF);
    };
    auto push16 = [&](uint16_t v) {
        binary.push_back(v & 0xFF); binary.push_back((v >> 8) & 0xFF);
    };
    push32(count);
    for (auto& r : resources) {
        push16(static_cast<uint16_t>(r.name.size()));
        binary.insert(binary.end(), r.name.begin(), r.name.end());
        push32(static_cast<uint32_t>(r.data.size()));
        binary.insert(binary.end(), r.data.begin(), r.data.end());
    }
}

} // namespace nanlang
