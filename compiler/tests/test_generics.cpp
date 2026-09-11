#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include "modules/Module.h"
#include "modules/ModuleManager.h"
#include "utils/FileSystem.h"
#include "utils/StringPool.h"
#include "builtins/builtins.h"
#include "errors/ErrorManager.h"
#include "types/TypeManager.h"
#include "sema/TypeResolver.h"

struct SemaFixture {
    sw::FileSystem  fs;
    sw::StringPool  pool{4096};
    ModuleManager   modman;
    Module*         mod;
    sw::TypeManager type_manager;

    std::vector<std::pair<ErrCode, ErrorContext>> errors;
    sw::Target      target{sw::Target::fromHostTriple()};

    explicit SemaFixture(const std::string_view source)
        : modman(pool, target, type_manager)
        , type_manager(modman)
    {
        const auto Triple = target.getTriple();
        fs.createVirtualFile(SW_BUILTIN_FILE_PATH, SW_BUILTIN_SOURCE);

        auto* fh = fs.createVirtualFile("test.sw", std::string(source));
        const ModuleContext ctx{fh, modman, pool, target, type_manager};
        mod = modman.insert(ctx);

        mod->parse([this](ErrCode code, ErrorContext ctx) {
            errors.emplace_back(code, std::move(ctx));
        });

        sema::TypeResolver::VisitedNodes.clear();
        mod->performSema([this](ErrCode code, ErrorContext ctx) {
            errors.emplace_back(code, std::move(ctx));
        });
    }

    bool hasErrors() const { return !errors.empty(); }
};

bool hasError(const std::vector<std::pair<ErrCode, ErrorContext>>& errors, ErrCode code) {
    return std::ranges::find_if(errors, [&](const auto& e) { return e.first == code; }) != errors.end();
}

