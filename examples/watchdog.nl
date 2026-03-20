// watchdog.nl
// Example: watchdog loop with fail-fast behavior.

fn pet_watchdog(port: potential) -> void {
    @port = 1;
    @port = 0;
}

fn main() -> void {
    let mut ticks: potential = 0;
    loop {
        let healthy: potential = @0x20;
        if healthy == 0 {
            say "watchdog: fault";
            halt;
        }

        pet_watchdog(0x21);
        ticks = ticks + 1;
        if ticks == 32 {
            say "watchdog: cycle complete";
            break;
        }
    }
}
