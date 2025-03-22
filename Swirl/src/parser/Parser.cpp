#include <filesystem>
#include <iterator>
#include <memory>
#include <fstream>
#include <print>
#include <stdexcept>
#include <utility>
#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>

#include <utils/utils.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <tokenizer/Tokens.h>
#include <definitions.h>
#include <backend/LLVMBackend.h>
#include <managers/ErrorManager.h>
#include <managers/SymbolManager.h>
#include <parser/SemanticAnalysis.h>


using SwNode = std::unique_ptr<Node>;

extern std::string     SW_OUTPUT;
extern std::string     SW_FED_FILE_PATH;
extern std::thread::id MAIN_THREAD_ID;

extern std::optional<ThreadPool_t> ThreadPool;

extern void printIR(const LLVMBackend& instance);
extern void GenerateObjectFileLLVM(const LLVMBackend&);

ModuleMap_t ModuleMap;


Parser::Parser(const std::filesystem::path& path)
    : m_Stream{path}
    , m_FilePath{path}
    , ErrMan{&m_Stream}
{
    m_Stream.ErrMan = &ErrMan;
    m_RelativeDir = path.parent_path();
}


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
    ParsedType pt;

    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "&") {
        pt.is_reference = true;
        forwardStream();
    }

    pt.ident = SymbolTable.getIDInfoFor(forwardStream().value);
    return SymbolTable.lookupType(pt.ident);

    // TODO handle pointers and references
}

SwNode Parser::dispatch() {
    while (!m_Stream.eof()) {
        const TokenType type  = m_Stream.CurTok.type;
        const std::string value = m_Stream.CurTok.value;

        // pattern matching in C++ when?
        switch (m_Stream.CurTok.type) {
            case KEYWORD:
                if (m_Stream.CurTok.value == "const" || m_Stream.CurTok.value == "var") {
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
                    handleImports();
                    continue;
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

            case IDENT:
                {
                    auto id = parseIdent();
                    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                        return parseCall(std::move(id));
                    }
                }

                if (const Token p_tk = m_Stream.peek();
                    p_tk.type == PUNC && p_tk.value == "(") {
                    auto nd = parseCall();

                    // handle call assignments
                    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value.ends_with("=")) {
                        Assignment ass{};
                        ass.op = m_Stream.CurTok.value;
                        ass.l_value.expr.push_back(std::move(nd));
                        forwardStream();
                        ass.r_value = parseExpr();

                        return std::make_unique<Assignment>(std::move(ass));
                    }

                    return std::move(nd);
                }

                {
                    Expression expr = parseExpr();

                    if (m_Stream.CurTok.type == OP && m_Stream.CurTok.value == "=") {
                        Assignment ass;
                        ass.l_value = std::move(expr);

                        forwardStream();
                        ass.r_value = parseExpr();
                        return std::make_unique<Assignment>(std::move(ass));
                    } continue;
                }
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
                if (m_Stream.CurTok.value == "(")
                    goto assignment_lhs;  // sorry
                if (m_Stream.CurTok.value == ";") {
                    forwardStream();

                    continue;
                } if (m_Stream.CurTok.value == "}") {
                    return std::make_unique<Node>();
                }
            default:
                auto [line, _, col] = m_Stream.getStreamState();
                throw std::runtime_error("dispatch: nothing found");
        }
    }
    throw std::runtime_error("dispatch: this wasn't supposed to happen");
}


void Parser::handleImports() {
    forwardStream();  // skip 'import'


    if (m_Stream.CurTok.type == STRING) {
        const auto mod_path = m_RelativeDir / m_Stream.CurTok.value;

        if (!std::filesystem::exists(mod_path)) {
            throw std::runtime_error("import of a non-existent file!");
        }
        
        forwardStream();
        if (m_Stream.CurTok.type != KEYWORD && m_Stream.CurTok.value == "as") {
            throw std::runtime_error("aliasing is required here!");
        }

        forwardStream();
        SymbolTable.registerModuleAlias(forwardStream().value, mod_path);


        // do nothing if the module is already being parsed
        if (ModuleMap.contains(mod_path)) {
            return;
        }
        
        Parser mod_parser{mod_path};
        ModuleMap.insert(mod_path, std::move(mod_parser)); // MOVED!!
        ThreadPool->addTask([mod_path] { ModuleMap.get(mod_path).parse(); });
    }
}

void Parser::parse() {
    // uncomment to check the stream's output for debugging
    // while (!m_Stream.eof()) {
    //     std::cout << m_Stream.p_CurTk.value << ": " << to_string(m_Stream.p_CurTk.type) << " peek: " << m_Stream.peek().value << std::endl;
    //     m_Stream.next();
    // }

    forwardStream();
    while (!m_Stream.eof()) {
        AST.emplace_back(dispatch());
    }
    
    runPendingVerifications();

    AnalysisContext analysis_ctx{*this};
    analysis_ctx.startAnalysis();
}


