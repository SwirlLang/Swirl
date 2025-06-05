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
#include <managers/SymbolManager.h>
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

    if (m_ReturnFakeToken.has_value()) {
        auto tok = m_ReturnFakeToken.value();
        m_ReturnFakeToken = std::nullopt;
        return std::move(tok);
    }

    for (uint8_t _ = 0; _ < n; _++)
        m_Stream.next();

    return std::move(ret);
}


Type* Parser::parseType() {
    Type* base_type = nullptr;

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "&") {
        forwardStream();
        return SymbolTable.getReferenceType(
            SymbolTable.lookupType(SymbolTable.getIDInfoFor(forwardStream().value)));
    }

    if (m_Stream.CurTok.type == IDENT) {
        base_type = SymbolTable.lookupType(SymbolTable.getIDInfoFor(forwardStream().value));
    }

    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "[") {
        forwardStream();
        base_type = parseType();

        if (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != ";") {
            // ErrMan.newSyntaxError("Expected ';' (syntax: [<type>; <size>]).");
            return nullptr;
        }

        forwardStream();
        if (m_Stream.CurTok.type != NUMBER || m_Stream.CurTok.meta != CT_INT) {
            reportError(ErrCode::NON_INT_ARRAY_SIZE);
            return nullptr;
        }

        const auto ret = SymbolTable.getArrayType(base_type, std::stoi(forwardStream().value));
        forwardStream();
        return ret;
    }

    uint16_t ptr_level = 0;
    while (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "*") {
        ptr_level++;
        forwardStream();
    }

    if (ptr_level) {
        return SymbolTable.getPointerType(base_type, ptr_level);
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
            case IDENT: return std::make_unique<Expression>(parseExpr());
            case OP:
                assignment_lhs:
                {
                    Expression expr = parseExpr();
                    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
                        Assignment ass{};
                        ass.l_value = std::move(expr);

                        forwardStream();  // skip "="
                        ass.r_value = parseExpr();
                        return std::make_unique<Assignment>(std::move(ass));
                    }  continue;
                }
            case PUNC:
                if (m_Stream.CurTok.value == "[")
                    return std::make_unique<Expression>(parseExpr());
                if (m_Stream.CurTok.value == "(")
                    goto assignment_lhs;  // sorry
                if (m_Stream.CurTok.value == ";") {
                    forwardStream();
                    continue;
                } if (m_Stream.CurTok.value == "}") {
                    return std::make_unique<Node>();
                }
            default:
                reportError(ErrCode::SYNTAX_ERROR);
        }
    }

    reportError(ErrCode::UNEXPECTED_EOF);
    return {};
}


/*                 Examples                                *
 *          ------------------------                       *
 * import dir1::dir2::mod::sym1 as stuff;                  *
 * import dir1::dir2::mod::{ sym1, sym2 as other_stuff };  *
 */
std::unique_ptr<ImportNode> Parser::parseImport() {
    ImportNode ret;
    ret.location = m_Stream.getStreamState();

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
        } forwardStream();
    } else {
        if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "as") {
            forwardStream();
            ret.alias = forwardStream().value;
        } else forwardStream();
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

    m_LastSymWasExported = false;
    m_LastSymIsExtern = false;

    m_Stream.expectTypes({IDENT});
    const std::string func_ident = m_Stream.next().value;

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

        return std::move(param);
    };

    SymbolTable.newScope();  // emplace the function body scope
    if (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != ")") {
        while (m_Stream.CurTok.value != ")" && m_Stream.CurTok.type != PUNC) {
            func_nd.params.emplace_back(parse_params());
            if (m_Stream.CurTok.value == ",")
                forwardStream();
        }
    }
    SymbolTable.destroyLastScope();  // decrement the scope index

    // current token == ')'
    forwardStream();

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == ":") {
        forwardStream();
        // func_nd.ret_type = parseType();
        function_t->ret_type = parseType();
    }

    TableEntry entry;
    entry.swirl_type = function_t.get();
    entry.is_exported = func_nd.is_exported;
    func_nd.ident = SymbolTable.registerDecl(func_ident, entry);
    function_t->ident = func_nd.ident;

    SymbolTable.registerType(func_nd.ident, function_t.release());
    
    // WARNING: func_nd has been MOVED here!!
    auto ret = std::make_unique<Function>(std::move(func_nd));
    // ReSharper disable once CppDFALocalValueEscapesFunction
    m_LatestFuncNode = ret.get();
    NodeJmpTable[ret->ident] = ret.get();

    // parse the children
    forwardStream();
    SymbolTable.lockNewScpEmplace();  // to reuse the scope we created for params above
    SymbolTable.newScope();  // increment the scope index
    SymbolTable.unlockNewScpEmplace();  // undo the locking of scope-emplace

    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        ret->children.push_back(dispatch());
    } forwardStream();
    SymbolTable.destroyLastScope();  // decrement the scope index, we onto the global scope now

    return std::move(ret);
}

