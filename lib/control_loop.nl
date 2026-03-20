// control_loop.nl

fn wait_until_high(addr: potential, limit: potential) -> signal {
    let mut ticks: potential = 0;
    loop {
        if ticks == limit {
            return _;
        }

        let value: potential = @addr;
        if value > 0 {
            return ^;
        }

        jitter;
        ticks = ticks + 1;
    }
}

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

fn run_simple_controller(sensor_addr: potential, out_addr: potential) -> void {
    let ready: signal = wait_until_high(sensor_addr, 32);
    if ready {
        @out_addr = 1;
        pulse_n(^, 8);
    } else {
        @out_addr = 0;
        pulse_n(_, 4);
    }
}
