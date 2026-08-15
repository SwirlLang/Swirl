#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include "modules/Module.h"
#include "modules/ModuleManager.h"
#include "utils/FileSystem.h"
#include "utils/StringPool.h"
#include "builtins/builtins.h"
#include "errors/ErrorManager.h"

struct SemaFixture {
    sw::FileSystem  fs;
    sw::StringPool  pool{4096};
    ModuleManager   modman;
    Module*         mod;

    std::vector<std::pair<ErrCode, ErrorContext>> errors;
    sw::Target      target{sw::Target::fromHostTriple()};

    explicit SemaFixture(std::string_view source) {
        const auto Triple = target.getTriple();
        fs.createVirtualFile(SW_BUILTIN_FILE_PATH, SW_BUILTIN_SOURCE);

        auto* fh = fs.createVirtualFile("test.sw", std::string(source));
        const ModuleContext ctx{fh, modman, pool, target};
        mod = modman.insert(ctx);

        mod->parse([this](ErrCode code, ErrorContext ctx) {
            errors.emplace_back(code, std::move(ctx));
        });

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
fn run() { var x = identity!{i32}(42); }
)");
    CHECK_FALSE(f.hasErrors());
}

TEST_CASE("Generic struct usage", "[sema][generic][struct]") {
    SemaFixture f(R"(
struct Box<T> { var value: T; }
fn run() { var b: Box!{i32}; }
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
