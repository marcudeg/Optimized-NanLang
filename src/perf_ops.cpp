#include "../include/nan_perf.h"
#include <iostream>

namespace nanlang {

void run_waitfree_demo(int count) {
    WaitFreeQueue<uint64_t, 256> q;
    int pushed = 0, popped = 0;
    uint64_t v;

    for (int i = 0; i < count; ++i)
        if (q.push(static_cast<uint64_t>(i))) ++pushed;

    while (q.pop(v)) ++popped;

    std::cout << "[WAITFREE] count=" << count
              << " pushed=" << pushed
              << " popped=" << popped << "\n";
}

void run_deadlock_demo() {
    DeadlockDetector dd;
    // Thread 0 → resource 0
    dd.try_acquire(0, 0);
    // Thread 1 → resource 1
    dd.try_acquire(1, 1);
    // Thread 0 → resource 1 (waits on T1)
    bool ok0 = dd.try_acquire(0, 1);
    // Thread 1 → resource 0 (deadlock!)
    bool ok1 = dd.try_acquire(1, 0);
    std::cout << "[DEADLOCK] T0-R1=" << ok0 << " T1-R0=" << ok1
              << " (cycle expected=0,0)\n";
}

void run_branch_predictor_demo(int rounds) {
    BranchPredictor bp;
    int correct = 0;

    for (int i = 0; i < rounds; ++i) {
        bool taken = (i % 3 != 0); // deterministik pattern
        bool pred  = bp.predict(static_cast<uint8_t>(i));
        if (pred == taken) ++correct;
        bp.update(static_cast<uint8_t>(i), taken);
    }
    std::cout << "[BRANCH] rounds=" << rounds
              << " correct=" << correct
              << " accuracy=" << (100.0 * correct / rounds) << "%\n";
}

void run_pipeline_reorder_demo() {
    using Inst = PipelineReorder::Inst;
    // op, src1, src2, dst
    std::vector<Inst> prog = {
        {0x05, 1, 2, 3},  // ADD R3 = R1+R2
        {0x05, 3, 4, 5},  // ADD R5 = R3+R4  (dep on R3)
        {0x09, 0, 1, 6},  // XOR R6 = R0^R1  (independent)
        {0x06, 6, 2, 7},  // SUB R7 = R6-R2  (dep on R6)
    };

    auto reordered = PipelineReorder::reorder(prog);
    std::cout << "[PIPELINE] original=" << prog.size()
              << " reordered=" << reordered.size() << " instructions\n";
}

void run_ooo_demo() {
    ReservationStation rs;
    int s0 = rs.issue(0x05, 10, 20); // ADD 10+20
    int s1 = rs.issue(0x09,  7,  3); // XOR 7^3
    rs.execute_ready();
    std::cout << "[OOO] stations issued=" << (s0 >= 0 ? 1 : 0) + (s1 >= 0 ? 1 : 0)
              << " result_bus=" << rs.result_bus << "\n";
}

} // namespace nanlang
