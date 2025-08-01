#include <filesystem>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>

#include <utils/utils.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <lexer/Tokens.h>
#include <definitions.h>
#include <backend/LLVMBackend.h>
#include <managers/ErrorManager.h>
#include <symbols/SymbolManager.h>
#include <managers/ModuleManager.h>
#include <parser/SemanticAnalysis.h>


using SwNode = std::unique_ptr<Node>;


Parser::Parser(const std::filesystem::path& path, ErrorCallback_t error_callback, ModuleManager& mod_man)
    : m_Stream(m_SrcMan)
    , m_SrcMan(path)
    , m_ModuleMap(mod_man)
    , m_ErrorCallback(std::move(error_callback))
    , m_FilePath(path)
    , SymbolTable(m_SrcMan.getSourcePath(), m_ModuleMap)
{
    m_RelativeDir = m_FilePath.parent_path();
}


/// returns the current token and forwards the stream
Token Parser::forwardStream(const uint8_t n) {
    Token ret = m_Stream.CurTok;
    static const
    auto get_opposite_brack = [](const char ch) {
        switch (ch) {
            case '}':
                return '{';
            case ')':
                return '(';
            case ']':
                return '[';
            case '(':
                return ')';
            case '[':
                return ']';
            case '{':
                return '}';
            default:
                throw;
        }
    };

    for (uint8_t _ = 0; _ < n; _++) {
        if (m_Stream.CurTok.type == PUNC) {
            if (
               m_Stream.CurTok.value == "{" ||
               m_Stream.CurTok.value == "(" ||
               m_Stream.CurTok.value == "[")
                m_BracketTracker.emplace_back(m_Stream.CurTok.value[0], m_Stream.getStreamState());

            else if (
                m_Stream.CurTok.value == "}" ||
                m_Stream.CurTok.value == "]" ||
                m_Stream.CurTok.value == ")"
            ) {
                if (m_BracketTracker.empty()
                    || m_BracketTracker.back().val != get_opposite_brack(m_Stream.CurTok.value[0]))
                {
                    reportError(ErrCode::SYNTAX_ERROR, {
                        .msg = std::format(
                            "No '{}' is present to match '{}'.",
                            get_opposite_brack(m_Stream.CurTok.value[0]),
                            m_Stream.CurTok.value
                        )
                    });
                } else m_BracketTracker.pop_back();
            }
        }

        m_Stream.next();
    } return std::move(ret);
}


/// If `tok` is the current token in the stream, forward it, report a syntax error otherwise.
void Parser::ignoreButExpect(const Token& tok) {
    if (m_Stream.CurTok != tok) {
        reportError(ErrCode::SYNTAX_ERROR, {.msg = std::format("Expected '{}'.", tok.value)});
    } else forwardStream();
}



Type* Parser::parseType() {
    Type* base_type = nullptr;
    bool  is_mutable  = false;
    bool  is_reference = false;


    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "&") {
        forwardStream();
        is_reference = true;
    }

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "const") {
        is_mutable = false;
        forwardStream();
    }

    else if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "mut") {
        is_mutable = true;
        forwardStream();
    }

    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "[") {
        forwardStream();
        base_type = parseType();

        ignoreButExpect({OP, "|"});  // skip '|'

        if (m_Stream.CurTok.type != NUMBER || m_Stream.CurTok.meta != CT_INT) {
            reportError(ErrCode::NON_INT_ARRAY_SIZE);
            return nullptr;
        }

        base_type = SymbolTable.getArrayType(base_type, std::stoi(forwardStream().value));
        ignoreButExpect({PUNC, "]"});  // skip ']'

    } else if (m_Stream.CurTok.type == IDENT) {
        base_type = SymbolTable.lookupType(SymbolTable.getIDInfoFor(parseIdent()));
    }

    uint16_t ptr_level = 0;
    while (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "*") {
        ptr_level++;
        forwardStream();
    }

    if (ptr_level) {
        base_type = SymbolTable.getPointerType(base_type, ptr_level);
    }

    if (is_reference) {
        base_type = SymbolTable.getReferenceType(base_type, is_mutable);
    }

    return base_type;
}