void Parser::callBackend() {
    SymbolTable.lockNewScpEmplace();
    ErrMan.raiseAll();

    LLVMBackend llvm_instance{std::move(AST), m_FilePath.string(), std::move(SymbolTable), std::move(ErrMan)};  // TODO
    llvm_instance.startGeneration();
    
    printIR(llvm_instance);
    GenerateObjectFileLLVM(llvm_instance);
}

/// returns whether `t1` is implicitly convertible to `t2`
bool implicitlyConvertible(Type* t1, Type* t2) {
    // TODO: handled signed-ness
    if (t1->isIntegral() && t2->isIntegral()) {
        return t1->getBitWidth() <= t2->getBitWidth();
    }
    return false;
}

std::unique_ptr<Function> Parser::parseFunction() {
    Function func_nd;
    
    func_nd.is_exported = m_LastSymWasExported;
    func_nd.is_extern = m_LastSymIsExtern;

    m_LastSymWasExported = false;
    m_LastSymIsExtern = false;

    m_Stream.expectTypes({IDENT});
    const std::string func_ident = m_Stream.next().value;

    m_Stream.expectTokens({Token{PUNC, "("}});
    forwardStream(2);
    auto function_t = std::make_unique<FunctionType>();
    func_nd.reg_ret_type = &function_t->ret_type;

     
    auto parse_params = [function_t = function_t.get(), this] {
        Var param;
        const std::string var_name = m_Stream.CurTok.value;

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
    func_nd.ident = SymbolTable.registerDecl(func_ident, entry);
    function_t->ident = func_nd.ident;

    SymbolTable.registerType(func_nd.ident, function_t.release());
    
    // WARNING: func_nd has been MOVED here!!
    auto ret = std::make_unique<Function>(std::move(func_nd));
    // ReSharper disable once CppDFALocalValueEscapesFunction
    m_LatestFuncNode = ret.get();

    // parse the children
    forwardStream();
    SymbolTable.lockNewScpEmplace();  // to reuse the scope we created for params above
    SymbolTable.newScope();  // increment the scope index
    SymbolTable.unlockNewScpEmplace();  // undo the locking of scope-emplaces

    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        ret->children.push_back(dispatch());
    } forwardStream();
    SymbolTable.destroyLastScope();  // decrement the scope index, we onto the global scope now

    return std::move(ret);
}

std::unique_ptr<Var> Parser::parseVar(const bool is_volatile) {
    Var var_node;

    var_node.is_const = m_Stream.CurTok.value[0] == 'c';
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

    if (!var_node.var_type)
        var_node.var_type = var_node.value.expr_type;
    else
        var_node.value.expr_type = var_node.var_type;

    entry.swirl_type = var_node.var_type;
    var_node.var_ident = SymbolTable.registerDecl(var_ident, entry);

    return std::make_unique<Var>(std::move(var_node));
}

std::unique_ptr<FuncCall> Parser::parseCall(const std::optional<ParsedIdent> ident) {
    auto call_node = std::make_unique<FuncCall>();

    auto str_ident = ident->name;
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

    VerificationQueue.emplace([str_ident, ptr = call_node.get(), mod_path = ident->mod_path, this] {
        if (mod_path.empty()) {
            ptr->ident = SymbolTable.getIDInfoFor(str_ident);
        } else {
            std::println("mod-path: {}", mod_path.string());
            auto future = ModuleMap.get(mod_path).SymbolTable.subscribeForTableEntry(str_ident);
            std::pair<IdentInfo*, TableEntry*> entry = future.get();

            ptr->ident = entry.first;
            ptr->signature = entry.second->swirl_type;

            // TODO: match the signature against the arguments' types
        }
    });

    if (m_Stream.CurTok.value == ")") {
        forwardStream();
    }
    return call_node;
}

std::unique_ptr<ReturnStatement> Parser::parseRet() {
    ReturnStatement ret;

    forwardStream();
    if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == ";")
        return std::make_unique<ReturnStatement>(std::move(ret));

    ret.value = parseExpr();
    // if (m_LatestFuncNode->ret_type == nullptr) {
    //     m_LatestFuncNode->updateRetTypeTo(ret.value.expr_type);
    //     ret.parent_ret_type = ret.value.expr_type;
    // }
    // else {
        // if (!implicitlyConvertible(ret.value.expr_type, m_LatestFuncNode->ret_type)) {
        //     throw std::runtime_error("returned value type-mismatch");
        // }
    //     ret.value.expr_type = m_LatestFuncNode->ret_type;
    //     ret.parent_ret_type = m_LatestFuncNode->ret_type;
    // }

    return std::make_unique<ReturnStatement>(std::move(ret));
}


