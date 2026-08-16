
// ===================================================================
//  Protocol conformance stress test
//  Exercises the protocol-impl checker + associated-type substitution
//  (bare aliases, compositions, generics, negative cases at the bottom).
// ===================================================================

protocol Stuff {  fn stuff(): i32; }

protocol Printable {
    fn print(): Stuff;
}

struct Console {}

impl Stuff for Console {
    fn stuff(): i32 { return 9; }
}

impl Printable for Console {
    fn print(): Console {}
}

// ---- 1. Bare associated type: impl binds a concrete type -----------
protocol Writable {
    type associate_type;
    fn write(content: str);
    fn get_handle(): associate_type;
}

struct File {}

impl Writable for File {
    type associate_type = i32;
    fn write(content: str) {}
    fn get_handle(): i32 { return 0; }
}

// ---- 2. Impl writes the alias name itself instead of the concrete type
protocol Iterable {
    type item_type;
    fn next(): item_type;
    fn count(): i32;
}

struct Vec {}

impl Iterable for Vec {
    type item_type = i32;
    fn next(): item_type { return 0; }
    fn count(): i32 { return 0; }
}

// ---- 3. Pointer composition over the associated type ----------------
protocol Borrowable {
    type payload_type;
    fn borrow(): *payload_type;
}

struct Buffer {}

impl Borrowable for Buffer {
    type payload_type = i32;
    fn borrow(): *i32 {}
}

// ---- 4. Slice composition over the associated type ------------------
protocol Viewable {
    type elem_type;
    fn as_slice(): &[elem_type];
}

struct Rings {}

impl Viewable for Rings {
    type elem_type = i32;
    fn as_slice(): &[i32] {}
}

// ---- 5. Fixed-size array composition over the associated type -------
protocol FixedSize {
    type slot_type;
    fn slots(): [slot_type | 4];
}

struct Grid {}

impl FixedSize for Grid {
    type slot_type = i32;
    fn slots(): [i32 | 4] {}
}

// ---- 6. Generic instantiation over the associated type --------------
struct Box<T> {}

protocol Boxable {
    type inner_type;
    fn boxed(): Box!{inner_type};
}

struct Container {}

impl Boxable for Container {
    type inner_type = i32;
    fn boxed(): Box!{i32} {}
}

// ---- 7. Multi-parameter generic + two associated types --------------
struct Pair<A, B> {}

protocol TwoAliases {
    type a_type;
    type b_type;
    fn comb(): Pair!{b_type, a_type};
    fn first(): a_type;
}

struct Thing {}

impl TwoAliases for Thing {
    type a_type = i32;
    type b_type = f64;
    fn comb(): Pair!{f64, i32} {}
    fn first(): i32 { return 1; }
}

// ---- 8. Protocol method with a default body -------------------------
protocol Defaulted {
    type t_type;
    fn defaulted_fn(): t_type { }
}

struct Def {}

impl Defaulted for Def {
    type t_type = i32;
    fn defaulted_fn(): i32 { return 42; }
}

// ---- 9. Void method (return type omitted on both sides) -------------
protocol Emitter {
    type e_type;
    fn emit(value: e_type);
    fn emit_default(): e_type;
}

struct Sink {}

impl Emitter for Sink {
    type e_type = i32;
    fn emit(value: i32) {}
    fn emit_default(): i32 { return 0; }
}

// ===================================================================
//  NEGATIVE cases (uncomment one at a time; each should produce
//  a PROTOCOL_VIOLATED / TYPE_ALIAS_REQUIRED error):
//
//  1. missing method:
//     impl Writable for File { type associate_type = i32; fn get_handle(): i32 { return 0; } }
//
//  2. wrong arity:
//     impl Writable for File { type associate_type = i32; fn write(content: str, extra: i32) {} fn get_handle(): i32 { return 0; } }
//
//  3. wrong concrete return type:
//     impl Writable for File { type associate_type = i32; fn write(content: str) {} fn get_handle(): f64 { return 0.0; } }
//
//  4. missing associated type:
//     impl Writable for File { fn write(content: str) {} fn get_handle(): i32 { return 0; } }
//
//  5. wrong param type:
//     impl Writable for File { type associate_type = i32; fn write(content: i64) {} fn get_handle(): i32 { return 0; } }
// ===================================================================

fn main() {
    var c: Console;
    var f: File;
    var v: Vec;
    var b: Buffer;
    var r: Rings;
    var g: Grid;
    var k: Container;
    var t: Thing;
    var d: Def;
    var s: Sink;
    // println("protocols loaded");
}
