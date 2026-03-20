// signal_core.nl
// Core signal composition helpers.

fn signal_and(a: signal, b: signal) -> signal {
    return a ^&^ b;
}

fn signal_merge(a: signal, b: signal) -> signal {
    return a _?_ b;
}

fn signal_propagate(src: signal, gate: signal) -> signal {
    return src >>> gate;
}

fn signal_is_high(line: signal) -> signal {
    if line {
        return ^;
    } else {
        return _;
    }
}
