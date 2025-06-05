
export fn another_sum(a: i32, b: i32) {
    return sum(a, b);
}

fn get_element_test(i: i64) {
    var a: [u32; 4] = [
        43,
        43,
        43,
        43
    ];

    return a[0];
}

fn get_eight(i: i32): i32 {
    return another_sum(
        get_element_test(i) as i32,
        get_element_test(i + 1) as i32
    );
}

