#pragma once
#include <utility>

#include "SymbolRegistrationPass.h"
#include "SymbolResolver.h"
#include "TypeResolver.h"


#define SW_SEMA_PIPELINE \
    SW_SEMA_PASS(SymbolRegistrationPass) \
    SW_SEMA_PASS(SymbolResolver, SymbolResolver::Data{}) \
    SW_SEMA_PASS(TypeResolver)


namespace sema {
class Sema {
public:
    Sema(Module* module, ErrorCallback_t error_callback)
        : m_Module(module), m_ErrorCallback(std::move(error_callback)) {}


    /// Performs sema on the entire module.
    void start() const {
    #define SW_SEMA_PASS(x, ...) \
        x x ## _inst{m_Module, m_ErrorCallback}; \
        x ## _inst.dispatch(m_Module->ast __VA_OPT__(,) __VA_ARGS__); \
        if (x ## _inst.errorsOccurred()) return;
        SW_SEMA_PIPELINE
    #undef SW_SEMA_PASS
    }


    /// Performs sema on the particular node.
    void start(Node* node) const {
    #define SW_SEMA_PASS(x, ...) \
        x x ## _inst{m_Module, m_ErrorCallback}; \
        x ## _inst.dispatch(node __VA_OPT__(,) __VA_ARGS__); \
        if (x ## _inst.errorsOccurred()) return;
        SW_SEMA_PIPELINE
    #undef SW_SEMA_PASS
    }


private:
    Module* m_Module;
    ErrorCallback_t m_ErrorCallback;
};
}