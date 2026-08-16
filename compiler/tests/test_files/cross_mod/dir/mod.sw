export protocol Greeter {
    fn greet(&self): str;
}

export struct GreeterImpl {
    var message: str;
}

impl Greeter for GreeterImpl {
    fn greet(&self): str {
        return self.message;
    }
}

export protocol Maker {
    fn make(): i32;
}

export struct Gizmo {}

impl Maker for Gizmo {
    fn make(): i32 {
        return 7;
    }
}

export protocol Incrementable {
    fn bump(&self): i32;
}

export struct Counter {
    var value: i32;
}

export impl Incrementable for Counter {
    fn bump(&self): i32 {
        return self.value + 1;
    }
}

export protocol Builder {
    fn build(): Gizmo;
}

export impl Builder for Gizmo {
    fn build(): Gizmo {
        var g: Gizmo;
        return g;
    }
}
