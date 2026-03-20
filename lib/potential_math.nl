// potential_math.nl
// 8-bit potential arithmetic helpers.

fn mean2(a: potential, b: potential) -> potential {
    return (a + b) / 2;
}

fn mean3(a: potential, b: potential, c: potential) -> potential {
    return (a + b + c) / 3;
}

fn abs_delta(a: potential, b: potential) -> potential {
    if a > b {
        return a - b;
    } else {
        return b - a;
    }
}

fn clamp_max(v: potential, maxv: potential) -> potential {
    if v > maxv {
        return maxv;
    } else {
        return v;
    }
}
