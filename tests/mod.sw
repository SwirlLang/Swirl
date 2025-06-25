import dir::mod;

export fn sum_i32(a: i32, b: i32) {
    return mod::sum(a, b);
}

export fn sum_i64(a: i64, b: i64) {
    return a + b;
}
