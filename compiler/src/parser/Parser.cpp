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
    , NodeJmpTable(context.module->node_jmp_table) {
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

    while (m_Stream.CurTok.type == OP && (m_Stream.CurTok.value == "&" || m_Stream.CurTok.value == "*")) {
        if (m_Stream.CurTok.value == "&") {
            wrapper->modifiers.push_back(TypeWrapper::Reference);  // do not push the slice-associated `&`
            is_reference_present = true;
            forwardStream();
        } else if (m_Stream.CurTok.value == "*") {
            wrapper->modifiers.push_back(TypeWrapper::Pointer);
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
                wrapper->modifiers.pop_back();
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
            ret.imported_symbols.emplace_back(m_StringPool.intern(forwardStream().value));

            if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {
                forwardStream();  // skip "as"
                ret.imported_symbols.back().assigned_alias = m_StringPool.intern(forwardStream().value);
            }

            if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
                forwardStream();
            }
        } ignoreButExpect({PUNC, "}"});
    } else { // otherwise, if non-specific but aliased or wildcard
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {  // non-specific but aliased
            forwardStream();
            ret.alias = m_StringPool.intern(forwardStream().value);
        }
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "*") {  // wildcard
            forwardStream();
            ret.is_wildcard = true;
            ModuleMap.get(handle).insertExportedSymbolsInto([this, &ret](const std::string& name) {
                ret.imported_symbols.push_back({.actual_name = m_StringPool.intern(name)});
            });
        }
        else forwardStream();
    }

    if (ret.imported_symbols.empty() && !ret.is_wildcard) {
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

        // TODO: remove duplicate import-registration logic
        if (!ModuleMap.contains(builtin_handle)) {
            ModuleMap.insert(builtin_handle, m_Module->getModuleContext());

            ModuleMap.get(builtin_handle).parse(m_ErrorCallback);
            ModuleMap.get(builtin_handle).performSema(m_ErrorCallback);
        }

        ModuleMap.get(builtin_handle).insertExportedSymbolsInto([this, &builtin_import](const std::string& name) {
            builtin_import->imported_symbols.push_back({.actual_name = m_StringPool.intern(name)});
        });

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
    auto func_nd = m_Module->makeNode<Function>();
    SET_NODE_ATTRS(func_nd);

    const std::string func_ident = m_Stream.next().value;

    // handle the special case of `main`
    if (func_ident == "main" && !m_Module->isMainModule()) {
        reportError(ErrCode::MAIN_REDEFINED);
    }

    if (m_CurrentStructTy.back()) {
        const auto struct_scope = m_CurrentStructTy.back()->to<StructType>()->scope;
        assert(struct_scope);
        func_nd->ident = struct_scope->getNewIDInfo(func_ident);
    }

    bool method_is_static = true;
    forwardStream(); // skip the ID

    // check for generics
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        func_nd->generic_params = parseGenericParamList();
    }

    ignoreButExpect({PUNC, "("});
    auto function_t = std::make_unique<FunctionType>();


    const auto fn_scope = m_Module->symbol_table.newScope();  // create the function body scope

    // register the generic parameters
    for (auto& g_param : func_nd->generic_params) {
        const auto generic_id = fn_scope->getNewIDInfo(g_param->name);
        const auto generic_ty = new GenericType{};

        generic_ty->id = generic_id;
        g_param->id = generic_id;

        m_Module->symbol_table.registerType(generic_id, generic_ty);
    }

    // parse the parameters...
    if (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != ")") {
        while (m_Stream.CurTok.value != ")" && m_Stream.CurTok.type != PUNC) {
            func_nd->params.emplace_back(parseParam(method_is_static));
            if (m_Stream.CurTok.value == ",")
                forwardStream();
        }
    }

    // current token == ')'
    forwardStream();  // skip ')'

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();
        func_nd->return_type = parseType();
    }

    TableEntry entry;
    entry.swirl_type  = function_t.get();
    entry.is_exported = func_nd->is_exported;
    entry.method_of   = m_CurrentStructTy.back();
    entry.is_static   = method_is_static;
    entry.node_ptr    = func_nd;

    if (!m_CurrentStructTy.back()) {  // when the function is not a method
        // register the function in the global scope
        func_nd->ident = m_Module->symbol_table.registerDecl(func_ident, entry, 0);
    } else {
        // not a method, func_nd.ident has been set before
        assert(func_nd->ident);
        auto _ = m_Module->symbol_table.registerDecl(func_nd->ident, entry);
        assert(_);
    }

    function_t->ident = func_nd->ident;


    // register the function's signature as a type in the symbol manager
    m_Module->symbol_table.registerType(func_nd->ident, function_t.release());

    // ReSharper disable once CppDFALocalValueEscapesFunction
    m_LatestFuncNode = func_nd;
    NodeJmpTable[func_nd->ident] = func_nd;

    if (func_nd->is_extern) {
        // TODO: report an error if a body is provided
        m_Module->symbol_table.moveToPreviousScope();
        return func_nd;
    }

    // parse the children
    forwardStream();  // skip '{'
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}") && !m_Stream.eof()) {
        func_nd->children.push_back(dispatch());
    } forwardStream();
    m_Module->symbol_table.moveToPreviousScope();  // back to the global scope

    m_LatestFuncNode = nullptr;
    return func_nd;
}