std::unique_ptr<Condition> Parser::parseCondition() {
    Condition cnd;
    forwardStream();  // skip "if"
    cnd.bool_expr = parseExpr(Token{PUNC, "{"});
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

            std::tuple<Expression, std::vector<SwNode>> children{};
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

Parser::ParsedIdent Parser::parseIdent() {
    ParsedIdent ret;  

    // assuming current token is an ident
    if (m_Stream.peek().type == OP && m_Stream.peek().value == "::") {
        ret.mod_path = SymbolTable.getModuleFromAlias(m_Stream.CurTok.value);
        std::println("module path: {}", ret.mod_path.string());
        forwardStream(2);
    }
    ret.name = m_Stream.CurTok.value;
    forwardStream();
    
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
            struct_t->fields.insert({field_ptr->var_ident, {mem_index, field_ptr->var_type}});
            mem_index++;
        }

        ret.members.push_back(std::move(member));
    }

    forwardStream();
    SymbolTable.destroyLastScope();
    SymbolTable.registerType(ret.ident, struct_t.release());
    return std::make_unique<Struct>(std::move(ret));
}

std::unique_ptr<WhileLoop> Parser::parseWhile() {
    WhileLoop loop{};
    forwardStream();

    loop.condition = parseExpr();

    SymbolTable.newScope();
    forwardStream();
    while (!(m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "}")) {
        loop.children.push_back(dispatch());
    } forwardStream();
    SymbolTable.destroyLastScope();

    return std::make_unique<WhileLoop>(std::move(loop));
}

Expression Parser::parseExpr(const std::optional<Token>& terminator) {
    Type* deduced_type = nullptr;

    const
    std::function<SwNode()> parse_component = [&, this] -> SwNode {
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
                std::string str;
                while (m_Stream.CurTok.type == STRING) {  // concatenation of adjacent string literals
                    str += m_Stream.CurTok.value;
                    forwardStream();
                } return std::make_unique<StrLit>(str);
            }
            case IDENT: {
                auto id = parseIdent();   
                if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                    auto call_node = parseCall(std::move(id));
                    return std::move(call_node);
                }
 
                auto id_node = std::make_unique<Ident>(SymbolTable.getIDInfoFor(id.name));

                return std::move(id_node);
            }
            default: {
                if (m_Stream.CurTok.type == PUNC && m_Stream.CurTok.value == "(") {
                    forwardStream();
                    auto ret = std::make_unique<Expression>(parseExpr());
                    // deduceType(&deduced_type, ret->expr_type);
                    forwardStream();
                    return std::move(ret);
                } return dispatch();
            }
        }
    };

    const
    std::function<std::optional<SwNode>()> parse_prefix = [&, this] -> std::optional<SwNode> {
        // assumption: current token is an OP
        if (m_Stream.CurTok.type == OP) {
            Op op(m_Stream.CurTok.value);
            op.arity = 1;
            forwardStream();
            if (auto next_op = parse_prefix()) {
                op.operands.push_back(std::move(*next_op));
                return std::make_unique<Op>(std::move(op));
            } return std::nullopt;
        }
        return parse_component();
    };

    const auto continue_parsing = [this]() -> bool {
        return m_Stream.CurTok.type == OP && m_Stream.CurTok.value != "=";
    };

    const
    std::function<SwNode(SwNode, int, bool)> handle_binary =
        [&, this](SwNode prev, const int last_prec = -1, bool is_left_associative = false) {
        // assumption: current token is an OP

        is_left_associative = m_Stream.CurTok.value == "." || m_Stream.CurTok.value == "-";
        SwNode op = std::make_unique<Op>(m_Stream.CurTok.value);
        const int current_prec = operators.at(m_Stream.CurTok.value);

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
                ErrMan.newSyntaxError("Expected '" + terminator->value + "'");
                m_ReturnFakeToken = terminator.value();
            }
        }
    };


    Expression ret;
    if (m_Stream.CurTok.type == OP) {
        auto op = parse_prefix().value();

        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(op), -1, false));
            ret.expr_type = deduced_type;

            check_for_terminator();
            return std::move(ret);
        }
        
        ret.expr.push_back(std::move(op));
        ret.expr_type = deduced_type;

        check_for_terminator();
        return std::move(ret);
        
    } else {
        auto tmp = parse_component();

        if (continue_parsing()) {
            ret.expr.push_back(handle_binary(std::move(tmp), -1, false));
            ret.expr_type = deduced_type;

            check_for_terminator();
            return std::move(ret);
        }
        ret.expr.push_back(std::move(tmp));
        ret.expr_type = deduced_type;

        check_for_terminator();
        return std::move(ret);
    }
}