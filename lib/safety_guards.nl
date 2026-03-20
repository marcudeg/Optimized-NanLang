// safety_guards.nl
// Runtime safety and guardrail helpers.

fn safe_div(numerator: potential, denominator: potential) -> potential {
    if denominator == 0 {
        return 0;
    } else {
        return numerator / denominator;
    }
}

fn bounded_write(addr: potential, value: potential, maxv: potential) -> void {
    if value > maxv {
        @addr = maxv;
    } else {
        @addr = value;
    }
}

fn fail_fast(healthy: signal) -> void {
    if healthy {
        // healthy path
    } else {
        halt;
    }
}