Var* Parser::parseParam(bool& method_is_static) {
    Var* param = m_Module->makeNode<Var>();
    param->is_param = true;

    SET_NODE_ATTRS(param);

    param->is_const = true;  // all parameters are immutable

    // special case of `&self`
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "&") {
        forwardStream();
        bool is_mutable = false;

        if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "mut") {
            is_mutable = true;
            forwardStream();
        }

        assert(m_CurrentStructTy.back() != nullptr);
        auto ty = TypeWrapper(m_Module->symbol_table.getReferenceType(m_CurrentStructTy.back(), is_mutable));
        param->var_type  = m_Module->makeNode<TypeWrapper>(ty);
        param->var_ident = m_Module->symbol_table.registerDecl("self", {
            .is_param = true,
            .swirl_type = param->var_type->type,
        });

        method_is_static = false;
        ignoreButExpect({IDENT, "self"});
        return param;
    }

    const std::string var_name = m_Stream.CurTok.value;

    forwardStream(2);
    param->var_type = parseType();

    param->initialized = m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "=";
    if (param->initialized)
        param->value = parseExpr();


    TableEntry param_entry;
    // param_entry.swirl_type = param.var_type;
    param_entry.is_param = true;
    param->var_ident = m_Module->symbol_table.registerDecl(var_name, param_entry);

    return param;
}


std::vector<GenericParam*> Parser::parseGenericParamList() {
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
    } return params;
}


GenericArgList Parser::parseGenericArgList() {
    GenericArgList ret;
    auto& args = ret.generic_args;

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
            args.emplace_back(new GenericArg(parseExpr()));
        } else args.emplace_back(new GenericArg(parseType()));
    }

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

    TableEntry entry;
    entry.is_const    = ret->is_const;
    entry.is_volatile = ret->is_volatile;
    entry.is_exported = ret->is_exported;
    entry.node_ptr    = ret;
    // entry.swirl_type  = var_node->var_type;

    ret->var_ident = m_Module->symbol_table.registerDecl(var_ident, entry);

    // is a global variable
    if (m_RecursionDepth == 1) {
        NodeJmpTable.insert({ret->getIdentInfo(), ret});
    }

    return ret;
}


Node* Parser::parseCall(std::optional<Ident*> ident) {
    auto call_node = m_Module->makeNode<FuncCall>();
    SET_NODE_ATTRS(call_node);
    call_node->ident = ident.value();

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        call_node->generic_args = parseGenericArgList();
    }

    forwardStream();  // skip '('
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ")")) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ",") {
            forwardStream();
            continue;
        }

        call_node->args.emplace_back(parseExpr());
    } forwardStream();

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

        Expression* arg;
        call_node->ident->full_qualification.push_back({.name = m_StringPool.intern("sizeof")});
        call_node->intrinsic_type = Intrinsic::SIZEOF;

        if (m_Stream.CurTok.value == "@" && m_Stream.CurTok.type == PUNC)
             arg = m_Module->makeNode<Expression>(Expression::makeExpression(parseIntrinsic()));
        else arg = m_Module->makeNode<Expression>(Expression::makeExpression(parseType()));

        call_node->args.push_back(arg);
        ignoreButExpect({PUNC, ")"});

    } else *call_node = dynamic_cast<FuncCall*>(parseCall(parseIdent()));

    return call_node;
}


