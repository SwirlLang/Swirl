#include "CompilerInst.h"
#include "modules/Module.h"
#include "sema/SymbolResolver.h"
#include "sema/TypeResolver.h"
#include "modules/ModuleManager.h"


Module::Module(const ModuleContext& context)
    : symbol_table(context.file_handle, context.module_manager)
    , file_handle(context.file_handle)
    , m_ModuleManager(context.module_manager)
    , m_StringPool(context.string_pool)
    , m_CtxCopy(context)
{}


// TODO: to be removed once Sema gets decoupled from the module
void Module::performSema(const ErrorCallback_t& error_callback) {
    if (m_IsSemaComplete) {
        return;
    }

    sema::SymbolResolver resolver{this, error_callback};
    resolver.dispatch(ast, sema::SymbolResolver::Data{});

    if (!resolver.errorsOccurred()) {
        sema::TypeResolver type_resolver{this, error_callback};
        type_resolver.dispatch(ast);
    }

    m_IsSemaComplete = true;
}


void Module::decrementUnresolvedDeps() {
    unresolved_deps--;
    if (unresolved_deps == 0) {
        m_ModuleManager.m_BackBuffer.push_back(this);
    }
}
