export struct Point {
    var x: i32;
    var y: i32;
}

export fn create_point(x: i32, y: i32): Point {
    var p: Point;
    p.x = x;
    p.y = y;
    return p;
}

struct Hidden {}
