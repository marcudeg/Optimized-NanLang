// interrupt_utils.nl
// Interrupt-oriented helper routines.

fn ack_interrupt(vector: potential) -> void {
    @0x20 = vector;
}

fn timer_tick_handler() -> void {
    pulse ^;
    emit ^;
    ack_interrupt(0x20);
}

fn io_irq_handler() -> void {
    pulse _;
    emit _;
    ack_interrupt(0x21);
}

fn guarded_irq_read(addr: potential) -> potential {
    on Error {
        return 0;
    }
    return @addr;
}
