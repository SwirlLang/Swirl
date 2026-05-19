#include <filesystem>
#include <memory>
#include <fstream>
#include <utility>
#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>
#include <expected>

#include "utils/utils.h"
#include "ast/Nodes.h"
#include "parser/Parser.h"

#include "lexer/Tokens.h"
#include "errors/ErrorManager.h"
#include "symbols/SymbolManager.h"
#include "modules/ModuleManager.h"

#include "CompilerInst.h"


/// Automatically sets certain node attributes
#define SET_NODE_ATTRS(x) NodeAttrHelper GET_UNIQUE_NAME(attr_setter_){x, *this}


Parser::Parser(const ParserContext& context)
    : m_Stream(m_SrcMan)
    , m_SrcMan(context.module)
    , m_Module(context.module)
    , m_ErrorCallback(context.error_callback)
    , m_FileHandle(context.module->file_handle)
    , m_FileSystem(*context.module->file_handle->getFileSystemHandle())
    , m_StringPool(context.string_pool)
    , ModuleMap(context.module_manager)
    {
      // set the error callback of the symbol manager to the Parser's error reporter
      m_Module->symbol_table.setErrorCallback(
          [this](auto code, const auto& ctx) {
              reportError(code, ctx);
          });
    }


void Parser::stackSafeguard() const {
    if (m_RecursionDepth > CompilerInst::RecursionDepth) {
        std::println(stderr,
            "Max recursion depth ({}) exceeded! "
            "Use the `-depth <i>` flag to increase it.",
            CompilerInst::RecursionDepth
            );
        std::exit(1);
    }
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
    } return ret;
}


/// If `tok` is the current token in the stream, forward it, report a syntax error otherwise.
void Parser::ignoreButExpect(const Token& tok) {
    if (m_Stream.eof()) {
        reportError(ErrCode::UNEXPECTED_EOF);
    }

    if (m_Stream.CurTok != tok) {
        reportError(ErrCode::SYNTAX_ERROR, {.msg = std::format("Expected '{}'.", tok.value)});
    } else forwardStream();
}


TypeWrapper* Parser::parseType() {
    auto wrapper = m_Module->makeNode<TypeWrapper>();
    SET_NODE_ATTRS(wrapper);

    // handle references and pointers (*&T...)
    bool is_reference_present = false;
    std::vector<TypeWrapper::Modifiers> modifiers;

    while (m_Stream.CurTok.type == OP && (m_Stream.CurTok.value == "&" || m_Stream.CurTok.value == "*")) {
        if (m_Stream.CurTok.value == "&") {
            modifiers.push_back(TypeWrapper::Reference);  // do not push the slice-associated `&`
            is_reference_present = true;
            forwardStream();
        } else if (m_Stream.CurTok.value == "*") {
            modifiers.push_back(TypeWrapper::Pointer);
            forwardStream();
        }
    }

    if (m_Stream.CurTok.tokenid == Token::PUNC_LBRACKET) {
        forwardStream();
        wrapper->of_type = parseType();

        // array declaration
        if (m_Stream.CurTok.tokenid == Token::OP_BITWISE_OR) {
            forwardStream();

            if (m_Stream.CurTok.type == NUMBER && m_Stream.CurTok.tokenid == Token::NUM_INT) {
                wrapper->array_size = toInteger(forwardStream().value);
            } else reportError(ErrCode::NON_INT_ARRAY_SIZE);
            ignoreButExpect({PUNC, "]"});
        } else {
            if (wrapper->modifiers.back() == TypeWrapper::Reference)
                modifiers.pop_back();
            wrapper->is_slice = true;  // only slices are allowed to not have a size
            ignoreButExpect({PUNC, "]"});
        }

    } else if (m_Stream.CurTok.type == IDENT) {
        wrapper->type_id = parseIdent();
        assert(wrapper->type_id != nullptr);
    } else if (m_Stream.CurTok.tokenid == Token::KW_MUT) {
        wrapper->is_mutable = true;
        forwardStream();
    } else {
        reportError(ErrCode::SYNTAX_ERROR, {.msg = "Expected a type"});
        forwardStream();
    }

    if (!is_reference_present && wrapper->is_slice) {
        reportError(ErrCode::SYNTAX_ERROR, {
            .msg = "Only slices are allowed to not have an explicit size. "
            "Did you forgot to place an `&`?"
        });
    }

    wrapper->modifiers = m_Module->internArray<TypeWrapper::Modifiers>(modifiers);
    return wrapper;
}