std::unique_ptr<Var> Parser::parseVar(const bool is_volatile) {
    Var var_node;

    var_node.location = m_Stream.getStreamState();
    var_node.is_const = m_Stream.CurTok.value[0] == 'l';
    var_node.is_volatile = is_volatile;
    var_node.is_exported = m_LastSymWasExported;
    var_node.is_extern = m_LastSymIsExtern;

    m_LastSymWasExported = false;
    m_LastSymIsExtern = false;

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

    if (!var_node.var_type)
        var_node.var_type = var_node.value.expr_type;
    else
        var_node.value.expr_type = var_node.var_type;

    entry.swirl_type = var_node.var_type;
    var_node.var_ident = SymbolTable.registerDecl(var_ident, entry);

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
    cnd.bool_expr = parseExpr(Token{PUNC, "{"});
    cnd.bool_expr.location = m_Stream.getStreamState();

    forwardStream();  // skip the opening brace

    SymbolTable.newScope();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}"))
        cnd.if_children.push_back(std::move(dispatch()));
    forwardStream();
    SymbolTable.destroyLastScope();

    // handle `else(s)`
    if (!(m_Stream.CurTok.type == KEYWORD && (m_Stream.CurTok.value == "else" || m_Stream.CurTok.value == "elif")))
        return std::make_unique<Condition>(std::move(cnd));

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
        while (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "elif") {
            SymbolTable.newScope();
            forwardStream();

            std::tuple<Expression, std::vector<SwNode>> children;
            std::get<0>(children) = parseExpr(Token{PUNC, "{"});

            forwardStream();
            while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
                std::get<1>(children).push_back(dispatch());
            } forwardStream();

            cnd.elif_children.emplace_back(std::move(children));
            SymbolTable.destroyLastScope();
        }
    }

    if (m_Stream.CurTok.type == KEYWORD && m_Stream.CurTok.value == "else") {
        SymbolTable.newScope();
        forwardStream(2);
        while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
            cnd.else_children.push_back(dispatch());
        } forwardStream();
        SymbolTable.destroyLastScope();
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
    forwardStream();
    Struct ret{};

    ret.ident = SymbolTable.getIDInfoFor<true>(m_Stream.CurTok.value);

    auto struct_t = std::make_unique<StructType>();
    struct_t->ident = ret.ident;

    forwardStream(2);

    SymbolTable.newScope();
    int mem_index = 0;

    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        auto member = dispatch();

        if (member->getNodeType() == ND_VAR) {
            const auto field_ptr = dynamic_cast<Var*>(member.get());
            struct_t->fields.insert({
                field_ptr->var_ident, {mem_index, field_ptr->var_type}
            }); mem_index++;
        } ret.members.push_back(std::move(member));
    }

    forwardStream();
    SymbolTable.destroyLastScope();
    SymbolTable.registerType(ret.ident, struct_t.release());
    return std::make_unique<Struct>(std::move(ret));
}

