#include <filesystem>
#include <ranges>
#include <string>
#include <vector>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "CompilerInst.h"
#include "modules/Module.h"
#include "modules/ModuleManager.h"
#include "utils/FileSystem.h"
#include "utils/StringPool.h"
#include "builtins/builtins.h"
#include "errors/ErrorManager.h"
#include "types/TypeManager.h"
#include "sema/TypeResolver.h"


namespace {
const auto ModulesRoot =
    std::filesystem::path(__FILE__).parent_path() / "test_files" / "modules";

constexpr std::string_view PackageAlias = "modpkg";
}


/// Runs parse + dependency-first sema (mirrors CompilerInst::compile's batch
/// loop) on the given main module, and reports the errors it produces.
struct ModuleSystemFixture {
    sw::FileSystem  fs;
    sw::StringPool  pool{4096};
    ModuleManager   modman;
    std::vector<std::pair<ErrCode, ErrorContext>> errors;
    sw::TypeManager type_manager;

    ModuleSystemFixture()
        : modman(pool, CompilerInst::Target, type_manager)
        , type_manager(modman)
    {
        const auto Triple = CompilerInst::Target.getTriple();
        fs.createVirtualFile(SW_BUILTIN_FILE_PATH, SW_BUILTIN_SOURCE);

        // register the package so that `import modpkg::…` resolves to the real
        // files under test_files/modules
        CompilerInst::PackageTable.erase(std::string(PackageAlias));
        CompilerInst::addPackageEntry(ModulesRoot.string() + ":" + std::string(PackageAlias), true);
    }

    ~ModuleSystemFixture() {
        CompilerInst::PackageTable.erase(std::string(PackageAlias));
    }

    /// Parse + sema a main module from the fixtures directory.
    void run(std::string_view main_file) {
        auto* fh = fs.open(ModulesRoot / main_file);
        const ModuleContext ctx{fh, modman, pool, CompilerInst::Target, type_manager};
        auto* mod = modman.insert(ctx);
        mod->parse([this](ErrCode code, ErrorContext e) {
            errors.emplace_back(code, std::move(e));
        });

        // run sema dependency-first (mirrors CompilerInst::compile's batch loop)
        while (!modman.zeroVecIsEmpty()) {
            while (const auto m = modman.popZeroDepVec()) {
                sema::TypeResolver::VisitedNodes.clear();
                m->performSema([this](ErrCode code, ErrorContext e) {
                    errors.emplace_back(code, std::move(e));
                });
            }
            modman.swapBuffers();
        }
    }

    /// How many of the collected errors carry the given code.
    [[nodiscard]]
    std::size_t errorCount(const ErrCode code) const {
        return std::ranges::count(errors | std::views::keys, code);
    }
};


// ──────────────────────────────────────────────────────────
// Positive cases: the import is fully usable
// ──────────────────────────────────────────────────────────

TEST_CASE("Import specific symbols from a module", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_specific_import.sw");
    CHECK(fx.errors.empty());
}

TEST_CASE("Import specific symbols with local aliases", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_aliased_sym_import.sw");
    CHECK(fx.errors.empty());
}

TEST_CASE("Import whole module and use its namespace qualifier", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_namespace_import.sw");
    CHECK(fx.errors.empty());
}

TEST_CASE("Import whole module under an alias", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_aliased_namespace_import.sw");
    CHECK(fx.errors.empty());
}

TEST_CASE("Wildcard import brings every exported symbol into scope", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_wildcard_import.sw");
    CHECK(fx.errors.empty());
}

TEST_CASE("Import through a multi-level path", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_deep_import.sw");
    CHECK(fx.errors.empty());
}

TEST_CASE("Import a struct type and a function from another module", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_struct_import.sw");
    CHECK(fx.errors.empty());
}


// ──────────────────────────────────────────────────────────
// Negative cases: symbol/export enforcement
// ──────────────────────────────────────────────────────────

TEST_CASE("Whole-module import does not leak unqualified symbols", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_symbol_after_mod_import.sw");
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 1);
}

TEST_CASE("Importing a non-exported symbol is rejected", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_unexported_import.sw");
    CHECK(fx.errorCount(ErrCode::SYMBOL_NOT_EXPORTED) == 1);
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 0);
}

TEST_CASE("Importing a symbol that doesn't exist is rejected", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_missing_symbol_import.sw");
    CHECK(fx.errorCount(ErrCode::SYMBOL_NOT_FOUND_IN_MOD) == 1);
    CHECK(fx.errors.size() == 1);
}

TEST_CASE("Using a non-exported symbol via the module namespace is rejected",
          "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_ns_unexported_access.sw");
    CHECK(fx.errorCount(ErrCode::SYMBOL_NOT_EXPORTED) == 1);
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 1);
}

TEST_CASE("Using a missing symbol via the module namespace is rejected",
          "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_ns_missing_member.sw");
    CHECK(fx.errorCount(ErrCode::NO_SYMBOL_IN_NAMESPACE) == 1);
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 1);
}

TEST_CASE("Aliased symbol import hides the original name", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_alias_shadows_original.sw");
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 1);
    CHECK(fx.errorCount(ErrCode::SYMBOL_NOT_EXPORTED) == 0);
}

TEST_CASE("Aliased module import hides the original namespace", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_alias_shadows_module.sw");
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 1);
}

TEST_CASE("Wildcard import does not create a module namespace", "[module][import][sema]") {
    ModuleSystemFixture fx;
    fx.run("main_wildcard_no_namespace.sw");
    CHECK(fx.errorCount(ErrCode::UNDEFINED_IDENTIFIER) == 1);
}