Node* Parser::dispatch() {
    while (!m_Stream.eof()) {
        switch (m_Stream.CurTok.tokenid) {
            case Token::KW_LET:
            case Token::KW_VAR:
            case Token::KW_COMPTIME:
                return parseVar(false);
            case Token::KW_IMPORT:
                return parseImport();
            case Token::KW_FN:
                return parseFunction();
            case Token::KW_IF:
                return parseCondition();
            case Token::KW_STRUCT:
                return parseStruct();
            case Token::KW_WHILE:
                return parseWhile();
            case Token::KW_RETURN:
                return parseRet();
            case Token::KW_PROTOCOL:
                return parseProtocol();
            case Token::KW_ENUM:
                return parseEnum();
            case Token::KW_TRUE:
            case Token::KW_FALSE:
                return parseExpr();
            case Token::KW_VOLATILE:
                forwardStream();
                return parseVar(true);
            case Token::KW_BREAK:
                forwardStream();
                return m_Module->makeNode<BreakStmt>();
            case Token::KW_CONTINUE:
                forwardStream();
                return m_Module->makeNode<ContinueStmt>();
            case Token::KW_EXPORT:
                m_LastSymWasExported = true;
                forwardStream();
                continue;
            case Token::KW_EXTERN:
                m_LastSymIsExtern = true;
                forwardStream();

                if (m_Stream.CurTok.type == STRING) {
                    m_ExternAttributes = std::move(m_Stream.CurTok.value);
                    forwardStream();
                }
                continue;

            case Token::PUNC_LBRACKET:
            case Token::PUNC_LPAREN:
                return parseExpr();

            case Token::PUNC_LBRACE:
                return parseScope();
            case Token::PUNC_HASH: {
                forwardStream();
                m_AttributeList = parseExpr();
                continue;
            }

            // ignore semicolons
            case Token::PUNC_SEMI:
                forwardStream();
                return m_Module->makeNode<Node>();
            case Token::PUNC_RBRACE:
                if (m_BracketTracker.empty() || m_BracketTracker.back().val != '{') {
                    forwardStream();
                    continue;
                }
                return m_Module->makeNode<Node>();
            default:
                switch (m_Stream.CurTok.type) {
                    case KEYWORD:
                        reportError(ErrCode::UNEXPECTED_KEYWORD, {.str_1 = m_Stream.CurTok.value});
                        forwardStream();
                        continue;
                    case NUMBER:
                    case STRING:
                    case IDENT:
                    case OP:
                        return parseExpr();
                    default:
                        reportError(ErrCode::SYNTAX_ERROR);
                        forwardStream();
                        break;
                }
        }
    }
    reportError(ErrCode::UNEXPECTED_EOF);
    return {};
}


