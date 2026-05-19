import project::dir::mod::{sum};

export fn sum_i32(a: i32, b: i32) {
    return sum(a, b);
}

export fn sum_i64(a: i64, b: i64) {
    return a + b;
}

struct T {
    var member = 43;
    fn method(&self) { return 4; }

    fn construct(): T {
        var instance: T;
        return instance;
    }
}

fn instance_taker(ref: &T) {
    return ref.method();
}