SwNode Parser::dispatch() {
    while (!m_Stream.eof()) {
        // pattern matching in C++ when?
        switch (m_Stream.CurTok.type) {
            case KEYWORD:
                if (m_Stream.CurTok.value == "let" || m_Stream.CurTok.value == "var") {
                    auto ret = parseVar(false);
                    return std::move(ret);
                }

                if (m_Stream.CurTok.value == "extern") {
                    m_LastSymIsExtern = true;
                    forwardStream();

                    if (m_Stream.CurTok.type == STRING) {
                        m_ExternAttributes = std::move(m_Stream.CurTok.value);
                    } forwardStream();
                    continue;
                }

                if (m_Stream.CurTok.value == "export") {
                    m_LastSymWasExported = true;
                    forwardStream();
                    continue;
                }

                if (m_Stream.CurTok.value == "import") {
                    return parseImport();
                }

                if (m_Stream.CurTok.value == "fn") {
                    auto ret = parseFunction();
                    return std::move(ret);
                }

                if (m_Stream.CurTok.value == "if") {
                    auto ret = parseCondition();
                    return std::move(ret);
                }

                if (m_Stream.CurTok.value == "struct")
                    return parseStruct();

                if (m_Stream.CurTok.value == "while")
                    return parseWhile();

                if (m_Stream.CurTok.value == "volatile") {
                    forwardStream();
                    return parseVar(true);
                }

                if (m_Stream.CurTok.value == "return") {
                    return parseRet();
                }

            case NUMBER:
            case STRING:
            case IDENT:
            case OP:
                return std::make_unique<Expression>(parseExpr());
            case PUNC:
                if (m_Stream.CurTok.value == "[" || m_Stream.CurTok.value == "(")
                    return std::make_unique<Expression>(parseExpr());
                if (m_Stream.CurTok.value == ";") {
                    forwardStream();
                    continue;
                } if (m_Stream.CurTok.value == "}") {
                    return std::make_unique<Node>();
                }
            default:
                reportError(ErrCode::SYNTAX_ERROR);
                forwardStream();
        }
    }

    reportError(ErrCode::UNEXPECTED_EOF);
    return {};
}


std::unique_ptr<ImportNode> Parser::parseImport() {
    ImportNode ret;
    ret.location = m_Stream.getStreamState();
    ret.is_exported = m_LastSymWasExported;

    m_LastSymWasExported = false;

    forwardStream();  // skip 'import'

    ret.mod_path = m_RelativeDir;

    while (m_Stream.CurTok.type == IDENT) {
        ret.mod_path /= forwardStream().value;
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "::") {
            forwardStream();
        }
    }

    if (is_directory(ret.mod_path)) {
        reportError(ErrCode::NO_DIR_IMPORT);
        return {};
    }

    ret.mod_path += ".sw";
    if (!exists(ret.mod_path)) {
        reportError(ErrCode::MODULE_NOT_FOUND, {.path_1 = ret.mod_path});
        return {};
    }

    if (!m_ModuleMap.contains(ret.mod_path)) {
        m_ModuleMap.insert(ret.mod_path, m_ErrorCallback);
        m_ModuleMap.get(ret.mod_path).parse();
    }

    if (m_Dependencies.contains(&m_ModuleMap.get(ret.mod_path))) {
        reportError(ErrCode::DUPLICATE_IMPORT);
    }

    m_Dependencies.insert(&m_ModuleMap.get(ret.mod_path));
    m_ModuleMap.get(ret.mod_path).m_Dependents.insert(this);

    // specific-symbol import
    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "{") {
        forwardStream();  // skip '{'

        while (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != "}") {
            ret.imported_symbols.emplace_back(forwardStream().value);

            if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {
                forwardStream();  // skip "as"
                ret.imported_symbols.back().assigned_alias = forwardStream().value;
            }

            if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
                forwardStream();
            }
        } ignoreButExpect({PUNC, "}"});
    } else { // otherwise, if non-specific but aliased or wildcard
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {  // non-specific but aliased
            forwardStream();
            ret.alias = forwardStream().value;
        }
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "*") {  // wildcard
            forwardStream();
            ret.is_wildcard = true;
            m_ModuleMap.get(ret.mod_path).insertExportedSymbolsInto([&ret](std::string name) {
                ret.imported_symbols.push_back({.actual_name = std::move(name)});
            });
        }
        else forwardStream();
    }

    if (ret.imported_symbols.empty() && !ret.is_wildcard) {
        const auto name = ret.mod_path.filename().replace_extension().string();

        // register the module-prefix in the symbol manager
        SymbolTable.registerDecl(
            ret.alias.empty() ? name : ret.alias,
            {
                .is_exported = ret.is_exported,
                .is_mod_namespace = true,
                .scope = SymbolTable.getGlobalScopeFromModule(ret.mod_path)
            }) ? void() : reportError(ErrCode::SYMBOL_ALREADY_EXISTS, {
                .str_1 = ret.alias.empty() ? name : ret.alias
            });
    }

    return std::make_unique<ImportNode>(std::move(ret));
}

