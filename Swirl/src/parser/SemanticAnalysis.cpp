#include <cassert>
#include <types/SwTypes.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <parser/SemanticAnalysis.h>


using SwNode = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;


// TODO: a mechanism for caching analyses to not analyze redundantly


// put the larger type of the two into `placed_into` 
Type* deduceType(Type* type1, Type* type2) {
    // TODO: signed types
    if (type1 == nullptr)
        return type2;
    else {
        if (type1->isFloatingPoint() && type2->getBitWidth() > type1->getBitWidth()) {
            return type2;
        }
        if (type1->isIntegral() && type2->getBitWidth() > type1->getBitWidth()) {
            return type2;
        }
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
    var_type = val_analysis.deduced_type;

    return ret;
}

AnalysisResult FuncCall::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    return ret;
}

AnalysisResult Ident::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    auto decl = ctx.SymMan.lookupDecl(this->value);
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
    
    for (auto& child : if_children) {
        child->analyzeSemantics(ctx);
    }

    for (auto& [cond, children] : elif_children) {
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
        if (child->getNodeType() == ND_RET) {  // TODO: handle multiple return statements
            auto ret_analysis = child->analyzeSemantics(ctx);
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
        auto analysis_2 = operands.at(1)->analyzeSemantics(ctx);

        if (this->value == "/") {
            ret.deduced_type = &GlobalTypeF64;
        } 
        else {
            ret.deduced_type = deduceType(analysis_1.deduced_type, analysis_2.deduced_type);
        }   
    }
    
    return ret;
}

AnalysisResult Expression::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    assert(this->expr.size() == 1);
    auto val = this->expr.front()->analyzeSemantics(ctx);

    ret.deduced_type = val.deduced_type;
    this->expr_type = val.deduced_type;
    return ret;
}


AnalysisResult Assignment::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    return ret;
}

