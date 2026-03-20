// io_registers.nl
// Minimal memory-mapped IO helpers (8-bit address space style).

const REG_STATUS: potential = 0x10;
const REG_CONTROL: potential = 0x11;
const REG_DATA: potential = 0x12;

fn read_status() -> potential {
    return @REG_STATUS;
}

fn write_control(value: potential) -> void {
    @REG_CONTROL = value;
}

fn write_data(value: potential) -> void {
    @REG_DATA = value;
}

fn pulse_if_ready() -> void {
    let status: potential = @REG_STATUS;
    if status > 0 {
        pulse ^;
    } else {
        pulse _;
    }
}
