#include "CompilerInst.h"
#include "modules/Module.h"

#include "modules/ModuleManager.h"
#include "sema/Sema.h"
#include "comptime/ComptimeEvaluator.h"
#include "generics/GenericInstantiator.h"


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

    // if (!evaluator.errorsOccurred()) {
    //     sw::GenericInstantiator instantiator{this, error_callback};
    //     instantiator.instantiateAllGenerics();
    // }
}


void Module::decrementUnresolvedDeps() {
    unresolved_deps--;
    if (unresolved_deps == 0) {
        m_ModuleManager.m_BackBuffer.push_back(this);
    }
}
