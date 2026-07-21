import project::mod;

struct T {
    fn method(&self) { return 4; }
}

fn instance_taker(ref: &T) {
    var s = mod::sum_i32(32, 64);
    return ref.method();
}

enum ENUM : i64 { A, B, C, D }

extern "C" fn write(fd: c_int, buf: *char, count: c_size_t): c_ssize_t;

fn generic_function<T>() {
    return T::function();
}

struct GenericStructArg_t {
    fn function() {
        return "HELLO WORLD";
    }
}

fn variadic_print(... args: str) {
    comptime for arg in args {
        write(1, arg.ptr(), arg.size());
    }
}

fn main() {
    let e = "";
    var instance: T;
    var my_stuff = instance.method();
    var i = instance_taker(&instance);
    mod::println("Hello, World!");
    variadic_print(generic_function!{GenericStructArg_t}());
    return ENUM::A;
}