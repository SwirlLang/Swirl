#include "CompilerInst.h"
#include "modules/Module.h"
#include "modules/ModuleManager.h"
#include "sema/Sema.h"


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

    const sema::Sema sema{this, error_callback};
    sema.start();

    m_IsSemaComplete = true;
}


void Module::decrementUnresolvedDeps() {
    unresolved_deps--;
    if (unresolved_deps == 0) {
        m_ModuleManager.m_BackBuffer.push_back(this);
    }
}
