#pragma once
#define SW_PRE_VISIT_IMPL_HOOK(x)  self->pushNodeToStack(x)
#define SW_POST_VISIT_IMPL_HOOK(x) self->popNodeFromStack(x)

#include <utility>

#include "parser/Parser.h"
#include "ast/Visitor.h"
#include "errors/ErrorManager.h"


template <typename T>
class SemaVisitor : public RecursiveVisitor<T> {
public:
    explicit
    SemaVisitor(Parser& parser)
        : m_Callback(parser.m_ErrorCallback)
        , m_SourceManager(&parser.m_SrcMan)
    {
        parser.SymbolTable.setErrorCallback([this](const ErrCode code, ErrorContext ctx) {
            reportError(code, std::move(ctx));
        });
    }


    void reportError(const ErrCode code, ErrorContext context) const {
        if (m_DisabledErrorCodes.contains(code))
            return;

        context.src_man = m_SourceManager;
        if (!context.location.has_value()) {
            assert(!m_NodeStack.empty());
            context.location = m_NodeStack.back()->location;
        } m_Callback(code, std::move(context));
    }

    friend class RecursiveVisitor<T>;



private:
    std::vector<Node*> m_NodeStack;
    ErrorCallback_t    m_Callback;
    SourceManager*     m_SourceManager;

    std::unordered_set<ErrCode> m_DisabledErrorCodes;


    void pushNodeToStack(Node* node) {
        m_NodeStack.push_back(node);
    }


    void popNodeFromStack(Node*) {
        m_NodeStack.pop_back();
    }
};
