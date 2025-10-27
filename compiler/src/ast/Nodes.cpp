#include "ast/Nodes.h"
#include "parser/Parser.h"


struct OpInfo {
    enum Associativity { LEFT, RIGHT };

    int precedence;
    Associativity associativity;

    OpInfo(const int prec, const Associativity ass) : precedence(prec), associativity(ass) {}
};


const static
std::unordered_map<Op::OpTag_t, OpInfo> OpInfoTable = {
    {Op::ASSIGNMENT, {0, OpInfo::RIGHT}},
    {Op::ADD_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::MUL_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::SUB_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::DIV_ASSIGN, {0, OpInfo::RIGHT}},
    {Op::MOD_ASSIGN, {0, OpInfo::RIGHT}},


    {Op::LOGICAL_OR, {10, OpInfo::LEFT}},
    {Op::LOGICAL_AND, {20, OpInfo::LEFT}},
    {Op::LOGICAL_EQUAL, {30, OpInfo::LEFT}},
    {Op::LOGICAL_NOTEQUAL, {30, OpInfo::LEFT}},

    {Op::GREATER_THAN, {40, OpInfo::LEFT}},
    {Op::GREATER_THAN_OR_EQUAL, {40, OpInfo::LEFT}},
    {Op::LESS_THAN, {40, OpInfo::LEFT}},
    {Op::LESS_THAN_OR_EQUAL, {40, OpInfo::LEFT}},


    {Op::BINARY_ADD, {50, OpInfo::LEFT}},
    {Op::BINARY_SUB, {50, OpInfo::LEFT}},

    {Op::MUL, {60, OpInfo::LEFT}},
    {Op::DIV, {60, OpInfo::LEFT}},
    {Op::MOD, {60, OpInfo::LEFT}},

    {Op::CAST_OP, {100, OpInfo::LEFT}},

    {Op::ADDRESS_TAKING, {150, OpInfo::LEFT}},
    {Op::UNARY_ADD, {150, OpInfo::LEFT}},
    {Op::UNARY_SUB, {150, OpInfo::LEFT}},
    {Op::DEREFERENCE, {150, OpInfo::LEFT}},

    {Op::INDEXING_OP, {200, OpInfo::LEFT}},
    {Op::DOT, {800, OpInfo::LEFT}},
};


const static
std::unordered_map<std::pair<std::string_view, int>, Op::OpTag_t> OpTagMap = {
    {{"+", 2}, Op::BINARY_ADD},
    {{"-", 2}, Op::BINARY_SUB},

    {{"+", 1}, Op::UNARY_ADD},
    {{"-", 1}, Op::UNARY_SUB},

    {{"*", 2}, Op::MUL},
    {{"/", 2}, Op::DIV},
    {{"%", 2}, Op::MOD},

    {{"!", 1},  Op::LOGICAL_NOT},
    {{"==", 2}, Op::LOGICAL_EQUAL},
    {{"!=", 2}, Op::LOGICAL_NOTEQUAL},
    {{"||", 2}, Op::LOGICAL_OR},
    {{"&&", 2}, Op::LOGICAL_AND},

    {{">", 2},  Op::GREATER_THAN},
    {{">=", 2}, Op::GREATER_THAN_OR_EQUAL},
    {{"<", 2},  Op::LESS_THAN},
    {{"<=", 2}, Op::LESS_THAN_OR_EQUAL},

    {{"[", 2},  Op::INDEXING_OP},
    {{"[]", 2}, Op::INDEXING_OP},
    {{"*", 1},  Op::DEREFERENCE},
    {{"&", 1},  Op::ADDRESS_TAKING},
    {{"as", 2}, Op::CAST_OP},

    {{".", 2},  Op::DOT},
    {{"=", 2},  Op::ASSIGNMENT},
    {{"+=", 2}, Op::ADD_ASSIGN},
    {{"-=", 2}, Op::SUB_ASSIGN},
    {{"*=", 2}, Op::MUL_ASSIGN},
    {{"/=", 2}, Op::DIV_ASSIGN},
    {{"%=", 2}, Op::MOD_ASSIGN}
};


Op::Op(const std::string_view str, const int8_t adicity): value(std::string(str)), arity(adicity) {
    op_type = getTagFor(str, adicity);  // compute and set the tag of the operator node
}


/// Returns the Left-Binding Power of the operator with the tag `op`.
int Op::getLBPFor(const OpTag_t op) {
    return OpInfoTable.at(op).precedence;
}


/// Returns the Right-Binding Power of the operator with the tag `op`.
int Op::getRBPFor(const OpTag_t op) {
    const auto info = OpInfoTable.at(op);
    return info.associativity == OpInfo::RIGHT ? info.precedence - 1 : info.precedence;
}


/// Returns the Prefix-Binding Power of the operator with the tag `op`.
int Op::getPBPFor(const OpTag_t op) {
    const auto info = OpInfoTable.at(op);
    return info.precedence;
}


