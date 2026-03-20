#include "../include/nan_core.h"
#include <unordered_map>
#include <cstdlib>

namespace nanlang {

class ExecutionEngine {
private:
    std::unordered_map<std::string, Value> registers;
    std::vector<uint8_t> linear_memory;

public:
    ExecutionEngine() {
        // Allocate 8MB to minimize dynamic allocation during runtime.
        linear_memory.resize(8 * 1024 * 1024); 
    }

    /**
     * Updates the accumulator register.
     * [[likely]] attribute helps the compiler optimize branch prediction.
     */
    void update_state(const std::string& reg_id, Value val) {
        [[likely]] if (!reg_id.empty()) {
            registers[reg_id] = val;
        }
    }

    /**
     * Mitigates timing-based side-channel attacks.
     * Introduces controlled non-deterministic delay.
     */
    void security_delay() {
        volatile int garbage = 0;
        int limit = rand() % 32;
        for(int i = 0; i < limit; ++i) {
            garbage += i;
            __builtin_ia32_pause(); 
        }
    }

    void write_io_port(uintptr_t addr, uint8_t val) {
        std::cout << "[IO_BUS] Writing to 0x" << std::hex << addr 
                  << " -> " << std::dec << (int)val << std::endl;
    }
};

} // namespace nanlang