/// begin parsing ///
void Parser::parse() {
    forwardStream();
    while (!m_Stream.eof()) {
        AST.emplace_back(dispatch());
    }

    m_UnresolvedDeps = m_Dependencies.size();
    if (m_Dependencies.empty())
        m_ModuleMap.m_ZeroDepVec.push_back(this);
}

/// begin semantic analysis  ///
void Parser::performSema() {
    AnalysisContext analysis_ctx{*this};
    analysis_ctx.startAnalysis();
}


void Parser::decrementUnresolvedDeps() {
    m_UnresolvedDeps--;
    if (m_UnresolvedDeps == 0) {
        m_ModuleMap.m_BackBuffer.push_back(this);
    }
}


std::unique_ptr<Function> Parser::parseFunction() {
    Function func_nd;
    func_nd.location = m_Stream.getStreamState();
    func_nd.is_exported = m_LastSymWasExported;
    func_nd.is_extern = m_LastSymIsExtern;
    func_nd.extern_attributes = m_ExternAttributes;

    m_LastSymWasExported = false;
    m_LastSymIsExtern = false;
    m_ExternAttributes.clear();

    // handle the special case of `main`
    const std::string func_ident = m_Stream.next().value;
    if (func_ident == "main" && !m_IsMainModule) {
        reportError(ErrCode::MAIN_REDEFINED);
    }

    m_Stream.expectTokens({Token{PUNC, "("}});
    forwardStream(2);
    auto function_t = std::make_unique<FunctionType>();

    auto parse_params = [function_t = function_t.get(), this] {
        Var param;
        const std::string var_name = m_Stream.CurTok.value;
        param.location = m_Stream.getStreamState();

        forwardStream(2);
        param.var_type = parseType();

        param.initialized = m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "=";
        if (param.initialized)
            param.value = parseExpr();

        assert(function_t != nullptr);
        function_t->param_types.push_back(param.var_type);

        TableEntry param_entry;
        param_entry.swirl_type = param.var_type;
        param_entry.is_param = true;

        param.var_ident = SymbolTable.registerDecl(var_name, param_entry);
        if (!param.var_ident) {
            reportError(ErrCode::SYMBOL_ALREADY_EXISTS, {.str_1 = var_name});
        }

        return std::move(param);
    };

    // parsing the parameters...
    SymbolTable.newScope();  // emplace the function body scope
    if (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != ")") {
        while (m_Stream.CurTok.value != ")" && m_Stream.CurTok.type != PUNC) {
            func_nd.params.emplace_back(parse_params());
            if (m_Stream.CurTok.value == ",")
                forwardStream();
        }
    }

    // current token == ')'
    forwardStream();  // skip ')'

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();
        // func_nd.ret_type = parseType();
        function_t->ret_type = parseType();
    }

    TableEntry entry;
    entry.swirl_type = function_t.get();
    entry.is_exported = func_nd.is_exported;
    func_nd.ident = SymbolTable.registerDecl(func_ident, entry, 0);
    function_t->ident = func_nd.ident;

    if (!func_nd.ident)
        reportError(ErrCode::SYMBOL_ALREADY_EXISTS, {.str_1 = func_ident});

    // register the function's signature as a type in the symbol manager
    SymbolTable.registerType(func_nd.ident, function_t.release());

    // WARNING: func_nd has been MOVED here!!
    auto ret = std::make_unique<Function>(std::move(func_nd));
    // ReSharper disable once CppDFALocalValueEscapesFunction
    m_LatestFuncNode = ret.get();
    NodeJmpTable[ret->ident] = ret.get();

    if (ret->is_extern) {
        // TODO: report an error if a body is provided
        return std::move(ret);
    }

    // parse the children
    forwardStream();  // skip '{'
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        ret->children.push_back(dispatch());
    } forwardStream();
    SymbolTable.moveToPreviousScope();  // decrement the scope index, back to the global scope!

    return std::move(ret);
}

