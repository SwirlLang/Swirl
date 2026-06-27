#pragma once
#include "ast/RecursiveVisitor.h"
#include "modules/Module.h"
#include "utils/logging.h"


namespace sema {
class SymbolRegistrationVerifier : public RecursiveVisitor<SymbolRegistrationVerifier> {
public:
    explicit SymbolRegistrationVerifier(Module* module)
        : m_Module(module) {}


    bool preVisit(Function*) {
        m_IsLocalScope = true;
        return true;
    }


    bool postVisit(Function*) {
        m_IsLocalScope = false;
        return true;
    }


    void handle(const Ident* ident) {
        if (!ident->value && !m_IsLocalScope && !m_Module->isErroneous()) {
            SW_LOG_FATAL(
                "SymbolRegistration: verification failed in {}. Local identifier at {} unresolved.",
                m_Module->file_handle->getPath(), ident->location.toString());
            m_IsErroneous = true;
        }
    }


    [[nodiscard]]
    bool isErroneous() const { return m_IsErroneous; }

private:
    Module* m_Module;
    bool    m_IsLocalScope = false;
    bool    m_IsErroneous  = false;
};
}