TEST_CASE("Simple function", "[sema][baseline]") {
    SemaFixture f(R"(
fn add(a: i32, b: i32): i32 { return a + b; }
fn run() { var x = add(1, 2); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("DOT method call", "[sema][dot]") {
    SemaFixture f(R"(
struct T { fn method(&self): i32 { return 42; } }
fn run() { var instance: T; var result = instance.method(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Struct field access via DOT", "[sema][field]") {
    SemaFixture f(R"(
struct Point {
    var x: i32;
    var y: i32;
}
fn run() { var p: Point; var v = p.x; }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Method accessing self fields", "[sema][self]") {
    SemaFixture f(R"(
struct Point {
    var x: i32;
    var y: i32;
    fn sum(&self): i32 { return self.x + self.y; }
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Function with 4 params", "[sema][params]") {
    SemaFixture f(R"(
fn sum4(a: i32, b: i32, c: i32, d: i32): i32 { return a + b + c + d; }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Function call", "[sema][call]") {
    SemaFixture f(R"(
fn add(a: i32, b: i32): i32 { return a + b; }
fn run() { var r = add(1, 2); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Static method call", "[sema][static]") {
    SemaFixture f(R"(
struct Factory {
    fn create(): Factory { var tmp: Factory; return tmp; }
}
fn run() { var f = Factory::create(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Enum declaration and access", "[sema][enum]") {
    SemaFixture f(R"(
enum Color { RED, GREEN, BLUE, }
fn run() { var c = Color::RED; }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic function instantiation", "[sema][generic]") {
    SemaFixture f(R"(
fn identity<T>(x: T): T { return x; }
fn run() { var x = identity!<i32>(42); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct usage", "[sema][generic][struct]") {
    SemaFixture f(R"(
struct Box<T> { var value: T; }
fn run() { var b: Box<i32>; }
)");
    CHECK_FALSE(f.hasErrors());
}

// ──────────────────────────────────────────────────────────
// Complex generic scenarios
// ──────────────────────────────────────────────────────────

TEST_CASE("Generic function instantiated with multiple distinct types", "[sema][generic][reuse]") {
    SemaFixture f(R"(
fn identity<T>(x: T): T { return x; }
fn run() {
    var a: i32 = identity!<i32>(42);
    var b: f64 = identity!<f64>(3.14);
    var c: bool = identity!<bool>(true);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Same generic instantiation reused across multiple call sites", "[sema][generic][reuse][cache]") {
    SemaFixture f(R"(
fn identity<T>(x: T): T { return x; }
fn run() {
    var a = identity!<i32>(1);
    var b = identity!<i32>(2);
    var c = identity!<i32>(3);
    var d = identity!<f64>(1.5);
    var e = identity!<f64>(2.5);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct instantiated with multiple distinct types", "[sema][generic][struct][reuse]") {
    SemaFixture f(R"(
struct Box<T> { var value: T; }
fn run() {
    var bi: Box<i32>;
    var bf: Box<f64>;
    var bb: Box<bool>;
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct type aliased through function usage", "[sema][generic][struct][reuse]") {
    SemaFixture f(R"(
struct Box<T> { var value: T; }
fn make<T>(x: T): Box<T> { var b: Box<T>; b.value = x; return b; }
fn run() {
    var b1: Box<i32> = make!<i32>(5);
    var b2: Box<i32> = make!<i32>(7);
    var b3: Box<f64> = make!<f64>(2.5);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct with methods on the instantiating type", "[sema][generic][struct][methods]") {
    SemaFixture f(R"(
struct Pair<T> {
    var first: T;
    var second: T;
    fn swap(&self): T { return self.second; }
}
fn run() {
    var p: Pair<i32>;
    var v: i32 = p.swap();
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct with static method", "[sema][generic][struct][methods][static]") {
    SemaFixture f(R"(
struct Pair<T> {
    var first: T;
    var second: T;
    fn create(a: T, b: T): Pair<T> { var p: Pair<T>; p.first = a; p.second = b; return p; }
}
fn run() {
    var p = Pair!<i32>::create(1, 2);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Multi-parameter generic struct", "[sema][generic][multi-param]") {
    SemaFixture f(R"(
struct Pair<A, B> {
    var first: A;
    var second: B;
}
fn run() {
    var p: Pair<i32, str>;
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Multi-parameter generic function", "[sema][generic][multi-param]") {
    SemaFixture f(R"(
fn make_pair<A, B>(a: A, b: B): Pair<A, B> { var p: Pair<A, B>; p.first = a; p.second = b; return p; }
struct Pair<A, B> { var first: A; var second: B; }
fn run() {
    var p = make_pair!<i32, str>(1, "x");
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic constrained by a single protocol", "[sema][generic][constraint]") {
    SemaFixture f(R"(
protocol Show { fn show(&self): str; }
fn make<T: Show>(x: T): T { return x; }
struct Impl {}
impl Show for Impl { fn show(&self): str { return "impl"; } }
fn run() {
    var i: Impl;
    var r = make!<Impl>(i);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic constrained by multiple protocols", "[sema][generic][constraint][multi]") {
    SemaFixture f(R"(
protocol P1 { fn a(&self): i32; }
protocol P2 { fn b(&self): i32; }
fn make<T: [P1, P2]>(x: T): T { return x; }
struct Impl {}
impl P1 for Impl { fn a(&self): i32 { return 1; } }
impl P2 for Impl { fn b(&self): i32 { return 2; } }
fn run() {
    var i: Impl;
    var r = make!<Impl>(i);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Nested generic instantiation", "[sema][generic][nested]") {
    SemaFixture f(R"(
struct Pair<A, B> { var first: A; var second: B; }
struct Box<T> { var value: T; }
fn run() {
    var b: Box<Pair<i32, str>>;
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Nested generic with function instantiation", "[sema][generic][nested]") {
    SemaFixture f(R"(
struct Box<T> { var value: T; }
fn pack<T>(x: T): Box<T> { var b: Box<T>; b.value = x; return b; }
fn wrap<U>(u: U): Box<U> { var b: Box<U>; b.value = u; return b; }
fn run() {
    var b1 = pack!<i32>(1);
    var b2 = wrap!<f64>(2.5);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Comptime local variable", "[sema][comptime]") {
    SemaFixture f(R"(
fn run() { comptime let x: i32 = 42; var y: i32 = x; }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Struct with only methods (no fields)", "[sema][onlymethods]") {
    SemaFixture f(R"(
struct M {
    fn foo(&self): i32 { return 1; }
    fn bar(&self): i32 { return 2; }
}
fn run() { var m: M; var a = m.foo(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Nested DOT field access", "[sema][nested]") {
    SemaFixture f(R"(
struct Inner { var value: i32; }
struct Outer { var inner: Inner; }
fn run() { var o: Outer; var v = o.inner.value; }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("DOT method call via protocol impl", "[sema][dot][protocol]") {
    SemaFixture f(R"(
protocol Greeter { fn greet(&self): i32; }
struct Console {}
impl Greeter for Console { fn greet(&self): i32 { return 7; } }
fn run() { var c: Console; var result = c.greet(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Static method call via protocol impl", "[sema][static][protocol]") {
    SemaFixture f(R"(
protocol Maker { fn make(): T; }
struct T {}
impl Maker for T { fn make(): T { var tmp: T; return tmp; } }
fn run() { var t = T::make(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("DOT method call ambiguous between two impls", "[sema][dot][protocol][ambiguous]") {
    SemaFixture f(R"(
protocol P { fn m(&self): i32; }
protocol Q { fn m(&self): i32; }
struct T {}
impl P for T { fn m(&self): i32 { return 1; } }
impl Q for T { fn m(&self): i32 { return 2; } }
fn run() { var t: T; var r = t.m(); }
)");
    const auto it = std::ranges::find_if(f.errors, [](const auto& e) {
        return e.first == ErrCode::AMBIGUOUS_MEMBER;
    });
    REQUIRE(it != f.errors.end());
}

TEST_CASE("DOT method call ambiguous between type scope and impl", "[sema][dot][protocol][ambiguous]") {
    SemaFixture f(R"(
protocol P { fn m(&self): i32; }
struct T { fn m(&self): i32 { return 0; } }
impl P for T { fn m(&self): i32 { return 1; } }
fn run() { var t: T; var r = t.m(); }
)");
    const auto it = std::ranges::find_if(f.errors, [](const auto& e) {
        return e.first == ErrCode::AMBIGUOUS_MEMBER;
    });
    REQUIRE(it != f.errors.end());
}

TEST_CASE("Static method call ambiguous via protocol impls", "[sema][static][protocol][ambiguous]") {
    SemaFixture f(R"(
protocol P { fn m(&self): i32; }
protocol Q { fn m(&self): i32; }
struct T {}
impl P for T { fn m(&self): i32 { return 1; } }
impl Q for T { fn m(&self): i32 { return 2; } }
fn run() { var t: T; var r = T::m(t); }
)");
    const auto it = std::ranges::find_if(f.errors, [](const auto& e) {
        return e.first == ErrCode::AMBIGUOUS_MEMBER;
    });
    REQUIRE(it != f.errors.end());
}

TEST_CASE("Impl method with value parameter via DOT", "[sema][dot][protocol][params]") {
    SemaFixture f(R"(
protocol Writer { fn write(&self, content: i32): i32; }
struct File {}
impl Writer for File { fn write(&self, content: i32): i32 { return content; } }
fn run() { var f: File; var r = f.write(42); }
)");
    CHECK_FALSE(f.hasErrors());
}


TEST_CASE("Multiple protocols on one type, both DOT-callable", "[sema][dot][protocol][multi-impl]") {
    SemaFixture f(R"(
protocol A { fn a(&self): i32; }
protocol B { fn b(&self): i32; }
struct T {}
impl A for T { fn a(&self): i32 { return 1; } }
impl B for T { fn b(&self): i32 { return 2; } }
fn run() { var t: T; var x = t.a(); var y = t.b(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Protocol with multiple methods, all DOT-callable", "[sema][dot][protocol][multi-method]") {
    SemaFixture f(R"(
protocol Shape { fn area(&self): i32; fn perimeter(&self): i32; }
struct Rect {}
impl Shape for Rect { fn area(&self): i32 { return 1; } fn perimeter(&self): i32 { return 2; } }
fn run() { var r: Rect; var a = r.area(); var p = r.perimeter(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Nested DOT through protocol impl member", "[sema][dot][protocol][nested]") {
    SemaFixture f(R"(
protocol Clickable { fn click(&self): i32; }
struct Button {}
impl Clickable for Button { fn click(&self): i32 { return 1; } }
struct Dialog { var button: Button; }
fn run() { var d: Dialog; var r = d.button.click(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Two types implementing the same protocol resolve independently", "[sema][dot][protocol][shared-protocol]") {
    SemaFixture f(R"(
protocol Speaker { fn speak(&self): i32; }
struct Dog {}
struct Cat {}
impl Speaker for Dog { fn speak(&self): i32 { return 1; } }
impl Speaker for Cat { fn speak(&self): i32 { return 2; } }
fn run() { var d: Dog; var c: Cat; var a = d.speak(); var b = c.speak(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Instance and static methods in one protocol, both callable", "[sema][static][protocol][combined]") {
    SemaFixture f(R"(
protocol Gadget { fn activate(&self): i32; fn count(): i32; }
struct G {}
impl Gadget for G { fn activate(&self): i32 { return 1; } fn count(): i32 { return 2; } }
fn run() { var g: G; var a = g.activate(); var c = G::count(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Non-exported impl is visible within the same module", "[sema][protocol][same-module]") {
    SemaFixture f(R"(
protocol P { fn m(&self): i32; }
struct T {}
impl P for T { fn m(&self): i32 { return 1; } }
fn run() { var t: T; var r = t.m(); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Impl missing a required protocol method", "[sema][protocol][violated]") {
    SemaFixture f(R"(
protocol Greeter { fn greet(&self): i32; fn farewell(&self): i32; }
struct T {}
impl Greeter for T { fn greet(&self): i32 { return 1; } }
)");
    CHECK(hasError(f.errors, ErrCode::PROTOCOL_VIOLATED));
}

TEST_CASE("Impl method with a mismatched return type", "[sema][protocol][mismatch]") {
    SemaFixture f(R"(
protocol Greeter { fn greet(&self): i32; }
struct T {}
impl Greeter for T { fn greet(&self): str { return "hi"; } }
)");
    CHECK(hasError(f.errors, ErrCode::PROTOCOL_METHOD_MISMATCH));
}

TEST_CASE("Impl method with the wrong kind (static vs instance)", "[sema][protocol][mismatch]") {
    SemaFixture f(R"(
protocol Greeter { fn greet(&self): i32; }
struct T {}
impl Greeter for T { fn greet(): i32 { return 1; } }
)");
    CHECK(hasError(f.errors, ErrCode::PROTOCOL_METHOD_MISMATCH));
}

TEST_CASE("DOT call on a type with no matching member or impl", "[sema][dot][protocol][no-member]") {
    SemaFixture f(R"(
struct T {}
fn run() { var t: T; var r = t.missing(); }
)");
    CHECK(hasError(f.errors, ErrCode::NO_SUCH_MEMBER));
}

TEST_CASE("Generic struct static method on multiple instantiations", "[sema][generic][struct][methods][static][reuse]") {
    SemaFixture f(R"(
struct Pair<T> {
    var first: T;
    var second: T;
    fn create(a: T, b: T): Pair<T> { var p: Pair<T>; p.first = a; p.second = b; return p; }
    fn empty(): Pair<T> { var p: Pair<T>; return p; }
}
fn run() {
    var pa = Pair!<i32>::create(1, 2);
    var pb = Pair!<f64>::create(1.0, 2.0);
    var pc = Pair!<i32>::empty();
    var pd = Pair!<i32>::create(3, 4);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct static method missing member on instantiation", "[sema][generic][struct][methods][static][no-member]") {
    SemaFixture f(R"(
struct Pair<T> {
    var first: T;
    fn create(): Pair<T> { var p: Pair<T>; return p; }
}
fn run() {
    var p = Pair!<i32>::missing(1, 2);
}
)");
    CHECK(hasError(f.errors, ErrCode::NO_SYMBOL_IN_NAMESPACE));
}


TEST_CASE("Generic struct generic method static access", "[sema][generic][struct][methods][static][own-param]") {
    SemaFixture f(R"(
struct Pair<T> {
    var first: T;
    var second: T;
    fn another<N>(a: T, b: N): Pair<T> { var p: Pair<T>; p.first = a; p.second = b; return p; }
}
fn run() {
    var b = Pair!<i32>::another!<i32>(4, 5);
}
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct generic method distinct instantiation combos", "[sema][generic][struct][methods][static][own-param][reuse]") {
    SemaFixture f(R"(
struct Pair<T> {
    var first: T;
    fn label<N>(a: T, b: N): N { return b; }
}
fn run() {
    var a = Pair!<i32>::label!<i32>(1, 2);
    var b = Pair!<i32>::label!<f64>(3, 4.0);
    var c = Pair!<f64>::label!<i32>(1.0, 5);
    var d = Pair!<i32>::label!<i32>(6, 7);
}
)");
    CHECK_FALSE(f.hasErrors());
}
