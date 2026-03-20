// timing_utils.nl
// Deterministic wait and timing-noise helpers.

fn wait_ticks(ticks: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == ticks {
            break;
        }
        jitter;
        i = i + 1;
    }
}

fn pulse_with_delay(line: signal, ticks: potential) -> void {
    pulse line;
    wait_ticks(ticks);
    emit line;
}

fn guarded_jitter(active: signal) -> void {
    if active {
        jitter;
    } else {
        // no-op branch
    }
}