std::unique_ptr<WhileLoop> Parser::parseWhile() {
    WhileLoop loop{};
    forwardStream();

    loop.condition = parseExpr(Token{PUNC, "{"});

    SymbolTable.newScope();
    forwardStream();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        loop.children.push_back(dispatch());
    } forwardStream();
    SymbolTable.destroyLastScope();

    return std::make_unique<WhileLoop>(std::move(loop));
}

Expression Parser::parseExpr(const std::optional<Token>& terminator) {
    const
    std::function<SwNode()> parse_component = [&, this] -> SwNode {
        // this helper returns a packaged element-access operator if it detects its presence
        auto package_element_access =  [this, parse_component] (SwNode& operand_1) -> std::optional<SwNode> {
            if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "[") {
                forwardStream();
                auto op = std::make_unique<Op>("[]");
                op->location = m_Stream.getStreamState();
                op->operands.push_back(std::move(operand_1));
                op->operands.push_back(parse_component());

                if (m_Stream.CurTok.type != PUNC && m_Stream.CurTok.value != "]") {
                    reportError(ErrCode::UNMATCHED_SQ_BRACKET);
                    return std::nullopt;
                }
                forwardStream();
                return std::move(op);
            } return std::nullopt;
        };

        switch (m_Stream.CurTok.type) {
            case NUMBER: {
                if (m_Stream.CurTok.meta == CT_FLOAT) {
                    auto ret = std::make_unique<FloatLit>(m_Stream.CurTok.value);
                    forwardStream();
                    return std::move(ret);
                }

                auto ret = std::make_unique<IntLit>(m_Stream.CurTok.value);

                forwardStream();
                return std::move(ret);
            }
            case STRING: {
                std::unique_ptr<Node> str = std::make_unique<StrLit>("");
                while (m_Stream.CurTok.type == STRING) {  // concatenation of adjacent string literals
                    str->value += m_Stream.CurTok.value;
                    forwardStream();
                }
                if (auto op = package_element_access(str)) {
                    return std::move(*op);
                } return std::move(str);
            }
            case IDENT: {
                auto id = parseIdent();
                if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                    std::unique_ptr<Node> call_node = parseCall(std::move(id));
                    if (auto op = package_element_access(call_node)) {
                        return std::move(*op);
                    } return std::move(call_node);
                }

                std::unique_ptr<Node> id_node = std::make_unique<Ident>(std::move(id));
                if (auto op = package_element_access(id_node)) {
                    return std::move(*op);
                } return std::move(id_node);
            }
            case PUNC: {
                if (m_Stream.CurTok.value == "[") {
                    ArrayNode arr;
                    arr.location = m_Stream.getStreamState();
                    forwardStream();  // skip the '['

                    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "]")) {
                        arr.elements.push_back(parseExpr());

                        if (m_Stream.CurTok.type == PUNC) {
                            if (m_Stream.CurTok.value == ",") {
                                forwardStream();
                                continue;
                            }
                        }
                        if (m_Stream.peek().type != PUNC && m_Stream.peek().value != "]")
                            reportError(ErrCode::COMMA_SEP_REQUIRED);
                    } forwardStream();

                    std::unique_ptr<Node> arr_node = std::make_unique<ArrayNode>(std::move(arr));
                    if (auto op = package_element_access(arr_node)) {
                        return std::move(*op);
                    } return std::move(arr_node);
                }

                if (m_Stream.CurTok.value == "(") {
                    forwardStream();
                    auto ret = std::make_unique<Expression>(parseExpr());
                    ret->location = m_Stream.getStreamState();
                    forwardStream();
                    return std::move(ret);
                } return dispatch();
            }

            default: {
                reportError(ErrCode::EXPECTED_EXPRESSION);
                return nullptr;
            }
        }
    };

    const
    std::function<std::optional<SwNode>()> parse_prefix = [&, this] -> std::optional<SwNode> {
        // assumption: current token is an OP
        if (m_Stream.CurTok.type == OP) {
            Op op(m_Stream.CurTok.value);
            op.arity = 1;
            op.location = m_Stream.getStreamState();
            forwardStream();
            if (auto next_op = parse_prefix()) {
                op.operands.push_back(std::move(*next_op));
                return std::make_unique<Op>(std::move(op));
            } return std::nullopt;
        }
        return parse_component();
    };

    const auto continue_parsing = [this]() -> bool {
        return m_Stream.CurTok.type == OP ||
            (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "[");
    };

    const
    std::function<SwNode(SwNode, int, bool)> handle_binary =
        [&, this](SwNode prev, const int last_prec = -1, bool is_left_associative = false) {
        // assumption: current token is an OP

        is_left_associative = m_Stream.CurTok.value == "." || m_Stream.CurTok.value == "-";
        SwNode op = std::make_unique<Op>(m_Stream.CurTok.value == "[" ? "[]" : m_Stream.CurTok.value);
        op->location = m_Stream.getStreamState();
        const int current_prec = operators.at(m_Stream.CurTok.value == "[" ? "[]" : m_Stream.CurTok.value);

        forwardStream();  // we are onto the rhs

        if (last_prec < 0) {
            auto rhs = m_Stream.CurTok.type == OP ? parse_prefix().value() : parse_component();

            op->getMutOperands().push_back(std::move(prev));
            op->getMutOperands().push_back(std::move(rhs));

            // if there's another operator...
            if (continue_parsing()) {
                if (!op) throw std::invalid_argument("Invalid operation");
                op = handle_binary(std::move(op), current_prec, is_left_associative);
            }
        }

        else if (current_prec < last_prec || is_left_associative) {
            auto rhs = m_Stream.CurTok.type == OP ? parse_prefix().value() : parse_component();

            if (!rhs || !prev) throw std::invalid_argument("rhs || prev: Invalid operation");
            op->getMutOperands().push_back(std::move(prev));
            op->getMutOperands().push_back(std::move(rhs));


            if (continue_parsing())
                op = handle_binary(std::move(op), current_prec, is_left_associative);

            return std::move(op);
        }

        else if (current_prec >= last_prec) {
            SwNode tmp_operand = std::move(prev->getMutOperands().back());
            prev->getMutOperands().pop_back();
            // prepare the higher-precedence Op and push it as an operand
            auto rhs = m_Stream.CurTok.type == OP ? parse_prefix().value() : parse_component();

            if (!tmp_operand || !rhs) throw std::invalid_argument("rhs || prev: Invalid operation");

            op->getMutOperands().push_back(std::move(tmp_operand));
            op->getMutOperands().push_back(std::move(rhs));

            prev->getMutOperands().push_back(std::move(op));

            if (continue_parsing()) {
                op = handle_binary(std::move(prev), current_prec, is_left_associative);
                return std::move(op);
            }
            return std::move(prev);
        }

        return std::move(op);
    };

    auto check_for_terminator = [this, &terminator] {
        if (terminator.has_value()) {
            if (m_Stream.CurTok != terminator.value()) {
                // ErrMan.newSyntaxError("Expected '" + terminator->value + "'");
                m_ReturnFakeToken = terminator.value();
            }
        }
    };

    Expression ret;
    if (m_Stream.CurTok.type == OP) {
        auto op = parse_prefix().value();

        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(op), -1, false));

            check_for_terminator();
            return std::move(ret);
        }

        ret.expr.push_back(std::move(op));

        check_for_terminator();
        ret.location = m_Stream.getStreamState();
        return std::move(ret);
    }

    auto tmp = parse_component();

    if (continue_parsing()) {
        ret.expr.push_back(handle_binary(std::move(tmp), -1, false));

        check_for_terminator();
        ret.location = m_Stream.getStreamState();
        return std::move(ret);
    }
    ret.expr.push_back(std::move(tmp));

    check_for_terminator();
    ret.location = m_Stream.getStreamState();
    return std::move(ret);
}