Node* Parser::parseScope() {
    auto scope = m_Module->makeNode<Scope>();
    SET_NODE_ATTRS(scope);

    forwardStream(); // skip '{'
    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            break;
        }

        if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}") {
            forwardStream();
            break;
        }

        scope->children.emplace_back(dispatch());
    } return scope;
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

    forwardStream();  // skip the opening brace

    m_Module->symbol_table.newScope();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}"))
        cnd->if_children.push_back(dispatch());
    forwardStream();
    m_Module->symbol_table.moveToPreviousScope();

    // handle `else(s)`
    if (!(m_Stream.CurTok.type == KEYWORD && (m_Stream.CurTok.value == "else" || m_Stream.CurTok.value == "elif")))
        return cnd;

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
        while (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
            m_Module->symbol_table.newScope();
            forwardStream();

            std::tuple<Expression*, std::vector<Node*>> children;
            std::get<0>(children) = parseExpr();

            if (cnd->is_comptime) {
                std::get<0>(children) = m_Module->makeNode<Expression>(Expression::makeExpression(
                    std::get<0>(children)->evaluate(*this)));
            }

            forwardStream();
            while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
                std::get<1>(children).push_back(dispatch());
            } forwardStream();

            cnd->elif_children.emplace_back(std::move(children));
            m_Module->symbol_table.moveToPreviousScope();
        }
    }

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "else") {
        m_Module->symbol_table.newScope();
        forwardStream(2);
        while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
            cnd->else_children.push_back(dispatch());
        } forwardStream();
        m_Module->symbol_table.moveToPreviousScope();
    }

    return cnd;
}


Ident* Parser::parseIdent() {
    auto* ret = m_Module->makeNode<Ident>();
    SET_NODE_ATTRS(ret);

    ret->full_qualification.emplace_back(m_StringPool.intern(forwardStream().value));
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "!") {
        forwardStream();
        ret->full_qualification.back().generic_args = parseGenericArgList();
    }

    while (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "::") {
        forwardStream();
        ret->full_qualification.emplace_back(m_StringPool.intern(forwardStream().value));

        // check for generics
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "!") {
            forwardStream();
            ret->full_qualification.back().generic_args = parseGenericArgList();
        }
    }

    if (ret->full_qualification.size() == 1 && ret->full_qualification.at(0).generic_args.empty()) {
        ret->value = m_Module->symbol_table.getIDInfoFor(
            std::string(ret->full_qualification.front().name));
    }

    assert(!ret->full_qualification.empty());
    return ret;
}


Node* Parser::parseEnum() {
    auto ret = m_Module->makeNode<Enum>();
    SET_NODE_ATTRS(ret);

    forwardStream();  // skip 'enum'
    TableEntry entry;
    entry.is_enum  = true;
    entry.node_ptr = ret;
    ret->ident  = m_Module->symbol_table.registerDecl(forwardStream().value, entry);

    auto scope = m_Module->symbol_table.newScope();
    m_Module->symbol_table.lookupDecl(ret->ident).scope = scope;

    auto ty = new EnumType();
    ty->scope = entry.scope;
    ty->id = ret->ident;

    m_Module->symbol_table.registerType(ret->ident, ty);

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
            auto name = m_StringPool.intern(forwardStream().value);
            ret->addEntry(name);

            const auto id_info = scope->getNewIDInfo(name, true);
            m_Module->symbol_table.registerFictitiousID(id_info, ret);
            continue;
        }

        reportError(ErrCode::SYNTAX_ERROR, {.msg = "Expected identifier."});
        forwardStream();
    } forwardStream();  // skip '}'

    m_Module->symbol_table.moveToPreviousScope();

    if (m_RecursionDepth == 1) {
        NodeJmpTable[ret->ident] = ret;
    }

    return ret;
}


