// telemetry.nl
// Example: telemetry sampler with signal thresholding.

fn classify(value: potential, limit: potential) -> signal {
    if value > limit {
        return ^;
    } else {
        return _;
    }
}

fn main() -> void {
    let sample_a: potential = @0x30;
    let sample_b: potential = @0x31;
    let avg: potential = (sample_a + sample_b) / 2;
    let alarm: signal = classify(avg, 120);

    if alarm {
        say "telemetry: high";
        pulse ^;
        emit ^;
    } else {
        say "telemetry: normal";
        pulse _;
        emit _;
    }
}
