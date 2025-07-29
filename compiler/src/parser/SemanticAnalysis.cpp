#include <cassert>
#include <utility>

#include <utils/utils.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <parser/SemanticAnalysis.h>
#include <managers/ModuleManager.h>


using SwNode = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;


Type* AnalysisContext::deduceType(Type* type1, Type* type2, StreamState location) const {
    // either of them being nullptr implies a violation of the typing-rules
    if (type1 == nullptr || type2 == nullptr) {
        return nullptr;
    }

    if (type1->getSwType() == Type::REFERENCE) type1 = dynamic_cast<ReferenceType*>(type1)->of_type;
    if (type2->getSwType() == Type::REFERENCE) type2 = dynamic_cast<ReferenceType*>(type2)->of_type;


    if (type1->isIntegral() && type2->isIntegral()) {
        // return the greater of the two types
        if ((type1->isUnsigned() && type2->isUnsigned()) || (!type1->isUnsigned() && !type2->isUnsigned())) {
            if (type1->getBitWidth() >= type2->getBitWidth()) {
                return type1;
            } return type2;
        }

        reportError(
            ErrCode::INCOMPATIBLE_TYPES,
            {.type_1 = type1, .type_2 = type2, .location = location}
            );

        return nullptr;
    }

    if (type1->isFloatingPoint() && type2->isFloatingPoint()) {
        if (type1->getBitWidth() >= type2->getBitWidth()) {
            return type1;
        } return type2;
    }

    if (type1->getSwType() == Type::ARRAY && type2->getSwType() == Type::ARRAY) {
        auto arr_1 = dynamic_cast<ArrayType*>(type1);
        auto arr_2 = dynamic_cast<ArrayType*>(type2);

        if (arr_1->size != arr_2->size) {
            reportError(
                ErrCode::INCOMPATIBLE_TYPES,
                {.type_1 = arr_1, .type_2 = arr_2, .location = location}
                );
            return nullptr;
        }

        return SymMan.getArrayType(deduceType(arr_1->of_type, arr_2->of_type, location), arr_1->size);
    }

    if (type1->getSwType() == Type::STRUCT || type2->getSwType() == Type::STRUCT) {
        if (checkTypeCompatibility(type1, type2, location)) {
            return type1;
        }
    }

    reportError(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = type1, .type_2 = type2, .location = location});
    return nullptr;
}


bool AnalysisContext::checkTypeCompatibility(Type* from, Type* to, StreamState location) const {
    if (!from || !to) return false;
    if (from->getSwType() == Type::REFERENCE) from = dynamic_cast<ReferenceType*>(from)->of_type;
    if (to->getSwType() == Type::REFERENCE) to = dynamic_cast<ReferenceType*>(to)->of_type;

    if ((from->isIntegral() && to->isFloatingPoint()) || (from->isFloatingPoint() && to->isIntegral())) {
        reportError(ErrCode::INT_AND_FLOAT_CONV, {.type_1 = from, .type_2 = to, .location = location});
        return false;
    }

    if (from->isIntegral() && to->isIntegral()) {
        if ((from->isUnsigned() && !to->isUnsigned()) || (!from->isUnsigned() && to->isUnsigned())) {
            reportError(ErrCode::NO_SIGNED_UNSIGNED_CONV, {.type_1 = from, .type_2 = to, .location = location});
            return false;
        }

        if (to->getBitWidth() < from->getBitWidth()) {
            reportError(ErrCode::NO_NARROWING_CONVERSION, {.type_1 = from, .type_2 = to, .location = location});
            return false;
        }
    }

    if (from->isFloatingPoint() && to->isFloatingPoint()) {
        if (to->getBitWidth() < from->getBitWidth()) {
            reportError(ErrCode::NO_NARROWING_CONVERSION, {.type_1 = from, .type_2 = to, .location = location});
            return false;
        }
    }

    if (from->getSwType() == Type::ARRAY && to->getSwType() == Type::ARRAY) {
        const auto* base_t1 = dynamic_cast<ArrayType*>(from);
        const auto* base_t2 = dynamic_cast<ArrayType*>(to);

        if (base_t1->size != base_t2->size) {
            reportError(ErrCode::DISTINCTLY_SIZED_ARR, {.type_1 = from, .type_2 = to, .location = location});
            return false;
        }

        return checkTypeCompatibility(base_t1->of_type, base_t2->of_type, location);
    }

    if (from->getSwType() == Type::STRUCT || to->getSwType() == Type::STRUCT) {
        if (from != to) {
            reportError(
                ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = from,
                    .type_2 = to,
                    .location = location
                }
            );
            return false;
        }
    } return true;
}