std::unique_ptr<Var> Parser::parseVar(const bool is_volatile) {
    Var var_node;

    var_node.location = m_Stream.getStreamState();
    var_node.is_const = m_Stream.CurTok.value[0] == 'l';
    var_node.is_volatile = is_volatile;
    var_node.is_exported = m_LastSymWasExported;
    var_node.is_extern = m_LastSymIsExtern;
    var_node.extern_attributes = m_ExternAttributes;

    m_LastSymWasExported = false;
    m_LastSymIsExtern = false;
    m_ExternAttributes.clear();

    const std::string var_ident = m_Stream.next().value;
    forwardStream();  // [:, =]


    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();
        var_node.var_type = parseType();
    }

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
        var_node.initialized = true;
        forwardStream();
        var_node.value = parseExpr();
    }

    TableEntry entry;
    entry.is_const = var_node.is_const;
    entry.is_volatile = var_node.is_volatile;
    entry.is_exported = var_node.is_exported;
    entry.swirl_type = var_node.var_type;

    var_node.var_ident = SymbolTable.registerDecl(var_ident, entry);
    if (!var_node.var_ident) {
        reportError(ErrCode::SYMBOL_ALREADY_EXISTS, {.str_1 = var_ident});
    }

    return std::make_unique<Var>(std::move(var_node));
}

std::unique_ptr<FuncCall> Parser::parseCall(std::optional<Ident> ident) {
    auto call_node = std::make_unique<FuncCall>();
    call_node->location = m_Stream.getStreamState();
    call_node->ident = std::move(ident.value());

    forwardStream();  // skip '('

    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ")") {
        forwardStream();
    }
    else {
        while (true) {
            if (m_Stream.CurTok.type == PUNC) {
                if (m_Stream.CurTok.value == ",")
                    forwardStream();
                if (m_Stream.CurTok.value == ")")
                    break;
            }

            call_node->args.emplace_back(parseExpr());
        }
    }

    if (m_Stream.CurTok.value == ")") {
        forwardStream();
    }
    return call_node;
}

std::unique_ptr<ReturnStatement> Parser::parseRet() {
    ReturnStatement ret;
    ret.location = m_Stream.getStreamState();

    forwardStream();
    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ";")
        return std::make_unique<ReturnStatement>(std::move(ret));

    ret.value = parseExpr();
    return std::make_unique<ReturnStatement>(std::move(ret));
}


