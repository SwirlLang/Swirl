#include <cassert>
#include <print>
#include <types/SwTypes.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <parser/SemanticAnalysis.h>
#include <unordered_set>


using SwNode = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;


Type* deduceType(Type* type1, Type* type2) {
    // TODO: signed types
    if (type1 == nullptr)
        return type2;
    if (type1->isFloatingPoint() && type2->getBitWidth() > type1->getBitWidth()) {
        return type2;
    }
    if (type1->isIntegral() && type2->getBitWidth() > type1->getBitWidth()) {
        return type2;
    }
    return type1;
}


AnalysisResult IntLit::analyzeSemantics(AnalysisContext&) {
    return {.deduced_type = &GlobalTypeI32};
}

AnalysisResult FloatLit::analyzeSemantics(AnalysisContext&) {
    return {.deduced_type = &GlobalTypeF64};
}

AnalysisResult StrLit::analyzeSemantics(AnalysisContext&) {
    return {}; 
}


AnalysisResult Var::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    auto val_analysis = value.analyzeSemantics(ctx);

    if (var_type == nullptr) {
        var_type = val_analysis.deduced_type;
        ctx.SymMan.lookupDecl(var_ident).swirl_type = var_type;
    } else {
        // TODO: check whether deduced_type is implicitly convertible to var_type 
        value.setType(var_type);
    }

    return ret;
}

AnalysisResult FuncCall::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    auto* fn_type = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(ident));
    assert(fn_type->param_types.size() == args.size());

    for (std::size_t i = 0; i < args.size(); ++i) {
        // TODO check whether the types are compatible
        args[i].setType(fn_type->param_types[i]);
    }

    return ret;
}

AnalysisResult Ident::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    auto decl = ctx.SymMan.lookupDecl(this->value);
    ret.deduced_type = decl.swirl_type;
    return ret;
}

AnalysisResult WhileLoop::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    for (auto& child : children)
        child->analyzeSemantics(ctx);
        
    return ret;
}

AnalysisResult Condition::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    // bool_expr.setType(&GlobalTypeBool);
    bool_expr.analyzeSemantics(ctx);

    for (auto& child : if_children) {
        child->analyzeSemantics(ctx);
    }

    for (auto& [cond, children] : elif_children) {
        // cond.setType(&GlobalTypeBool);
        cond.analyzeSemantics(ctx);
        for (auto& child : children) {
            child->analyzeSemantics(ctx);
        }
    }
    
    for (auto& child : else_children) {
        child->analyzeSemantics(ctx);
    }

    return ret;
}

AnalysisResult Struct::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    return ret;
}

AnalysisResult ReturnStatement::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    ret.deduced_type = this->value.analyzeSemantics(ctx).deduced_type;
    return ret;
}

AnalysisResult Function::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    Type* deduced_type = nullptr;
    
    for (auto& child : children) {
        if (child->getNodeType() == ND_RET) { 
            auto ret_analysis = child->analyzeSemantics(ctx);

            if (deduced_type != nullptr) {
                deduced_type = deduceType(deduced_type, ret_analysis.deduced_type);
                continue;
            }

            deduced_type = ret_analysis.deduced_type;
            continue;
        }
        child->analyzeSemantics(ctx);
    }

    FunctionType* fn_type = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(this->ident));
    fn_type->ret_type = deduced_type;

    return ret;
}


AnalysisResult Op::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    // 1st operand
    auto analysis_1 = operands.at(0)->analyzeSemantics(ctx);

    if (arity == 1) {
        ret.deduced_type = analysis_1.deduced_type;

    } else {
        // 2nd operand
        auto analysis_2 = value != "as" ? operands.at(1)->analyzeSemantics(ctx) : AnalysisResult{};

        if (value == "/") {
            ret.deduced_type = &GlobalTypeF64;
        }

        else if (value == "as") {

            ret.deduced_type = ctx.SymMan.lookupType(operands.at(1)->getIdentInfo());
        }

        else {
            ret.deduced_type = deduceType(analysis_1.deduced_type, analysis_2.deduced_type);
        }   
    }
    
    return ret;
}

AnalysisResult Expression::analyzeSemantics(AnalysisContext& ctx) {
    if (ctx.Cache.contains(this))
        return ctx.Cache.at(this);

    AnalysisResult ret;

    assert(this->expr.size() == 1);
    auto val = this->expr.front()->analyzeSemantics(ctx);

    ret.deduced_type = val.deduced_type;
    setType(val.deduced_type);

    ctx.Cache.insert({this, ret});
    return ret;
}


AnalysisResult Assignment::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    return ret;
}

void Op::setType(Type* to) {
    if (operands.front()->getNodeType() == ND_EXPR) {
        dynamic_cast<Expression*>(operands.front().get())->setType(to);
    }
    
    if (arity == 1) return;
    if (operands.back()->getNodeType() == ND_EXPR) {
        auto expr = dynamic_cast<Expression*>(operands.back().get());
        expr->setType(to);
        return;
    }
}

void Expression::setType(Type* to) {
    expr_type = to;
    if (expr.front()->getNodeType() == ND_EXPR) {
        dynamic_cast<Expression*>(expr.front().get())->setType(to);
    }
    else if (expr.front()->getNodeType() == ND_OP) {
        dynamic_cast<Op*>(expr.front().get())->setType(to);
    }
}
