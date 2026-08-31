#include <filesystem>
#include <ranges>
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
const auto CrossModRoot =
    std::filesystem::path(__FILE__).parent_path() / "test_files" / "cross_mod";
}


TEST_CASE("Cross-module impl export gating", "[sema][cross-module]") {
    const auto Triple = CompilerInst::Target.getTriple();

    struct Fixture {
        sw::FileSystem  fs;
        sw::StringPool  pool{4096};
        ModuleManager   modman;
        std::vector<std::pair<ErrCode, ErrorContext>> errors;
        sw::TypeManager type_manager;

        Fixture()
            : modman(pool, CompilerInst::Target, type_manager)
            , type_manager(modman)
        {}
    };

    Fixture fx;
    fx.fs.createVirtualFile(SW_BUILTIN_FILE_PATH, SW_BUILTIN_SOURCE);

    // register the package so that `import testpkg::dir::mod` resolves to
    // the real files under test_files/cross_mod
    CompilerInst::PackageTable.erase("testpkg");
    CompilerInst::addPackageEntry(CrossModRoot.string() + ":testpkg", true);

    auto* fh = fx.fs.open(CrossModRoot / "main.sw");
    const ModuleContext ctx{fh, fx.modman, fx.pool, CompilerInst::Target, fx.type_manager};
    auto* mod = fx.modman.insert(ctx);
    mod->parse([&fx](ErrCode code, ErrorContext e) {
        fx.errors.emplace_back(code, std::move(e));
    });

    // run sema dependency-first (mirrors CompilerInst::compile's batch loop)
    while (!fx.modman.zeroVecIsEmpty()) {
        while (const auto m = fx.modman.popZeroDepVec()) {
            sema::TypeResolver::VisitedNodes.clear();
            m->performSema([&fx](ErrCode code, ErrorContext e) {
                fx.errors.emplace_back(code, std::move(e));
            });
        }
        fx.modman.swapBuffers();
    }

    CompilerInst::PackageTable.erase("testpkg");

    // gi.greet(), g.make() and Gizmo::make() must each fail with
    // PROTO_IMPL_NOT_EXPORTED; c.bump() and Gizmo::build() must resolve
    // cleanly (so exactly 3 errors, all of the same kind).
    CHECK(fx.errors.size() == 3);
    for (const auto& code: fx.errors | std::views::keys) {
        CHECK(code == ErrCode::PROTO_IMPL_NOT_EXPORTED);
    }
}
