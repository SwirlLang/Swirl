import std::functional;


fn main() : f32 {
    struct T {
        var a: i32 = 2333;
        var b: i64 = 2333;
        var c: i32 = 0000001;
    }

    var instance: T = match b.a {
        33 {}
        34 {}
        24 {}
        _ {}
    };

    var array: [T] = [...];

    var config: [T] = array | std::filter((T a) {
        return a > 12;
    });


}

