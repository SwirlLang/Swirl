#include <cassert>
#include <utility>

#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <parser/SemanticAnalysis.h>
#include <managers/ModuleManager.h>


using SwNode = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;

#define PRE_SETUP() AnalysisContext::SemaSetupHandler GET_UNIQUE_NAME(presetup){ctx, this};


Type* AnalysisContext::deduceType(Type* type1, Type* type2, const SourceLocation& location) const {
    // either of them being nullptr implies a violation of the typing-rules
    if (type1 == nullptr || type2 == nullptr) {
        return nullptr;
    }

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

    if (type1->getTypeTag() == Type::ARRAY && type2->getTypeTag() == Type::ARRAY) {
        auto arr_1 = dynamic_cast<ArrayType*>(type1);
        auto arr_2 = dynamic_cast<ArrayType*>(type2);

        if (arr_1->size != arr_2->size) {
            reportError(
                ErrCode::DISTINCTLY_SIZED_ARR,
                {.type_1 = arr_1, .type_2 = arr_2, .location = location}
                );
            return nullptr;
        }

        return SymMan.getArrayType(deduceType(arr_1->of_type, arr_2->of_type, location), arr_1->size);
    }

    if (type1->getTypeTag() == Type::STRUCT || type2->getTypeTag() == Type::STRUCT) {
        if (checkTypeCompatibility(type1, type2, location)) {
            return type1;
        }
    }

    if (type1->getTypeTag() == Type::REFERENCE || type2->getTypeTag() == Type::REFERENCE) {
        const auto enclosed_type_1 = type1->getTypeTag() == Type::REFERENCE ?
            dynamic_cast<ReferenceType*>(type1)->of_type : type1;

        const auto enclosed_type_2 = type2->getTypeTag() == Type::REFERENCE ?
            dynamic_cast<ReferenceType*>(type2)->of_type : type2;

        return deduceType(enclosed_type_1, enclosed_type_2, location);
    }

    reportError(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = type1, .type_2 = type2, .location = location});
    return nullptr;
}


bool AnalysisContext::checkTypeCompatibility(Type* from, Type* to, const SourceLocation& location) const {
    if (!from || !to) return false;

    if (from->getTypeTag() == Type::REFERENCE && to->getTypeTag() == Type::REFERENCE) {
        if (to->is_mutable && !from->is_mutable) {
            reportError(ErrCode::IMMUTABILITY_VIOLATION, {
                .type_1 = from,
                .type_2 = to,
                .location = location
            });
            return false;
        }
    }

    if (from->getTypeTag() == Type::REFERENCE && to->getTypeTag() != Type::REFERENCE) {
        return checkTypeCompatibility(
            dynamic_cast<ReferenceType*>(from)->of_type,
            to,
            location
        );
    }

    if (from->getTypeTag() != Type::REFERENCE && to->getTypeTag() == Type::REFERENCE) {
        return checkTypeCompatibility(
            from,
            dynamic_cast<ReferenceType*>(to)->of_type,
            location
        );
    }

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

    if (from->getTypeTag() == Type::ARRAY && to->getTypeTag() == Type::ARRAY) {
        const auto* base_t1 = dynamic_cast<ArrayType*>(from);
        const auto* base_t2 = dynamic_cast<ArrayType*>(to);

        if (base_t1->size != base_t2->size) {
            reportError(ErrCode::DISTINCTLY_SIZED_ARR, {.type_1 = from, .type_2 = to, .location = location});
            return false;
        }

        return checkTypeCompatibility(base_t1->of_type, base_t2->of_type, location);
    }

    if (from->getTypeTag() == Type::REFERENCE && to->getTypeTag() == Type::REFERENCE) {
        const bool result = checkTypeCompatibility(
            dynamic_cast<ReferenceType*>(from)->of_type,
            dynamic_cast<ReferenceType*>(to)->of_type,
            location
            );

        if (!result) {
            reportError(
                ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = from,
                    .type_2 = to,
                    .location = location
                }
            );
        }
    }

    if (from->getTypeTag() == Type::STRUCT || to->getTypeTag() == Type::STRUCT) {
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

#undef SW_EXPAND_WITHIN_SEMA_METHOD
AnalysisResult IntLit::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    if (ctx.getBoundTypeState() != nullptr && ctx.getBoundTypeState()->isIntegral()) {
        return {.deduced_type = ctx.getBoundTypeState()};
    } return {.deduced_type = &GlobalTypeI32};
}