Node* Parser::parseProtocol() {
    auto ret = m_Module->makeNode<Protocol>();
    SET_NODE_ATTRS(ret);

    forwardStream(); // skip 'protocol'
    ret->protocol_name = m_StringPool.intern(forwardStream().value);

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
                ret->members.push_back(Protocol::MemberSignature{
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

                ret->methods.push_back(Protocol::MethodSignature{
                    .name = m_StringPool.intern(func->ident->toString()),
                    .return_type = func->return_type,
                    .params = std::move(func_params),
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

    const TableEntry entry{.is_protocol = true, .node_ptr = ret};
    ret->protocol_id = m_Module->symbol_table.registerDecl(std::string(ret->protocol_name), entry);

    if (m_RecursionDepth == 1) {
        NodeJmpTable.insert({ret->protocol_id, ret});
    }

    return ret;
}


std::vector<Ident*> Parser::parseProtocolList() {
    std::vector<Ident*> ret;
    forwardStream();  // skip ':'

    while (true) {
        if (m_Stream.eof()) {
            reportError(ErrCode::UNEXPECTED_EOF);
            return ret;
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

    return ret;
}


Node* Parser::parseStruct() {
    forwardStream();  // skip 'struct'
    auto ret = m_Module->makeNode<Struct>();
    SET_NODE_ATTRS(ret);

    const auto struct_ty = new StructType{};
    const auto struct_name = forwardStream().value;
    m_CurrentStructTy.push_back(struct_ty);

    // ask for a decl registry from the symbol manager
    ret->ident = m_Module->symbol_table.registerDecl(struct_name, {
        .is_exported = ret->is_exported,
    });

    // parse generic parameters
    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "<") {
        ret->generic_params = parseGenericParamList();
    }

    // register the type and the scope of the struct as a qualifier
    m_Module->symbol_table.registerType(ret->ident, struct_ty);

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        ret->protocols = parseProtocolList();
    }

    // create the struct's scope
    ignoreButExpect({PUNC, "{"});  // skip '{'
    const auto scope_pointer = m_Module->symbol_table.newScope();
    struct_ty->scope = scope_pointer;

    m_Module->symbol_table.lookupDecl(ret->ident).scope = scope_pointer;

    // register the generic parameters in the scope
    for (auto& param : ret->generic_params) {
        param->id = scope_pointer->getNewIDInfo(param->name);
        m_Module->symbol_table.registerType(param->id, new GenericType());
    }

    // handle the children
    std::size_t i = 0;
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        m_LastSymWasExported = true;
        auto member = dispatch();
        if (member->getNodeType() == ND_VAR) {
            struct_ty->field_offsets.insert(
                {m_StringPool.intern(member->getIdentInfo()->toString()).data(), i++});
        }
        ret->members.emplace_back(member);
    } ignoreButExpect({PUNC, "}"});  // skip '}'

    m_CurrentStructTy.pop_back();

    // exit the struct's scope
    m_Module->symbol_table.moveToPreviousScope();

    struct_ty->ident = ret->ident;
    m_Module->symbol_table.lookupDecl(ret->ident).scope = scope_pointer;

    if (m_RecursionDepth == 1) {
        NodeJmpTable.insert({ret->getIdentInfo(), ret});
    }

    return ret;
}


Node* Parser::parseWhile() {
    auto ret = m_Module->makeNode<WhileLoop>();
    SET_NODE_ATTRS(ret);

    forwardStream();
    ret->condition = parseExpr();

    m_Module->symbol_table.newScope();
    forwardStream();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        ret->children.push_back(dispatch());
    } forwardStream();
    m_Module->symbol_table.moveToPreviousScope();

    return ret;
}


Expression* Parser::parseExpr() {
    const auto ret = m_Module->makeNode<Expression>(m_ExpressionParser.parseExpr());
    SET_NODE_ATTRS(ret);
    return ret;
}