Node* Parser::parseImport() {
    ImportNode ret;
    SET_NODE_ATTRS(&ret);

    forwardStream();  // skip 'import'

    fs::path mod_path;
    std::vector<ImportNode::ImportedSymbol_t> imported_symbols;

    if (CompilerInst::PackageTable.contains(m_Stream.CurTok.value)) {
        mod_path = CompilerInst::PackageTable[m_Stream.CurTok.value].package_root;
    } else reportError(ErrCode::PACKAGE_NOT_FOUND,
        {.str_1 = m_StringPool.intern(m_Stream.CurTok.value)});

    forwardStream();  // skip the current IDENT
    ignoreButExpect({OP, "::"});

    while (m_Stream.CurTok.type == IDENT) {
        mod_path /= forwardStream().value;
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "::") {
            forwardStream();
        }
    }

    if (is_directory(mod_path)) {
        reportError(ErrCode::NO_DIR_IMPORT);
        return {};
    }

    mod_path += ".sw";
    if (!exists(mod_path)) {
        reportError(ErrCode::MODULE_NOT_FOUND, {.path_1 = mod_path});
        return {};
    }

    const auto handle = m_FileSystem.open(mod_path);
    ret.mod_handle = handle;

    if (!ModuleMap.contains(handle)) {
        ModuleMap.insert(handle, m_Module->getModuleContext());
        ModuleMap.get(handle).parse(m_ErrorCallback);
    }

    if (m_Module->dependencies.contains(&ModuleMap.get(handle))) {
        reportError(ErrCode::DUPLICATE_IMPORT);
    }

    m_Module->dependencies.insert(&ModuleMap.get(handle));
    ModuleMap.get(handle).dependents.insert(m_Module);

    // specific-symbol import
    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "{") {
        forwardStream();  // skip '{'

        while (m_Stream.CurTok.type != PUNC || m_Stream.CurTok.value != "}") {
            imported_symbols.emplace_back(m_StringPool.intern(forwardStream().value));

            if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {
                forwardStream();  // skip "as"
                imported_symbols.back().assigned_alias = m_StringPool.intern(forwardStream().value);
            }

            if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
                forwardStream();
            }
        } ignoreButExpect({PUNC, "}"});
    }

    // otherwise, if non-specific but aliased or wildcard
    else {
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {  // non-specific but aliased
            forwardStream();
            ret.alias = m_StringPool.intern(forwardStream().value);
        }
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "*") {  // wildcard
            forwardStream();
            ret.is_wildcard = true;
            ModuleMap.get(handle).insertExportedSymbolsInto([&imported_symbols](const std::string_view name) {
                imported_symbols.push_back({.actual_name = name});
            });
        }
        else forwardStream();
    }

    if (imported_symbols.empty() && !ret.is_wildcard) {
        const auto name = ret.mod_handle->getPath().filename().replace_extension().string();

        // register the module-prefix in the symbol manager
        m_Module->symbol_table.registerDecl(
            (ret.alias.empty()) ? std::string(name) : std::string(ret.alias),
            {
                .is_exported = ret.is_exported,
                .is_mod_namespace = true,
                .scope = m_Module->symbol_table.getGlobalScopeFromModule(ret.mod_handle)
            }) ? void() : reportError(ErrCode::SYMBOL_ALREADY_EXISTS, {
                .str_1 = ret.alias.empty() ? std::string(name) : std::string(ret.alias)
            });
    }

    ret.imported_symbols = m_Module->internArray<ImportNode::ImportedSymbol_t>(imported_symbols);
    return m_Module->makeNode<ImportNode>(std::move(ret));
}


/// begin parsing ///
void Parser::parse() {
    forwardStream();

    const auto builtin_handle = m_FileSystem.fetchHandleFor(SW_BUILTIN_FILE_PATH);

    if (m_FileHandle != builtin_handle) {
        auto builtin_import = m_Module->makeNode<ImportNode>();
        builtin_import->is_wildcard = true;
        builtin_import->is_exported = false;
        builtin_import->mod_handle  = builtin_handle;
        std::vector<ImportNode::ImportedSymbol_t> imported_symbols;

        // TODO: remove duplicate import-registration logic
        if (!ModuleMap.contains(builtin_handle)) {
            ModuleMap.insert(builtin_handle, m_Module->getModuleContext());

            ModuleMap.get(builtin_handle).parse(m_ErrorCallback);
            ModuleMap.get(builtin_handle).performSema(m_ErrorCallback);
        }

        ModuleMap.get(builtin_handle).insertExportedSymbolsInto([&imported_symbols](const std::string_view name) {
            imported_symbols.push_back({.actual_name = name});
        });

        builtin_import->imported_symbols = m_Module->internArray<ImportNode::ImportedSymbol_t>(imported_symbols);
        m_Module->dependencies.insert(&ModuleMap.get(builtin_handle));
        ModuleMap.get(builtin_handle).dependents.insert(m_Module);

        m_Module->ast.emplace_back(builtin_import);
    }

    while (!m_Stream.eof()) {
        m_Module->ast.emplace_back(dispatch());
    }

    m_Module->unresolved_deps = m_Module->dependencies.size();
    if (m_Module->dependencies.empty())
        ModuleMap.m_ZeroDepVec.push_back(m_Module);
}


