// diagnostics.nl
// Runtime diagnostics and health probes.

fn report_boot() -> void {
    say "nanlib: boot ok";
}

fn report_fault() -> void {
    say "nanlib: fault";
}

fn probe_line(line: signal) -> signal {
    if line {
        say "line high";
        return ^;
    } else {
        say "line low";
        return _;
    }
}

fn pulse_healthcheck(samples: potential) -> signal {
    if samples > 0 {
        return ^;
    } else {
        return _;
    }
}
