#include "../include/nan_pulse.h"
#include <iostream>

namespace nanlang {

// AsyncPulseEmitter bulk emit and statistics module.

void run_pulse_emitter_demo(int count, uint8_t pattern) {
    AsyncPulseEmitter emitter;
    ClockCycleSync    clock;
    PulseFlowAnalyzer analyzer;
    std::vector<uint8_t> sample_trace;
    sample_trace.reserve(static_cast<size_t>(count > 0 ? count : 0));

    clock.calibrate();
    std::cout << "[PULSE] Emitting " << count << " signals, pattern=0x"
              << std::hex << (int)pattern << std::dec << "\n";

    for (int i = 0; i < count; ++i) {
        uint8_t signal = static_cast<uint8_t>((pattern + i) & 0xFF);
        emitter.emit(signal);
        sample_trace.push_back(signal);
    }

    analyzer.ingest(sample_trace);
    auto stats = analyzer.snapshot();

    uint64_t cycles = clock.elapsed();
    std::cout << "[PULSE] Done. Buffer pos=" << emitter.pos()
              << "  cycles=" << cycles
              << "  cycles/emit=" << (count ? cycles / count : 0) << "\n";
    std::cout << "[PULSE] Flow stats: high=" << stats.high_count
              << " low=" << stats.low_count
              << " uncertain=" << stats.uncertain_count
              << " rising=" << stats.rising_edges
              << " falling=" << stats.falling_edges
              << " toggles=" << stats.toggles
              << " longest_high=" << stats.longest_high_streak
              << " longest_low=" << stats.longest_low_streak << "\n";

    emitter.flush();
    std::cout << "[PULSE] Buffer flushed.\n";
}

void run_signal_buffer_demo(int items) {
    SignalBuffer buf;
    int pushed = 0, popped = 0;

    for (int i = 0; i < items; ++i) {
        if (buf.push(static_cast<uint8_t>(i & 0xFF))) ++pushed;
    }
    std::cout << "[SIGBUF] Pushed=" << pushed << " size=" << buf.size() << "\n";

    uint8_t v;
    while (buf.pop(v)) ++popped;
    std::cout << "[SIGBUF] Popped=" << popped << " empty=" << buf.empty() << "\n";
}

} // namespace nanlang