Node* Parser::parseFunction() {
    const auto func_nd = m_Module->makeNode<Function>();
    SET_NODE_ATTRS(func_nd);

    const std::string func_ident = m_Stream.next().value;
    func_nd->name = m_StringPool.intern(func_ident);

    // handle the special case of `main`
    if (func_ident == "main" && !m_Module->isMainModule()) {
        reportError(ErrCode::MAIN_REDEFINED);
    }

    forwardStream(); // skip the ID

    // check for generics
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        func_nd->generic_params = parseGenericParamList();
    }

    ignoreButExpect({PUNC, "("});

    // parse the parameters...
    std::vector<Var*> params;

    if (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != ")") {
        while (m_Stream.CurTok.value != ")" && m_Stream.CurTok.type != PUNC) {
            params.emplace_back(parseParam(func_nd->is_static_method));
            if (m_Stream.CurTok.value == ",")
                forwardStream();
        }
    }  func_nd->params = m_Module->internArray<Var*>(params);

    forwardStream();  // skip ')'

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();
        func_nd->return_type = parseType();
    }

    m_LatestFuncNode = func_nd;

    if (func_nd->is_extern) {
        if (m_Stream.CurTok.tokenid == Token::PUNC_LBRACE) {
            reportError(ErrCode::EXTERN_CANNOT_HAVE_BODY);
            parseScope();
        }

        return func_nd;
    }

    // parse the children
    func_nd->children = parseScope();

    m_LatestFuncNode = nullptr;
    return func_nd;
}


Var* Parser::parseParam(bool& method_is_static) {
    Var* param = m_Module->makeNode<Var>();
    param->is_param = true;

    SET_NODE_ATTRS(param);

    // special case of `&self`
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "&") {
        forwardStream();
        bool is_mutable = false;

        if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "mut") {
            is_mutable = true;
            forwardStream();
        }

        param->is_instance_param = true;
        param->is_const = !is_mutable;

        method_is_static = false;
        ignoreButExpect({IDENT, "self"});
        param->name = internString("self");
        return param;
    }

    param->name = internString(m_Stream.CurTok.value);

    forwardStream(2);
    param->var_type = parseType();

    param->initialized = m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "=";
    if (param->initialized)
        param->value = parseExpr();

    return param;
}


std::span<GenericParam*> Parser::parseGenericParamList() {
    std::vector<GenericParam*> params;

    forwardStream();  // skip '<'
    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ">") {
            forwardStream();
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
            forwardStream();
            continue;
        }

        auto* param = m_Module->makeNode<GenericParam>();
        param->name = m_StringPool.intern(forwardStream().value);
        params.push_back(param);
    }

    return m_Module->internArray<GenericParam*>(params);
}


GenericArgList Parser::parseGenericArgList() {
    GenericArgList ret;
    std::vector<GenericArg*> args;

    forwardStream();  // skip '<'

    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ">") {
            forwardStream();
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
            forwardStream();
            continue;
        }

        // parsing logic: if comptime - parseExpr, otherwise parseType
        if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "comptime") {
            forwardStream();
            args.emplace_back(m_Module->makeNode<GenericArg>(parseExpr()));
        } else args.emplace_back(m_Module->makeNode<GenericArg>(parseType()));
    }

    ret.generic_args = m_Module->internArray<GenericArg*>(args);
    return ret;
}


Node* Parser::parseVar(const bool is_volatile) {
    auto ret = m_Module->makeNode<Var>();
    SET_NODE_ATTRS(ret);

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "comptime") {
        ret->is_comptime = true;
    }

    ret->is_const = m_Stream.CurTok.value[0] == 'l';
    ret->is_volatile = is_volatile;

    const std::string var_ident = m_Stream.next().value;
    forwardStream();  // [:, =]


    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();
        ret->var_type = parseType();
        ret->var_type->is_mutable = !ret->is_const;
    }


    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
        forwardStream();
        if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "undefined") {
               ret->initialized = false;
        } else ret->initialized = true;
        ret->value = parseExpr();
    }

    if (ret->is_comptime && !ret->initialized) {
        reportError(ErrCode::INITIALIZER_REQUIRED);
    } else if (ret->is_comptime && ret->initialized) {
        ret->value = m_Module->makeNode<Expression>(Expression::makeExpression(ret->value->evaluate(*this)));
    }

    ret->name = m_StringPool.intern(var_ident);

    return ret;
}


Node* Parser::parseCall(std::optional<Ident*> ident) {
    auto call_node = m_Module->makeNode<FuncCall>();
    SET_NODE_ATTRS(call_node);
    call_node->ident = ident.value();

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        call_node->generic_args = parseGenericArgList();
    }

    // parse arguments
    forwardStream();  // skip '('

    std::vector<Expression*> args;
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ")")) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
            forwardStream();
            continue;
        }

        args.emplace_back(parseExpr());
    } forwardStream();

    call_node->args = m_Module->internArray<Expression*>(args);
    return call_node;
}


