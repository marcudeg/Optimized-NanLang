// pulse_controller.nl
// Example: simple pulse controller.

fn pulse_n(line: signal, n: potential) -> void {
    let mut i: potential = 0;
    loop {
        if i == n {
            break;
        }
        pulse line;
        emit line;
        i = i + 1;
    }
}

fn main() -> void {
    let status: potential = @0x10;
    if status > 0 {
        say "controller: ready";
        pulse_n(^, 8);
    } else {
        say "controller: idle";
        pulse_n(_, 4);
    }
}
