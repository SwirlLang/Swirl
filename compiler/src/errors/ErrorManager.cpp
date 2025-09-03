#include "errors/ErrorManager.h"
#include "errors/ErrorPipeline.h"


void ErrorManager::newError(const ErrCode code, const ErrorContext &ctx) const {
    m_OutputPipeline->write(generateMessage(code, ctx), ctx);
}


void ErrorManager::newErrorLocked(const ErrCode code, const ErrorContext &ctx) {
    std::lock_guard guard(m_Mutex);
    m_OutputPipeline->write(generateMessage(code, ctx), ctx);
}


void ErrorManager::flush() const {
    m_OutputPipeline->flush();
}


[[nodiscard]]
bool ErrorManager::errorOccurred() const {
    return m_OutputPipeline->errorOccurred();
}