Node* Parser::parseRet() {
    auto ret = m_Module->makeNode<ReturnStatement>();
    SET_NODE_ATTRS(ret);

    if (m_LatestFuncNode == nullptr) {
        reportError(ErrCode::SYNTAX_ERROR, {
            .msg = "`return` cannot appear outside functions."});
    }

    forwardStream();
    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ";")
        return ret;

    ret->value = parseExpr();
    return ret;
}


Node* Parser::parseIntrinsic() {
    auto call_node = m_Module->makeNode<Intrinsic>();
    SET_NODE_ATTRS(call_node);

    forwardStream();  // skip the `@`

    if (m_Stream.CurTok.value == "sizeof") {
        forwardStream();
        ignoreButExpect({PUNC, "("});

        std::array<Expression*, 1> arg = {nullptr};
        std::array qualifier = {Ident::Qualifier{.name = m_StringPool.intern("sizeof")}};

        call_node->ident->full_qualification = m_Module->internArray<Ident::Qualifier>(qualifier);
        call_node->intrinsic_type = Intrinsic::SIZEOF;

        if (m_Stream.CurTok.value == "@" && m_Stream.CurTok.type == PUNC)
             arg[0] = m_Module->makeNode<Expression>(Expression::makeExpression(parseIntrinsic()));
        else arg[0] = m_Module->makeNode<Expression>(Expression::makeExpression(parseType()));

        call_node->args = m_Module->internArray<Expression*>(arg);
        ignoreButExpect({PUNC, ")"});

    } else *call_node = dynamic_cast<FuncCall*>(parseCall(parseIdent()));

    return call_node;
}


Scope* Parser::parseScope() {
    auto scope = m_Module->makeNode<Scope>();
    SET_NODE_ATTRS(scope);

    std::vector<Node*> children;

    forwardStream(); // skip '{'
    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.tokenid == Token::PUNC_RBRACE) {
            forwardStream();
            break;
        }

        children.emplace_back(dispatch());
    }

    scope->children = m_Module->internArray<Node*>(children);
    return scope;
}


Node* Parser::parseCondition() {
    auto cnd = m_Module->makeNode<Condition>();
    SET_NODE_ATTRS(cnd);

    forwardStream();  // skip "if"
    if (m_Stream.CurTok == Token{KEYWORD, "comptime"}) {
        cnd->is_comptime = true;
        forwardStream();  // skip "comptime"
    }

    cnd->bool_expr = parseExpr();
    if (cnd->is_comptime) {
        cnd->bool_expr =
            m_Module->makeNode<Expression>(Expression::makeExpression(cnd->bool_expr->evaluate(*this)));
    }


    std::vector<Node*> if_children;

    cnd->if_children = parseScope();

    // handle `else(s)`
    if (!(m_Stream.CurTok.type == KEYWORD && (m_Stream.CurTok.value == "else" || m_Stream.CurTok.value == "elif")))
        return cnd;

    std::vector<Condition::elif_t> elif_children;

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
        while (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
            forwardStream();

            Condition::elif_t children;
            std::get<0>(children) = parseExpr();

            if (cnd->is_comptime) {
                std::get<0>(children) = m_Module->makeNode<Expression>(Expression::makeExpression(
                    std::get<0>(children)->evaluate(*this)));
            }

            std::get<1>(children) = parseScope();

            elif_children.emplace_back(
                std::get<0>(children),
                std::get<1>(children));
        }
    } cnd->elif_children = m_Module->internArray<Condition::elif_t>(elif_children);


    std::vector<Node*> else_children;
    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "else") {
        forwardStream();  // skip 'else'

        cnd->else_children = parseScope();
    }

    return cnd;
}


Ident* Parser::parseIdent() {
    auto* ret = m_Module->makeNode<Ident>();
    SET_NODE_ATTRS(ret);

    std::vector<Ident::Qualifier> full_qualification;

    full_qualification.emplace_back(m_StringPool.intern(forwardStream().value));
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "!") {
        forwardStream();
        ret->has_generic_args = true;
        full_qualification.back().generic_args = parseGenericArgList();
    }

    while (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "::") {
        forwardStream();
        full_qualification.emplace_back(m_StringPool.intern(forwardStream().value));

        // check for generics
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "!") {
            forwardStream();
            ret->has_generic_args = true;
            full_qualification.back().generic_args = parseGenericArgList();
        }
    }

    ret->full_qualification = m_Module->internArray<Ident::Qualifier>(full_qualification);
    assert(!ret->full_qualification.empty());
    return ret;
}


