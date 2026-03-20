#pragma once
#include <immintrin.h> 
#include <vector>
#include <string>
#include <variant>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace nanlang {

/**
 * Basic energy unit. 
 * We use uint8_t to represent a 0-255 potential range, 
 * simulating hardware voltage levels.
 */
using Potential = uint8_t;

enum OpCode : uint8_t {
    OP_HALT = 0x01,
    OP_SAY  = 0x02
};

/**
 * Signal states. 
 * Using an enum class ensures type safety and prevents 
 * accidental arithmetic operations on binary states.
 */
enum class Signal : uint8_t { Low = 0, High = 1 };

/**
 * SignalPacket utilizing AVX2 SIMD.
 * Processes 256 bits of signal data in parallel.
 * This is essential for high-throughput stream processing.
 */
struct SignalPacket {
    __m256i data; 
    
    // Hardware-accelerated merge using bitwise OR.
    void merge(const SignalPacket& other) {
        data = _mm256_or_si256(data, other.data);
    }
};

/**
 * String wrapper with security-first memory handling.
 * Automatically overwrites data on destruction to protect 
 * sensitive information in system memory.
 */
struct ManagedString {
    std::string content;
    bool active = false;

    ManagedString(std::string v) : content(v), active(true) {}
    
    ~ManagedString() { 
        if(!content.empty()) {
            // Overwrite memory before deallocation.
            // Using volatile to prevent compiler optimization.
            volatile char* p = &content[0];
            for(size_t i = 0; i < content.size(); ++i) p[i] = 0;
        }
    }
};

using Value = std::variant<Potential, Signal, std::vector<Signal>, ManagedString>;

} // namespace nanlang
