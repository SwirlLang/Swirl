#pragma once
#define SW_PRE_VISIT_IMPL_HOOK(x)  self->pushNodeToStack(x)
#define SW_POST_VISIT_IMPL_HOOK(x) self->popNodeFromStack(x)

#include <utility>
#include <mutex>

#include "parser/Parser.h"
#include "ast/Visitor.h"
#include "errors/ErrorManager.h"
#include "modules/ModuleManager.h"


#define SEMA_DISABLE_ERROR_CODE(code) \
    auto GET_UNIQUE_NAME(err_code_disabler) = SemaVisitor<decltype(*this)>::DisableErrorCode(*this, code)

namespace sema {
class GlobalCache {
public:
    void insert(Node* node) {
        auto guard = std::lock_guard(m_Mutex);
        m_VisitedNodes.insert(node);
    }

    bool contains(Node* node) {
        auto guard = std::lock_guard(m_Mutex);
        return m_VisitedNodes.contains(node);
    }

private:
    std::mutex m_Mutex;
    std::unordered_set<Node*> m_VisitedNodes;
};


template <typename T>
class SemaVisitor : public RecursiveVisitor<T> {
public:
    explicit
    SemaVisitor(Module* module, ErrorCallback_t error_callback)
        : m_Callback(std::move(error_callback))
        , m_Module(module)
    {
        module->symbol_table.setErrorCallback([this](const ErrCode code, ErrorContext ctx) {
            reportError(code, std::move(ctx));
        });
    }


    void reportError(const ErrCode code, ErrorContext context) {
        if (m_DisabledErrorCodes.contains(code))
            return;

        m_ErrorOccurred = true;
        context.module = m_Module;

        if (!context.location.has_value()) {
            assert(!m_NodeStack.empty());
            context.location = m_NodeStack.back()->location;
        } m_Callback(code, std::move(context));
    }

    bool errorsOccurred() const {
        return m_ErrorOccurred;
    }

    friend class RecursiveVisitor<T>;

    struct DisableErrorCode {
        DisableErrorCode(SemaVisitor& instance,  ErrCode code)
            : m_Instance(instance)
            , m_ErrorCode(code)
        {
            if (!instance.m_DisabledErrorCodes.contains(code)) {
                m_Instance.m_DisabledErrorCodes.insert(code);
            }
        }

        ~DisableErrorCode() {
            m_Instance.m_DisabledErrorCodes.erase(m_ErrorCode);
        }


    private:
        SemaVisitor& m_Instance;
        ErrCode      m_ErrorCode;
    };

private:
    std::vector<Node*> m_NodeStack;
    ErrorCallback_t    m_Callback;
    Module*            m_Module;

    std::unordered_set<ErrCode> m_DisabledErrorCodes;

    bool m_ErrorOccurred = false;

    void pushNodeToStack(Node* node) {
        m_NodeStack.push_back(node);
    }


    void popNodeFromStack(Node*) {
        m_NodeStack.pop_back();
    }
};}