std::unique_ptr<Condition> Parser::parseCondition() {
    Condition cnd;
    forwardStream();  // skip "if"
    cnd.bool_expr = parseExpr();
    cnd.bool_expr.location = m_Stream.getStreamState();

    forwardStream();  // skip the opening brace

    SymbolTable.newScope();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}"))
        cnd.if_children.push_back(std::move(dispatch()));
    forwardStream();
    SymbolTable.moveToPreviousScope();

    // handle `else(s)`
    if (!(m_Stream.CurTok.type == KEYWORD && (m_Stream.CurTok.value == "else" || m_Stream.CurTok.value == "elif")))
        return std::make_unique<Condition>(std::move(cnd));

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
        while (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
            SymbolTable.newScope();
            forwardStream();

            std::tuple<Expression, std::vector<SwNode>> children;
            std::get<0>(children) = parseExpr();

            forwardStream();
            while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
                std::get<1>(children).push_back(dispatch());
            } forwardStream();

            cnd.elif_children.emplace_back(std::move(children));
            SymbolTable.moveToPreviousScope();
        }
    }

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "else") {
        SymbolTable.newScope();
        forwardStream(2);
        while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
            cnd.else_children.push_back(dispatch());
        } forwardStream();
        SymbolTable.moveToPreviousScope();
    }

    return std::make_unique<Condition>(std::move(cnd));
}

Ident Parser::parseIdent() {
    Ident ret;
    ret.location = m_Stream.getStreamState();

    ret.full_qualification.emplace_back(forwardStream().value);
    while (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "::") {
        forwardStream();
        ret.full_qualification.emplace_back(forwardStream().value);
    }

    if (ret.full_qualification.size() == 1) {
        ret.value = SymbolTable.getIDInfoFor(ret.full_qualification.front());
    }

    return std::move(ret);
}


std::unique_ptr<Struct> Parser::parseStruct() {
    forwardStream();  // skip 'struct'
    Struct ret;
    ret.is_exported = m_LastSymWasExported;
    ret.location = m_Stream.getStreamState();
    ret.is_externed = m_LastSymIsExtern;
    ret.extern_attributes = m_ExternAttributes;

    m_LastSymWasExported = false;
    m_LastSymIsExtern = false;

    const auto struct_ty = new StructType{};
    auto struct_name = forwardStream().value;

    // ask for a decl registry to the symbol manager
    ret.ident = SymbolTable.registerDecl(struct_name, {
        .is_exported = ret.is_exported,
    });

    if (!ret.ident) {
        reportError(ErrCode::SYMBOL_ALREADY_EXISTS, {.str_1 = struct_name});
    }

    // create the struct's scope
    ignoreButExpect({PUNC, "{"});  // skip '{'
    const auto scope_pointer = SymbolTable.newScope();
    SymbolTable.lookupDecl(ret.ident).scope = scope_pointer;

    // handle the children
    std::size_t i = 0;
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        m_LastSymWasExported = true;
        auto member = dispatch();
        if (member->getNodeType() == ND_VAR) {
            struct_ty->field_offsets.insert({member->getIdentInfo()->toString(), i++});
        } ret.members.emplace_back(std::move(member));
    } ignoreButExpect({PUNC, "}"});  // skip '}'

    // exit the struct's scope
    SymbolTable.moveToPreviousScope();

    struct_ty->scope = scope_pointer;
    struct_ty->ident = ret.ident;

    // register the type and the scope of the struct as a qualifier
    SymbolTable.registerType(ret.ident, struct_ty);
    SymbolTable.lookupDecl(ret.ident).scope = scope_pointer;

    return std::make_unique<Struct>(std::move(ret));
}


std::unique_ptr<WhileLoop> Parser::parseWhile() {
    WhileLoop loop;
    forwardStream();

    loop.condition = parseExpr();

    SymbolTable.newScope();
    forwardStream();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        loop.children.push_back(dispatch());
    } forwardStream();
    SymbolTable.moveToPreviousScope();

    return std::make_unique<WhileLoop>(std::move(loop));
}

Expression Parser::parseExpr(const int min_bp) {
    auto ret = m_ExpressionParser.parseExpr(min_bp);
    return std::move(ret);
}