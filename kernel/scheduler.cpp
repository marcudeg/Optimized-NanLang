#include <atomic>
#include <thread>
#include <vector>
#include <cstdint>

namespace nanlang {

/**
 * High-performance task distribution.
 * Implements a spin-lock barrier to synchronize signal 
 * processing across multiple CPU cores without context switching.
 */
class TaskScheduler {
    std::atomic<bool> sync_flag{false};
    std::vector<std::thread> thread_pool;

public:
    /**
     * Locks the execution flow for synchronization.
     * Faster than std::mutex for sub-microsecond tasks.
     */
    void core_sync() {
        while (sync_flag.exchange(true, std::memory_order_acquire)) {
            __builtin_ia32_pause(); 
        }
        // Critical section for signal alignment.
        sync_flag.store(false, std::memory_order_release);
    }

    void join_workers() {
        for(auto& t : thread_pool) if(t.joinable()) t.join();
    }
};

} // namespace nanlang