AnalysisResult FloatLit::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    if (ctx.getBoundTypeState() != nullptr && ctx.getBoundTypeState()->isFloatingPoint()) {
        return {.deduced_type = ctx.getBoundTypeState()};
    } return {.deduced_type = &GlobalTypeF64};
}

AnalysisResult BoolLit::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    return {.deduced_type = &GlobalTypeBool};
}


AnalysisResult StrLit::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    return {.deduced_type = ctx.SymMan.getStrType(value.size())};
}

AnalysisResult ArrayLit::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
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
    PRE_SETUP();
    // specific-symbol imports
    if (!imported_symbols.empty()) {
        for (auto& symbol : imported_symbols) {
            IdentInfo* id = ctx.SymMan.getIdInfoFromModule(mod_path, symbol.actual_name);

            if (!id) {
                ctx.reportError(
                    ErrCode::SYMBOL_NOT_FOUND_IN_MOD,
                    {.path_1 = mod_path, .str_1 = symbol.actual_name}
                    );
                continue;
            }

            if (!ctx.SymMan.lookupDecl(id).is_exported) {
                ctx.reportError(
                    ErrCode::SYMBOL_NOT_EXPORTED,
                    {.str_1 = symbol.actual_name}
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


AnalysisResult Scope::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    for (const auto& node : children) {
        node->analyzeSemantics(ctx);
    } return {};
}


AnalysisResult Struct::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
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
    PRE_SETUP();
    AnalysisResult ret;

    if (is_config && !initialized) {
        ctx.reportError(ErrCode::CONFIG_VAR_UNINITIALIZED, {});
        return {};
    }

    if (!initialized && !var_type) {
        ctx.reportError(
            ErrCode::INITIALIZER_REQUIRED,
            {.ident = var_ident}
            );
        return {};
    }

    if (var_type && var_type->getTypeTag() == Type::ARRAY) {
        const auto base_type = dynamic_cast<ArrayType*>(var_type)->of_type;
        ctx.setBoundTypeState(base_type);
    } else ctx.setBoundTypeState(var_type);

    if (initialized) {
        if (is_config && !value.expr->getExprValue()->isLiteral()) {
            ctx.reportError(ErrCode::CONFIG_INIT_NOT_LITERAL, {});
            return {};
        }

        auto val_analysis = value.analyzeSemantics(ctx);
        ctx.restoreBoundTypeState();

        if (var_type == nullptr) {
            var_type = val_analysis.deduced_type;
            ctx.SymMan.lookupDecl(var_ident).swirl_type = var_type;
            value.setType(var_type);
        } else {
            ctx.checkTypeCompatibility(val_analysis.deduced_type, var_type, location);
            value.setType(var_type);
        }
    } else ctx.restoreBoundTypeState();

    ret.deduced_type = var_type;
    return ret;
}

AnalysisResult FuncCall::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    AnalysisResult ret;

    if (ctx.Cache.contains(this)) {
        return ctx.Cache[this];
    }

    // method calls' id are not simple identifiers, do not analyze them normally
    if (!ctx.getIsMethodCallState()) {
        ident.analyzeSemantics(ctx);
    }

    IdentInfo* id = ident.getIdentInfo();

    if (!id) {
        ctx.Cache.insert({this, ret});
        return {};
    }

    if (ctx.getCurParentFunc()->getIdentInfo() != id)
        ctx.analyzeSemanticsOf(id);

    auto* fn_type = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(id));

    if (args.size() < fn_type->param_types.size() && !ctx.getIsMethodCallState()) {  // TODO: default params-values
        ctx.reportError(ErrCode::TOO_FEW_ARGS, {.ident = ident.getIdentInfo()});
        ctx.Cache.insert({this, ret});
        return {};
    }

    if (fn_type->ret_type == nullptr && id == ctx.getCurParentFunc()->getIdentInfo()) {
        ctx.reportError(ErrCode::RET_TYPE_REQUIRED, {});
    }


    if (!ctx.getIsMethodCallState()) {
        assert(fn_type->param_types.size() == args.size());
    } else assert(fn_type->param_types.size() - 1 == args.size());

    // the bias is added while indexing into the `param_types` vector (of `Type*`) to account for
    // the instance-reference Type in methods
    const auto bias = ctx.getIsMethodCallState() ? 1 : 0;

    for (std::size_t i = 0; i < args.size(); ++i) {
        ctx.setBoundTypeState(fn_type->param_types.at(i + bias));
        AnalysisResult arg = args.at(i).analyzeSemantics(ctx);
        ctx.restoreBoundTypeState();

        ctx.checkTypeCompatibility(arg.deduced_type, fn_type->param_types.at(i + bias), location);
        args[i].setType(fn_type->param_types[i + bias]);
    }

    ret.deduced_type = fn_type->ret_type;

    ctx.Cache.insert({this, ret});
    return ret;
}

