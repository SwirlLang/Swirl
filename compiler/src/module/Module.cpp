#include "CompilerInst.h"
#include "modules/Module.h"

#include "modules/ModuleManager.h"
#include "sema/Sema.h"
#include "comptime/ComptimeEvaluator.h"
#include "transformers/GenericInstantiator.h"


Module::Module(const ModuleContext& context)
    : file_handle(context.file_handle)
    , type_manager(context.type_manager)
    , module_man(context.module_manager)
    , symbol_table(this)
    , m_StringPool(context.string_pool)
    , m_Target(context.target)
    , m_CtxCopy(context) {}


void Module::parse(const ErrorCallback_t& error_callback) {
    auto context = ParserContext{this, error_callback, module_man, m_StringPool};
    const auto parser = std::make_unique<Parser>(context);
    parser->parse();
}


void Module::performSema(const ErrorCallback_t& error_callback) {
    if (m_IsSemaComplete) {
        return;
    }

    // register all builtins with the type manager for the builtin module
    if (SW_BUILTIN_FILE_PATH == file_handle->getPath()) {
        for (auto& [name, type] : BuiltinTypes) {
            const auto id = symbol_table.getIdInfoOfAGlobal(std::string(name));
            type_manager.registerType(id, type);
        }
    }

    sema::Sema sema{this, error_callback};
    sema.start();

    if (!sema.errorsOccurred()) {
        performComptimeEval(error_callback);
    }

    m_IsSemaComplete = true;
}


void Module::performComptimeEval(const ErrorCallback_t& error_callback) {
    sw::ComptimeEvaluator evaluator{this, error_callback};
    ast = evaluator.run(ast);
}


void Module::decrementUnresolvedDeps() {
    unresolved_deps--;
    if (unresolved_deps == 0) {
        module_man.m_BackBuffer.push_back(this);
    }
}
