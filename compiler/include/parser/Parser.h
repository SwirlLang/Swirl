#pragma once
#include <utility>
#include <filesystem>

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

namespace sw {
    class StringPool;
    class FileSystem;
}

namespace sema {
    template <typename T> class SemaVisitor;
}


struct ParserContext {
    Module* module;
    ErrorCallback_t error_callback;
    ModuleManager&  module_manager;
    sw::StringPool& string_pool;
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

    std::string          m_ExternAttributes;
    Expression*          m_AttributeList = nullptr;
    // ---*--- ---*--- ---*---

    int                   m_RecursionDepth = 0;
    std::vector<Node*>    m_ParseStack;        // currently-being-parsed object pointer stays at the top

    sw::FileHandle*       m_FileHandle;

    // used for buffering error reports until the nodes/types have been completed
    std::unordered_map<Node*, std::vector<std::tuple<ErrCode, ErrorContext>>> m_ErrorQueue;

    sw::FileSystem& m_FileSystem;
    sw::StringPool& m_StringPool;

    struct Bracket_t { char val{}; StreamState location; };
    std::vector<Bracket_t> m_BracketTracker;

    // extern declaration buffer
    std::vector<Node*> m_ExternBlockBuffer;
    size_t             m_ExternBlockIdx = 0;

    struct NodeAttrHelper;

    friend class CompilerInst;
    friend class ModuleManager;
    friend class ExpressionParser;
    friend class LLVMBackend;

    template <typename T>
    friend class sema::SemaVisitor;

public:
    ModuleManager& ModuleMap;

    explicit Parser(const ParserContext& context);


    Node* dispatch();
    Node* parseExternBlock();

    Function*        parseFunction();
    WhileLoop*       parseWhile();
    Struct*          parseStruct();
    ImportNode*      parseImport();
    ReturnStatement* parseRet();
    Intrinsic*       parseIntrinsic();
    Protocol*        parseProtocol();
    Enum*            parseEnum();
    TypeAlias*       parseTypeAlias();
    ProtocolImpl*    parseProtocolImpl();

    ForLoop*   parseForLoop  (bool is_comptime = false);
    Condition* parseCondition(bool is_comptime = false);

    Var*        parseVar(bool is_comptime = false);
    Node*       parseCall(std::optional<Ident*> _ = std::nullopt);
    Parameter*  parseParam(bool&);

    template <typename Fn = std::identity>
    Scope* parseScope(const Fn& hook = std::identity{});

    std::span<Ident*>         parseProtocolList();
    std::span<GenericParam*>  parseGenericParamList();

    Token forwardStream(uint8_t n = 1);

    Ident*           parseIdent(bool type_context = false);
    Expression*      parseExpr();
    TypeWrapper*     parseType();
    GenericArgList   parseGenericArgList();

    void parse();
    void ignoreButExpect(const Token&);
    void ignoreButExpect(Token::TokenValue tok);

    void stackSafeguard() const;

    /// Returns the current token and reports an error if it doesn't match the given token id
    Token expect(Token::TokenValue tok);


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
        if (node->isGlobal()) {
            node->to<GlobalNode>()->is_exported = instance.m_LastSymWasExported;
        }

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