/// Returns a tag which uniquely identifies the operator, given its string-representation and arity.
Op::OpTag_t Op::getTagFor(const std::string_view str, int arity) {
    return OpTagMap.at({str, arity});
}


void Ident::ImplDeleter::operator()(const TypeWrapper* ptr) const {
    delete ptr;
}

std::string TypeWrapper::toString() const {
    assert(type);
    return type->toString();
}

Ident::Qualifier::~Qualifier() {
    for (const auto* type : generic_args) {
        delete type;
    }
}

void Op::replaceType(const std::string_view from, Type* to) {
    if (op_type == CAST_OP) {
        assert(operands.at(1)->getNodeType() == ND_TYPE);
        auto* operand = dynamic_cast<TypeWrapper*>(operands.at(1).get());
        operand->replaceType(from, to);
    }
}

void Ident::replaceType(const std::string_view from, Type* to) {
    for (const auto& arg : full_qualification) {
        for (auto* type : arg.generic_args) {
            type->replaceType(from, to);
        }
    }
}


std::unique_ptr<Node> Function::instantiate(Parser& instance, const std::span<Type*> args, ErrorCallback_t err_callback) {
    auto cloned = instance.cloneNode(ident);
    const auto fn_node = dynamic_cast<Function*>(cloned.get());

    assert(fn_node != nullptr);
    assert(generic_params.size() >= args.size());

    std::size_t i = 0;
    for (Type* arg : args) {
        fn_node->replaceType(generic_params.at(i).name, arg);
        i++;
    }

    fn_node->generic_params.clear();
    return cloned;
}


/// Constructs and returns an expression out of the `EvalResult` variant
Expression Expression::makeExpression(const EvalResult& e) {
    return std::visit(
    VisitorHelper{
        [](std::monostate) { return makeExpression(nullptr); },
        [](const bool v) { return makeExpression(new BoolLit{v}); },
        [](const double v) { return makeExpression(new FloatLit{v}); },
        [](const std::size_t v) { return makeExpression(new IntLit{v}); },
        [](const std::string& v) { return makeExpression(new StrLit{v}); },
    }, e);
}


EvalResult Node::evaluate(Parser& ctx) {
    ctx.reportError(ErrCode::NOT_ALLOWED_CT_CTX);
    return {};
}

EvalResult IntLit::evaluate(Parser&) {
    return toInteger(value);
}

EvalResult FloatLit::evaluate(Parser&) {
    return std::stod(value);
}

EvalResult BoolLit::evaluate(Parser&) {
    return value;
}

EvalResult StrLit::evaluate(Parser&) {
    return value;
}


EvalResult Ident::evaluate(Parser& ctx) {
    if (const auto node = ctx.SymbolTable.lookupDecl(getIdentInfo()).node_ptr) {
        if (node->getNodeType() == ND_VAR) {
            const auto var = dynamic_cast<Var*>(node);

            // only allow config variables
            if (!var->is_comptime) {
                ctx.reportError(ErrCode::NOT_A_CT_VAR);
                return {};
            } return var->value.evaluate(ctx);
        } ctx.reportError(ErrCode::NOT_ALLOWED_CT_CTX);
    } return {};
}


EvalResult Expression::evaluate(Parser& ctx) {
    return expr->evaluate(ctx);
}


EvalResult Op::evaluate(Parser& ctx) {
    switch (op_type) {
        case Op::MUL:
            return operands.at(0)->evaluate(ctx) * operands.at(1)->evaluate(ctx);
        case Op::DIV:
            return operands.at(0)->evaluate(ctx) / operands.at(1)->evaluate(ctx);
        case Op::BINARY_ADD:
            return operands.at(0)->evaluate(ctx) + operands.at(1)->evaluate(ctx);
        case Op::BINARY_SUB:
            return operands.at(0)->evaluate(ctx) - operands.at(1)->evaluate(ctx);
        case Op::GREATER_THAN:
            return operands.at(0)->evaluate(ctx) > operands.at(1)->evaluate(ctx);
        case Op::GREATER_THAN_OR_EQUAL:
            return operands.at(0)->evaluate(ctx) >= operands.at(1)->evaluate(ctx);
        case Op::LESS_THAN:
            return operands.at(0)->evaluate(ctx) < operands.at(1)->evaluate(ctx);
        case Op::LESS_THAN_OR_EQUAL:
            return operands.at(0)->evaluate(ctx) <= operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_OR:
            return operands.at(0)->evaluate(ctx) || operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_AND:
            return operands.at(0)->evaluate(ctx) && operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_EQUAL:
            return operands.at(0)->evaluate(ctx) == operands.at(1)->evaluate(ctx);
        case Op::LOGICAL_NOTEQUAL:
            return !(operands.at(0)->evaluate(ctx) == operands.at(1)->evaluate(ctx)).toBool();
        default:
            ctx.reportError(ErrCode::OP_NOT_ALLOWED_HERE, {.str_1 = value});
            return {};
    }
}
