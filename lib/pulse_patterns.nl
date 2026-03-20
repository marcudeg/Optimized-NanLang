// pulse_patterns.nl
// Reusable pulse emission patterns.

fn emit_clock(cycles: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == cycles {
            break;
        }
        pulse ^;
        pulse _;
        i = i + 1;
    }
}

fn emit_high_burst(count: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == count {
            break;
        }
        pulse ^;
        emit ^;
        i = i + 1;
    }
}

fn emit_low_burst(count: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == count {
            break;
        }
        pulse _;
        emit _;
        i = i + 1;
    }
}
