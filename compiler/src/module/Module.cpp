#include "CompilerInst.h"
#include "modules/Module.h"
#include "sema/SymbolResolver.h"
#include "sema/TypeResolver.h"
#include "modules/ModuleManager.h"
#include "sema/SymbolRegistrationPass.h"


Module::Module(const ModuleContext& context)
    : symbol_table(context.file_handle, context.module_manager)
    , file_handle(context.file_handle)
    , m_ModuleManager(context.module_manager)
    , m_StringPool(context.string_pool)
    , m_CtxCopy(context)
{}


void Module::performSema(const ErrorCallback_t& error_callback) {
    if (m_IsSemaComplete) {
        return;
    }

    sema::SymbolRegistrationPass symbol_registration_pass{this, error_callback};
    symbol_registration_pass.dispatch(ast);

    if (!symbol_registration_pass.errorsOccurred()) {
        sema::SymbolResolver symbol_resolver{this, error_callback};
        symbol_resolver.dispatch(ast, sema::SymbolResolver::Data{});

        if (!symbol_resolver.errorsOccurred()) {
            sema::TypeResolver type_resolver{this, error_callback};
            type_resolver.dispatch(ast);
        }
    }

    m_IsSemaComplete = true;
}


void Module::decrementUnresolvedDeps() {
    unresolved_deps--;
    if (unresolved_deps == 0) {
        m_ModuleManager.m_BackBuffer.push_back(this);
    }
}
