import project::mod;

struct T {
    fn method(&self) { return 4; }
}

fn instance_taker(ref: &T) {
    var s = mod::sum_i32(32, 64);
    return ref.method();
}

fn main() {
    var instance: T;
    var my_stuff = instance.method();
    var i = instance_taker(&instance);
}