Node* Parser::parseEnum() {
    auto ret = m_Module->makeNode<Enum>();
    SET_NODE_ATTRS(ret);

    forwardStream();  // skip 'enum'

    const std::string enum_name = forwardStream().value;
    ret->name = m_StringPool.intern(enum_name);

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();  // skip ':'
        ret->enum_type = parseType();
    }

    forwardStream(); // skip "{"
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
            forwardStream();
            continue;
        }

        if (m_Stream.CurTok.type == IDENT) {
            const auto name = m_StringPool.intern(forwardStream().value);
            ret->addEntry(name);
            continue;
        }

        reportError(ErrCode::SYNTAX_ERROR, {.msg = "Expected identifier."});
        forwardStream();
    } forwardStream();  // skip '}'

    return ret;
}


Node* Parser::parseProtocol() {
    auto ret = m_Module->makeNode<Protocol>();
    SET_NODE_ATTRS(ret);

    std::vector<Protocol::MemberSignature> members;
    std::vector<Protocol::MethodSignature> methods;

    forwardStream(); // skip 'protocol'
    ret->name = m_StringPool.intern(forwardStream().value);

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        ret->generic_params = parseGenericParamList();
    }

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        ret->depended_protocols = parseProtocolList();
    }

    ignoreButExpect({PUNC, "{"});

    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}") {
            forwardStream();
            break;
        }

        const auto child = dispatch();
        switch (child->getNodeType()) {
            case ND_VAR: {
                const auto var = child->to<Var>();
                members.push_back(Protocol::MemberSignature{
                    .name = m_StringPool.intern(var->var_ident->toString()),
                    .type = var->var_type,
                });
                break;
            }

            case ND_FUNC: {
                const auto func  = child->to<Function>();
                std::vector<TypeWrapper*> func_params;

                // TODO: allow protocols to be used as "types" in the methods' params within the protocol

                func_params.reserve(func->params.size());
                for (auto& var : func->params) {
                    func_params.push_back(var->var_type);
                }

                methods.push_back(Protocol::MethodSignature{
                    .name = m_StringPool.intern(func->ident->toString()),
                    .return_type = func->return_type,
                    .params = func_params,
                });
                break;
            }

            case ND_INVALID:
                continue;

            default:
                reportError(ErrCode::SYNTAX_ERROR, {
                    .msg = "unexpected statement inside a protocol",
                    .location = child->location
                }); break;
        }
    }

    ret->members = m_Module->internArray<Protocol::MemberSignature>(members);
    ret->methods = m_Module->internArray<Protocol::MethodSignature>(methods);

    return ret;
}


std::span<Ident*> Parser::parseProtocolList() {
    std::vector<Ident*> ret;
    forwardStream();  // skip ':'

    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            return {};
        }

        if (m_Stream.CurTok.type == PUNC) {
            if (m_Stream.CurTok.value == ",") {
                forwardStream();
                continue;
            }

            if (m_Stream.CurTok.value == "{") {
                break;
            }
        }

        if (m_Stream.CurTok.type == IDENT) {
            ret.push_back(parseIdent());
        }
    }

    return m_Module->internArray<Ident*>(ret);
}


Node* Parser::parseStruct() {
    forwardStream();  // skip 'struct'
    auto ret = m_Module->makeNode<Struct>();
    SET_NODE_ATTRS(ret);

    ret->name = m_StringPool.intern(forwardStream().value);

    std::vector<Node*> members;

    // parse generic parameters
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        ret->generic_params = parseGenericParamList();
    }

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        ret->protocols = parseProtocolList();
    }

    m_LastSymWasExported = true;
    ret->members = parseScope();

    return ret;
}


Node* Parser::parseWhile() {
    auto ret = m_Module->makeNode<WhileLoop>();
    SET_NODE_ATTRS(ret);

    forwardStream();
    ret->condition = parseExpr();
    std::vector<Node*> children;

    ret->children = parseScope();

    return ret;
}


Expression* Parser::parseExpr() {
    const auto ret = m_Module->makeNode<Expression>(m_ExpressionParser.parseExpr());
    SET_NODE_ATTRS(ret);
    return ret;
}