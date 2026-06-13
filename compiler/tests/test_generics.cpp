#include <catch2/catch_test_macros.hpp>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

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

    explicit SemaFixture(std::string_view source) {
        using Triple = llvm::Triple;
        const auto LLVMTargetTriple = Triple(llvm::sys::getDefaultTargetTriple());
        fs.createVirtualFile(SW_BUILTIN_FILE_PATH, SW_BUILTIN_SOURCE);

        auto* fh = fs.createVirtualFile("test.sw", std::string(source));
        const ModuleContext ctx{fh, modman, pool};
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