AnalysisResult Ident::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
    AnalysisResult ret;

    if (ctx.Cache.contains(this)) {
        return ctx.Cache[this];
    }

    if (!value) {
        value = ctx.SymMan.getIDInfoFor(
            *this,
            [ctx](auto a, auto b) {
                ctx.reportError(a, std::move(b));
            });
    }

    if (!value) {
        ctx.reportError(
            ErrCode::UNDEFINED_IDENTIFIER,
            {.str_1 = full_qualification.back()}
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
    PRE_SETUP();
    AnalysisResult ret;

    condition.analyzeSemantics(ctx);
    for (auto& child : children)
        child->analyzeSemantics(ctx);

    return ret;
}

AnalysisResult Condition::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
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
    PRE_SETUP();
    AnalysisResult ret;
    ret.deduced_type = this->value.analyzeSemantics(ctx).deduced_type;
    return ret;
}

AnalysisResult Function::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
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
    PRE_SETUP();
    AnalysisResult ret;
    // 1st operand
    auto analysis_1 = operands.at(0)->analyzeSemantics(ctx);

    if (analysis_1.deduced_type == nullptr)
        return {};

    if (arity == 1) {
        if (value == "&") {
            // take a reference
            if (ctx.getBoundTypeState() && ctx.getBoundTypeState()->getTypeTag() == Type::REFERENCE) {
                is_mutable = ctx.getBoundTypeState()->is_mutable;
            }

            if (analysis_1.deduced_type->getTypeTag() == Type::REFERENCE) {
                if (!analysis_1.deduced_type->is_mutable && is_mutable) {
                    ctx.reportError(ErrCode::IMMUTABILITY_VIOLATION, {});
                }
            } ret.deduced_type = ctx.SymMan.getReferenceType(analysis_1.deduced_type, is_mutable);
        }

        else {
            ret.deduced_type = analysis_1.deduced_type;
        }

    } else {
        // 2nd operand
        auto analysis_2 = value != "as" && op_type != DOT && op_type != ASSIGNMENT ? operands.at(1)->analyzeSemantics(ctx) : AnalysisResult{};

        switch (op_type) {
            case DIV: {
                if (analysis_2.deduced_type == nullptr)
                    return {};

                if (analysis_1.deduced_type->isIntegral() && analysis_2.deduced_type->isIntegral()) {
                    ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
                    break;
                } if (analysis_1.deduced_type->isFloatingPoint() && analysis_2.deduced_type->isFloatingPoint()) {
                    ret.deduced_type = &GlobalTypeF64;
                    break;
                } ctx.reportError(ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = analysis_1.deduced_type,
                    .type_2 = analysis_2.deduced_type,
                });
                break;
            }

            case MOD: {
                if (analysis_2.deduced_type == nullptr)
                    return {};

                if (analysis_1.deduced_type->isIntegral() && analysis_2.deduced_type->isIntegral()) {
                    ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
                    break;
                }
                
                ctx.reportError(ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = analysis_1.deduced_type,
                    .type_2 = analysis_2.deduced_type,
                });
                break;
            }

            case ASSIGNMENT: {
                ret.deduced_type = &GlobalTypeVoid;

                if (((!analysis_1.deduced_type->is_mutable &&
                    (analysis_1.deduced_type->getTypeTag() == Type::REFERENCE)) ||
                    (analysis_1.deduced_type->getTypeTag() == Type::POINTER)
                ) ||
                    (operands.at(0)->getNodeType() == ND_IDENT &&
                        ctx.SymMan.lookupDecl(operands.at(0)->getIdentInfo()).is_const
                        )) {
                    ctx.reportError(ErrCode::CANNOT_ASSIGN_TO_CONST, {});
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
                if (analysis_2.deduced_type && !analysis_2.deduced_type->isIntegral()) {
                    ctx.reportError(
                        ErrCode::NON_INTEGRAL_INDICES, {
                            .type_1 = analysis_2.deduced_type,
                        });
                    break;
                } ret.deduced_type = analysis_1.deduced_type->getWrappedType();
                break;
            }

            case CAST_OP: {
                ret.deduced_type = operands.at(1)->getSwType();
                break;
            }

            case DOT: {
                AnalysisContext::DisableErrorCode _(ErrCode::UNDEFINED_IDENTIFIER, ctx);

                std::string id_str;
                Ident*      rhs_id = nullptr;
                bool        is_method = false;

                if (operands.at(1)->getNodeType() == ND_CALL) {
                    is_method = true;
                    rhs_id = &dynamic_cast<FuncCall*>(operands.at(1).get())->ident;
                    id_str = rhs_id->full_qualification.at(0);
                } else {
                    assert(operands.at(1)->getNodeType() == ND_IDENT);
                    rhs_id = dynamic_cast<Ident*>(operands.at(1).get());
                    id_str = rhs_id->full_qualification.at(0);
                }

                // automatic dereference
                auto struct_ty = analysis_1.deduced_type;
                if (struct_ty->getTypeTag() == Type::REFERENCE)
                    struct_ty = dynamic_cast<ReferenceType*>(struct_ty)->of_type;

                auto accessed_op_id = struct_ty->scope->getIDInfoFor(id_str);

                if (!accessed_op_id) {
                    ctx.reportError(ErrCode::NO_SUCH_MEMBER,{
                        .str_1 = id_str,
                        .str_2 = struct_ty->toString(),
                    }); ret.is_erroneous = true;
                }

                rhs_id->value = *accessed_op_id;

                Type* field_access_ty = nullptr;
                if (!is_method) {
                    field_access_ty = ctx.SymMan.lookupDecl(*accessed_op_id).swirl_type;
                    assert(field_access_ty);
                } else {  // is a method
                    ctx.setIsMethodCallState(true);
                    operands.at(1)->analyzeSemantics(ctx);
                    ctx.restoreIsMethodCallState();
                    auto call_node_id = operands.at(1)->getIdentInfo();
                    assert(call_node_id);
                    field_access_ty = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(call_node_id))->ret_type;
                }

                ret.deduced_type = field_access_ty;
                assert(field_access_ty);
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
    PRE_SETUP();
    AnalysisResult ret;

    if (ctx.Cache.contains(this)) {
        return ret;
    }

    auto val = this->expr->analyzeSemantics(ctx);

    ret.deduced_type = val.deduced_type;
    setType(val.deduced_type);

    ctx.Cache.insert({this, ret});
    return ret;
}


AnalysisResult Assignment::analyzeSemantics(AnalysisContext& ctx) {
    PRE_SETUP();
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
    if (expr->getNodeType() == ND_EXPR) {
        dynamic_cast<Expression*>(expr.get())->setType(to);
    }
    else if (expr->getNodeType() == ND_OP) {
        dynamic_cast<Op*>(expr.get())->setType(to);
    }
}


void AnalysisContext::analyzeSemanticsOf(IdentInfo* id) {
    if (!GlobalNodeJmpTable.contains(id)) { return; }
    GlobalNodeJmpTable[id]->analyzeSemantics(*this);
}