/// Assumes `of` is an "array-like" type, returns its underlying child type
Type* AnalysisContext::getUnderlyingType(Type* of) {
    throw;
}


AnalysisResult IntLit::analyzeSemantics(AnalysisContext& ctx) {
    if (ctx.getBoundTypeState() != nullptr && ctx.getBoundTypeState()->isIntegral()) {
        return {.deduced_type = ctx.getBoundTypeState()};
    } return {.deduced_type = &GlobalTypeI32};
}

AnalysisResult FloatLit::analyzeSemantics(AnalysisContext& ctx) {
    if (ctx.getBoundTypeState() != nullptr && ctx.getBoundTypeState()->isFloatingPoint()) {
        return {.deduced_type = ctx.getBoundTypeState()};
    } return {.deduced_type = &GlobalTypeF64};
}

AnalysisResult StrLit::analyzeSemantics(AnalysisContext& ctx) {
    return {.deduced_type = ctx.SymMan.getStrType(value.size())};
}

AnalysisResult ArrayNode::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    Type* common_type = nullptr;

    for (auto& element : elements) {
        auto expr_analysis = element.analyzeSemantics(ctx);

        if (common_type != nullptr) {
            common_type = ctx.deduceType(common_type, expr_analysis.deduced_type, location);
            continue;
        } common_type = expr_analysis.deduced_type;
    }

    ret.deduced_type = ctx.SymMan.getArrayType(common_type, elements.size());
    return ret;
}


AnalysisResult ImportNode::analyzeSemantics(AnalysisContext& ctx) {
    // specific-symbol imports
    if (!imported_symbols.empty()) {
        for (auto& symbol : imported_symbols) {
            IdentInfo* id = ctx.SymMan.getIdInfoFromModule(mod_path, symbol.actual_name);

            if (!id) {
                ctx.reportError(
                    ErrCode::SYMBOL_NOT_FOUND_IN_MOD,
                    {.path_1 = mod_path, .str_1 = symbol.actual_name, .location = location}
                    );
                continue;
            }

            if (!ctx.SymMan.lookupDecl(id).is_exported) {
                ctx.reportError(
                    ErrCode::SYMBOL_NOT_EXPORTED,
                    {.str_1 = symbol.actual_name, .location = location}
                    );
            }

            // make the symbol manager aware of the foreign symbol's `IdentInfo*`
            ctx.SymMan.registerForeignID(
                symbol.assigned_alias.empty() ? symbol.actual_name : symbol.assigned_alias, id, is_exported
                );

            ctx.GlobalNodeJmpTable.insert({id, ctx.ModuleMap.get(mod_path).NodeJmpTable.at(id)});
        } return {};
    }

    return {};
}


AnalysisResult Struct::analyzeSemantics(AnalysisContext& ctx) {
    if (ctx.Cache.contains(this)) {
        return ctx.Cache[this];
    }

    const auto ty = dynamic_cast<StructType*>(ctx.SymMan.lookupType(ident));

    for (auto& member : members) {
        member->analyzeSemantics(ctx);

        if (member->getNodeType() == ND_VAR) {
            ty->field_types.push_back(dynamic_cast<Var*>(member.get())->var_type);
        }
    }

    ctx.Cache[this] = {.deduced_type = ty};
    return {.deduced_type = ty};
}


AnalysisResult Var::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (!initialized && !var_type) {
        ctx.reportError(
            ErrCode::INITIALIZER_REQUIRED,
            {.ident = var_ident, .location = location}
            );
        return {};
    }

    if (var_type && var_type->getSwType() == Type::ARRAY) {
        const auto base_type = dynamic_cast<ArrayType*>(var_type)->of_type;
        ctx.setBoundTypeState(base_type);
    } else ctx.setBoundTypeState(var_type);

    if (initialized) {
        auto val_analysis = value.analyzeSemantics(ctx);
        ctx.restoreBoundTypeState();

        if (var_type == nullptr) {
            var_type = val_analysis.deduced_type;
            ctx.SymMan.lookupDecl(var_ident).swirl_type = var_type;
        } else {
            ctx.checkTypeCompatibility(val_analysis.deduced_type, var_type, location);
            value.setType(var_type);
        }
    } else ctx.restoreBoundTypeState();

    return ret;
}

