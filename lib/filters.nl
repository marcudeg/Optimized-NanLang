// filters.nl
// Simple signal/potential filtering primitives.

fn threshold(sample: potential, limit: potential) -> signal {
    return sample > limit;
}

fn hysteresis(sample: potential, low: potential, high: potential, prev: signal) -> signal {
    if sample > high {
        return ^;
    } else if sample < low {
        return _;
    } else {
        return prev;
    }
}

fn select_signal(flag: signal, on_high: signal, on_low: signal) -> signal {
    if flag {
        return on_high;
    } else {
        return on_low;
    }
}
