import testpkg::dir::mod::{GreeterImpl, Gizmo, Counter};

fn run() {
    var gi: GreeterImpl;
    gi.greet();            // -> PROTO_IMPL_NOT_EXPORTED (DOT, unexported impl)

    var g: Gizmo;
    g.make();              // -> PROTO_IMPL_NOT_EXPORTED (DOT, unexported impl)
    Gizmo::make();         // -> PROTO_IMPL_NOT_EXPORTED (::, unexported impl)

    var c: Counter;
    c.bump();              // OK: exported impl via DOT
    Gizmo::build();        // OK: exported impl via ::
}