AnalysisResult FuncCall::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (ctx.Cache.contains(this)) {
        return ctx.Cache[this];
    }

    ident.analyzeSemantics(ctx);

    IdentInfo* id = ident.getIdentInfo();

    if (!id) {
        ctx.Cache.insert({this, ret});
        return {};
    }

    if (ctx.getCurParentFunc()->getIdentInfo() != id)
        ctx.analyzeSemanticsOf(id);

    auto* fn_type = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(id));
    if (args.size() < fn_type->param_types.size()) {  // TODO: default params-values
        ctx.reportError(ErrCode::TOO_FEW_ARGS, {.ident = ident.getIdentInfo(), .location = location});
        ctx.Cache.insert({this, ret});
        return {};
    }

    if (fn_type->ret_type == nullptr && id == ctx.getCurParentFunc()->getIdentInfo()) {
        ctx.reportError(ErrCode::RET_TYPE_REQUIRED, {.location = location});
    }

    assert(fn_type->param_types.size() == args.size());


    for (std::size_t i = 0; i < args.size(); ++i) {
        ctx.setBoundTypeState(fn_type->param_types.at(i));
        AnalysisResult arg = args.at(i).analyzeSemantics(ctx);
        ctx.restoreBoundTypeState();

        ctx.checkTypeCompatibility(arg.deduced_type, fn_type->param_types.at(i), location);
        args[i].setType(fn_type->param_types[i]);
    }

    ret.deduced_type = fn_type->ret_type;

    ctx.Cache.insert({this, ret});
    return ret;
}

AnalysisResult Ident::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (ctx.Cache.contains(this)) {
        return ctx.Cache[this];
    }

    if (!value) {
        value = ctx.SymMan.getIDInfoFor(*this, [ctx](auto a, auto b) { ctx.reportError(a, std::move(b)); });
    }

    if (!value) {
        ctx.reportError(
            ErrCode::UNDEFINED_IDENTIFIER,
            {.str_1 = full_qualification.back(), .location = location}
            );
        ctx.Cache.insert({this, ret});
        return ret;
    }

    auto decl = ctx.SymMan.lookupDecl(this->value);
    ret.deduced_type = decl.swirl_type;

    ctx.Cache.insert({this, ret});
    return ret;
}

AnalysisResult WhileLoop::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    condition.analyzeSemantics(ctx);
    for (auto& child : children)
        child->analyzeSemantics(ctx);

    return ret;
}

AnalysisResult Condition::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    bool_expr.analyzeSemantics(ctx);

    for (auto& child : if_children) {
        child->analyzeSemantics(ctx);
    }

    for (auto& [cond, children] : elif_children) {
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


AnalysisResult ReturnStatement::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    ret.deduced_type = this->value.analyzeSemantics(ctx).deduced_type;
    return ret;
}

AnalysisResult Function::analyzeSemantics(AnalysisContext& ctx) {
    if (ctx.Cache.contains(this))
        return ctx.Cache[this];

    ctx.setCurParentFunc(this);

    AnalysisResult ret;
    Type* deduced_type = nullptr;

    for (auto& param : params) {
        param.analyzeSemantics(ctx);
    }

    auto* fn_type = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(this->ident));
    ctx.setBoundTypeState(fn_type->ret_type);

    for (auto& child : children) {
        if (child->getNodeType() == ND_RET) {
            auto ret_analysis = child->analyzeSemantics(ctx);

            if (fn_type->ret_type != nullptr) {
                // TODO: check for implicit convertibility
                auto* ret_t = dynamic_cast<ReturnStatement*>(child.get());
                ret_t->value.setType(fn_type->ret_type);
                continue;
            }

            if (deduced_type != nullptr) {
                deduced_type = ctx.deduceType(deduced_type, ret_analysis.deduced_type, location);
                continue;
            }

            deduced_type = ret_analysis.deduced_type;
            continue;
        }
        child->analyzeSemantics(ctx);
    }

    ctx.restoreBoundTypeState();
    if (fn_type->ret_type == nullptr)
        fn_type->ret_type = deduced_type;

    ctx.restoreCurParentFunc();
    ctx.Cache.insert({this, ret});
    return ret;
}


