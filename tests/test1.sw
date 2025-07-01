import mod;

struct A {
    let a: i32 = 32;
    let b = 32;
};

fn main(a: i32, b: i64) {

    if (a == b) { var me: i32 = 32; }
    var me: i32 = 32;
    var test = me;
    return mod::sum_i32(a, b as i32);
}

fn main_2(a: i32, b: i32) {
    var me: i32 = 32;
    var test = me;
    return mod::sum_i64(a, b);
}