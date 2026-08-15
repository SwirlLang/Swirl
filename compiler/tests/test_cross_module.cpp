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
#include "Target.h"


namespace {
const auto CrossModRoot =
    std::filesystem::path(__FILE__).parent_path() / "test_files" / "cross_mod";
}


TEST_CASE("Cross-module impl export gating", "[sema][cross-module]") {
    sw::FileSystem  fs;
    sw::StringPool  pool{4096};
    ModuleManager   modman;
    std::vector<std::pair<ErrCode, ErrorContext>> errors;
    sw::Target Target;
    sw::Target::Triple_t Triple = Target.getTriple();
    fs.createVirtualFile(SW_BUILTIN_FILE_PATH, SW_BUILTIN_SOURCE);

    // register the package so that `import testpkg::dir::mod` resolves to
    // the real files under test_files/cross_mod
    CompilerInst::PackageTable.erase("testpkg");
    CompilerInst::addPackageEntry(CrossModRoot.string() + ":testpkg", true);

    auto* fh = fs.open(CrossModRoot / "main.sw");
    const ModuleContext ctx{fh, modman, pool, CompilerInst::Target};
    auto* mod = modman.insert(ctx);
    mod->parse([&errors](ErrCode code, ErrorContext e) {
        errors.emplace_back(code, std::move(e));
    });

    // run sema dependency-first (mirrors CompilerInst::compile's batch loop)
    while (!modman.zeroVecIsEmpty()) {
        while (const auto m = modman.popZeroDepVec()) {
            m->performSema([&errors](ErrCode code, ErrorContext e) {
                errors.emplace_back(code, std::move(e));
            });
        }
        modman.swapBuffers();
    }

    CompilerInst::PackageTable.erase("testpkg");

    // gi.greet(), g.make() and Gizmo::make() must each fail with
    // PROTO_IMPL_NOT_EXPORTED; c.bump() and Gizmo::build() must resolve
    // cleanly (so exactly 3 errors, all of the same kind).
    CHECK(errors.size() == 3);
    for (const auto& code: errors | std::views::keys) {
        CHECK(code == ErrCode::PROTO_IMPL_NOT_EXPORTED);
    }
}