AnalysisResult Op::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    // 1st operand
    auto analysis_1 = operands.at(0)->analyzeSemantics(ctx);

    is_mutable = !ctx.getBoundTypeState() ? is_mutable : ctx.getBoundTypeState()->is_mutable;

    if (arity == 1) {
        if (value == "&") {
            // take a reference
            ret.deduced_type = ctx.SymMan.getReferenceType(analysis_1.deduced_type, is_mutable);
        }

        else {
            ret.deduced_type = analysis_1.deduced_type;
        }

    } else {
        // 2nd operand
        auto analysis_2 = value != "as" && op_type != DOT && op_type != ASSIGNMENT ? operands.at(1)->analyzeSemantics(ctx) : AnalysisResult{};

        switch (op_type) {
            case DIV: {
                if (analysis_1.deduced_type->isIntegral() && analysis_2.deduced_type->isIntegral()) {
                    ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
                    break;
                } if (analysis_1.deduced_type->isFloatingPoint() && analysis_2.deduced_type->isFloatingPoint()) {
                    ret.deduced_type = &GlobalTypeF64;
                    break;
                } ctx.reportError(ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = analysis_1.deduced_type,
                    .type_2 = analysis_2.deduced_type,
                    .location = location
                });
                break;
            }

            case MOD: {
                if (analysis_1.deduced_type->isIntegral() && analysis_2.deduced_type->isIntegral()) {
                    ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
                    break;
                }
                
                ctx.reportError(ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = analysis_1.deduced_type,
                    .type_2 = analysis_2.deduced_type,
                    .location = location
                });
                break;
            }

            case ASSIGNMENT: {
                ret.deduced_type = &GlobalTypeVoid;

                if ((!analysis_1.deduced_type->is_mutable &&
                    (analysis_1.deduced_type->getSwType() == Type::REFERENCE) ||
                    (analysis_1.deduced_type->getSwType() == Type::POINTER)
                ) ||
                    (operands.at(0)->getNodeType() == ND_IDENT &&
                        ctx.SymMan.lookupDecl(operands.at(0)->getIdentInfo()).is_const
                        )) {
                    ctx.reportError(ErrCode::CANNOT_ASSIGN_TO_CONST, {.location = location});
                    break;
                }

                ctx.setBoundTypeState(analysis_1.deduced_type);
                analysis_2 = operands.at(1)->analyzeSemantics(ctx);
                ctx.restoreBoundTypeState();
                ctx.checkTypeCompatibility(analysis_2.deduced_type, analysis_1.deduced_type, location);
                ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
                break;
            }

            case INDEXING_OP: {
                if (!analysis_2.deduced_type->isIntegral()) {
                    // ctx.ErrMan.newError("Non-integral values can't be used as indices.", location);
                } ret.deduced_type = dynamic_cast<ArrayType*>(analysis_1.deduced_type)->of_type;
                break;
            }

            case CAST_OP: {
                ret.deduced_type = ctx.SymMan.lookupType(operands.at(1)->getIdentInfo());
                break;
            }

            case DOT: {
                AnalysisContext::DisableErrorCode _(ErrCode::UNDEFINED_IDENTIFIER, ctx);

                auto accessed_operand = dynamic_cast<Ident*>(operands.at(1).get());
                if (!accessed_operand) throw std::runtime_error("accessed operand not an id!");

                auto accessed_op_id =
                    analysis_1.deduced_type->scope->getIDInfoFor(
                        accessed_operand->full_qualification.at(0)
                        );

                if (!accessed_op_id) {
                    ctx.reportError(ErrCode::NO_SUCH_MEMBER, {
                        .str_1 = accessed_operand->full_qualification.front(),
                        .str_2 = operands.at(0)->getIdentInfo()->toString(),
                        .location = location
                    }); ret.is_erroneous = true; break;
                }

                accessed_operand->value = *accessed_op_id;
                const auto access_entry = ctx.SymMan.lookupDecl(*accessed_op_id);
                ret.deduced_type = access_entry.swirl_type;
                break;
            }

            default: {
                ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
            }
        }
    }

    inferred_type = ret.deduced_type;
    return ret;
}

AnalysisResult Expression::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (ctx.Cache.contains(this)) {
        return ret;
    }

    assert(this->expr.size() == 1);
    auto val = this->expr.front()->analyzeSemantics(ctx);

    ret.deduced_type = val.deduced_type;
    setType(val.deduced_type);

    ctx.Cache.insert({this, ret});
    return ret;
}


AnalysisResult Assignment::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (l_value.getNodeType() == ND_IDENT) {
        if (ctx.SymMan.lookupDecl(l_value.getIdentInfo()).is_const) {
            // ctx.ErrMan.newError("Cannot assign, " + l_value.getIdentInfo()->toString() + " is const.");
        }
    }

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


void AnalysisContext::analyzeSemanticsOf(IdentInfo* id) {
    if (!GlobalNodeJmpTable.contains(id)) {
        // ModuleMap.get(id->getModulePath()).NodeJmpTable.at(id)->analyzeSemantics(*this);
        return;
    } GlobalNodeJmpTable[id]->analyzeSemantics(*this);
}