#pragma once
#include <mutex>
#include <utility>
#include <filesystem>
#include <unordered_set>

#include "definitions.h"
#include "ast/Nodes.h"
#include "lexer/Tokens.h"
#include "lexer/TokenStream.h"
#include "errors/ErrorManager.h"
#include "symbols/SymbolManager.h"

#include "ExpressionParser.h"
#include "utils/FileSystem.h"
#include "utils/StringPool.h"


class Parser;
class ModuleManager;
struct Module;

namespace sw   {
    class StringPool;
    class FileSystem;
}

namespace sema {
    template <typename T> class SemaVisitor;
}

/// A type which can represent either a `Type*` or a `Node*`.
struct SwObject : std::variant<Type*, Node*> {
    using std::variant<Type*, Node*>::variant;

    [[nodiscard]] Type* toType() const {
        assert(isType());
        return std::get<Type*>(*this);
    }

    [[nodiscard]] Node* toNode() const {
        assert(isNode());
        return std::get<Node*>(*this);
    }

    [[nodiscard]] bool isType() const {
        return std::holds_alternative<Type*>(*this);
    }

    [[nodiscard]] bool isNode() const {
        return std::holds_alternative<Node*>(*this);
    }
};


struct ParserContext {
    Module* module;
    ErrorCallback_t error_callback;
    ModuleManager&  module_manager;
    sw::StringPool& string_pool;
};


template <>
struct std::hash<SwObject> {
    std::size_t operator()(const SwObject& obj) const noexcept {
        return std::hash<std::variant<Type*, Node*>>{}(obj);
    }
};


class Parser {
    TokenStream      m_Stream;
    SourceManager    m_SrcMan;

    Module*          m_Module;
    ErrorCallback_t  m_ErrorCallback;  // the callback for reporting an error
    ExpressionParser m_ExpressionParser{*this};

    // ---*--- Flags  ---*---
    Function*    m_LatestFuncNode     = nullptr;
    bool         m_LastSymWasExported = false;
    bool         m_LastSymIsExtern    = false;

    std::vector<Type*>   m_CurrentStructTy{nullptr};  // the type of the struct being parsed

    std::string          m_ExternAttributes;
    Expression*          m_AttributeList = nullptr;
    // ---*--- ---*--- ---*---

    int                   m_RecursionDepth = 0;
    std::vector<SwObject> m_ParseStack;        // currently-being-parsed object pointer stays at the top

    sw::FileHandle*       m_FileHandle;

    // used for buffering error reports until the nodes/types have been completed
    std::unordered_map<SwObject, std::vector<std::tuple<ErrCode, ErrorContext>>> m_ErrorQueue;

    // maps the IdentInfo* of global nodes (nodes inheriting from `GlobalNode`) to where they begin and
    // where they end
    std::unordered_map<IdentInfo*, std::array<StreamState, 2>> m_GlobalOffsets;

    sw::FileSystem& m_FileSystem;
    sw::StringPool& m_StringPool;

    struct Bracket_t { char val{}; StreamState location; };
    std::vector<Bracket_t> m_BracketTracker;

    struct NodeAttrHelper;

    friend class CompilerInst;
    friend class ModuleManager;
    friend class ExpressionParser;
    friend class LLVMBackend;

    template <typename T>
    friend class sema::SemaVisitor;

public:
    ModuleManager& ModuleMap;
    std::unordered_map<IdentInfo*, Node*>& NodeJmpTable;  // maps global symbols to their nodes

    explicit Parser(const ParserContext& context);


    Node* dispatch();
    Node* parseFunction();
    Node* parseCondition();
    Node* parseWhile();
    Node* parseStruct();
    Node* parseImport();
    Node* parseScope();
    Node* parseRet();
    Node* parseIntrinsic();
    Node* parseProtocol();
    Node* parseEnum();

    Var*  parseParam(bool&);
    Node* parseVar(bool is_volatile = false);
    Node* parseCall(std::optional<Ident*> _ = std::nullopt);

    std::span<Ident*>         parseProtocolList();
    std::span<GenericParam*>  parseGenericParamList();

    Token forwardStream(uint8_t n = 1);

    Ident*           parseIdent();
    Expression*      parseExpr();
    TypeWrapper*     parseType();
    GenericArgList   parseGenericArgList();

    void parse();
    void ignoreButExpect(const Token&);
    void stackSafeguard() const;


    /// Buffers the reported errors, also sets certain context attributes automatically
    void reportError(const ErrCode code, ErrorContext ctx = {}) {
        ctx.module = m_Module;
        m_ErrorQueue.at(m_ParseStack.back()).emplace_back(code, ctx);
    }

    std::string_view internString(const std::string_view str) const {
        return m_StringPool.intern(str);
    }
};


struct Parser::NodeAttrHelper {
    /// Chief Node constructor
    NodeAttrHelper(Node* node, Parser& instance): node(node), instance(instance) {
        node->is_exported = instance.m_LastSymWasExported;
        node->location.from = instance.m_Stream.getStreamState();
        node->location.from.Pos -= instance.m_Stream.CurTok.value.size();

        node->location.source = instance.m_FileHandle;
        instance.m_RecursionDepth++;

        instance.stackSafeguard();

        // setup error-report buffering
        if (!instance.m_ErrorQueue.contains(node))
            instance.m_ErrorQueue.insert({node, {}});
        instance.m_ParseStack.emplace_back(node);

        if (node->isGlobal()) {
            const auto glob = dynamic_cast<GlobalNode*>(node);
            glob->is_extern = instance.m_LastSymIsExtern;
            glob->extern_attributes = instance.m_StringPool.intern(instance.m_ExternAttributes);

            begins_from = instance.m_Stream.getStreamState();
        }
    }


    /// Resets the states of the Parser
    ~NodeAttrHelper() {
        instance.m_RecursionDepth--;
        instance.m_LastSymIsExtern = false;
        instance.m_LastSymWasExported = false;
        instance.m_ExternAttributes.clear();

        if (node) {
            node->location.to = instance.m_Stream.getStreamState();
            node->location.to.Pos -= instance.m_Stream.CurTok.value.size();

            if (node->isGlobal()) {
                auto node_id = node->getIdentInfo();

                assert(node_id != nullptr);
                assert(begins_from.has_value());

                instance.m_GlobalOffsets.insert({node_id, {
                    begins_from.value(), instance.m_Stream.getStreamState()}});
            }
        }

        // flush all the errors
        for (auto& error : instance.m_ErrorQueue.at(node)) {
            auto& context = std::get<1>(error);
            if (!context.location.has_value()) {
                if (node) context.location = node->location;
            } instance.m_ErrorCallback(std::get<0>(error), context);
        } instance.m_ErrorQueue.at(node).clear();
        instance.m_ParseStack.pop_back();

        // else type->location.to = instance.m_Stream.getStreamState();
    }

private:
    Node*   node = nullptr;
    Type*   type = nullptr;

    Parser& instance;
    std::optional<StreamState> begins_from = std::nullopt;
};
