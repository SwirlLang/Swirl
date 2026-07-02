#include "sema/TypeResolver.h"

#include "sema/SymbolResolver.h"
#include "errors/ErrorManager.h"
#include "parser/Parser.h"


bool sema::TypeResolver::checkTypeCompatibility(Type* from, Type* to, bool report_errors) {
    if (!from || !to) return false;
    if (from == to)   return true;

    auto report_error = [this, report_errors](const ErrCode code, ErrorContext ctx) {
        if (report_errors) {
            reportError(code, std::move(ctx));
        }
    };

    if (from->isIntegral() && to->isFloatingPoint() || to->isFloatingPoint() && from->isIntegral()) {
        report_error(ErrCode::INT_AND_FLOAT_CONV, {.type_1 = from, .type_2 = to});
        return false;
    }


    if (from->isIntegral() && to->isIntegral() || from->isFloatingPoint() && to->isFloatingPoint()) {
        if (from->getBitWidth() > to->getBitWidth()) {
            report_error(ErrCode::NO_NARROWING_CONVERSION, {.type_1 = from, .type_2 = to});
            return false;
        } return true;
    }


    if (from->isStructType() && to->isStructType()) {
        if (from != to) {
            report_error(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = from, .type_2 = to});
            return false;
        } return true;
    }


    if (from->isReferenceLikeType() && to->isReferenceLikeType()) {
        if (from->getWrappedType() != to->getWrappedType()) {
            report_error(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = from, .type_2 = to});
            return false;
        }

        if (to->is_mutable && !from->is_mutable) {
            report_error(ErrCode::IMMUTABILITY_VIOLATION, {.type_1 = from, .type_2 = to});
            return false;
        }

        return true;
    }


    if (from->isArrayType() && to->isArrayType()) {
        if (from->getAggregateSize() != from->getAggregateSize()) {
            report_error(ErrCode::DISTINCTLY_SIZED_ARR, {.type_1 = from, .type_2 = to});
            return false;
        }

        if (!checkTypeCompatibility(from->getWrappedType(), to->getWrappedType(), false)) {
            report_error(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = from, .type_2 = to});
            return false;
        } return true;
    }

    // relax the rules for enums when monomorphizing, this is a clutch since enums are substituted
    // with integers after compile time evaluation, which precedes monomorphization, triggering the type checker
    // for implicit conversion between enums and integral types is prohibited
    if (IsMonomorphization && (from->isEnumType() || to->isEnumType())) {
        return true;
    }

    if (from->isEnumType() && to->isEnumType() && from == to) {
        return true;
    }

    if (from->getTypeTag() == Type::GENERIC || to->getTypeTag() == Type::GENERIC) {
        return true;
    }

    report_error(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = from, .type_2 = to});
    return false;
}


Type* sema::TypeResolver::unify(Type* type1, Type* type2) {
    // either of them being nullptr implies a semantic violation, it is assumed that this was already reported
    // hence the method simply returns
    if (!type1 || !type2) {
        return nullptr;
    }

    if (type1 == type2) {
        return type1;
    }

    // return the greater of the two types, ensuring that both integral types are either all signed or unsigned
    if (type1->isIntegral() && type2->isIntegral()) {
        if (type1->isUnsigned() && !type2->isUnsigned() || !type1->isUnsigned() && type2->isUnsigned()) {
            reportError(ErrCode::NO_SIGNED_UNSIGNED_CONV, {.type_1 = type1, .type_2 = type2});
            return nullptr;
        }

        if (type1->getBitWidth() >= type2->getBitWidth()) {
            return type1;
        } return type2;
    }


    // return the greater of the two types
    if (type1->isFloatingPoint() && type2->isFloatingPoint()) {
        if (type1->getBitWidth() >= type2->getBitWidth()) {
            return type1;
        } return type2;
    }

    if (type1->isArrayType() && type2->isArrayType()) {
        if (type1->getAggregateSize() != type2->getAggregateSize()) {
            reportError(
                ErrCode::DISTINCTLY_SIZED_ARR,
                {.type_1 = type1, .type_2 = type2}
                );
            return nullptr;
        }

        return SymMan.getArrayType(
            unify(type1->getWrappedType(), type2->getWrappedType()), type1->getAggregateSize());
    }

    if (type1->getTypeTag() == Type::GENERIC || type2->getTypeTag() == Type::GENERIC) {
        return &GlobalTypeGeneric;
    }

    if (type1->isEnumType() && type2->isEnumType() && type1 == type2) {
        return type1;
    }

    reportError(ErrCode::INCOMPATIBLE_TYPES, {.type_1 = type1, .type_2 = type2});
    return nullptr;
}


sema::TypeResolver::TypeInfo sema::TypeResolver::evaluateType(Op* node, const TypeContext& ctx) {
    TypeInfo ret{};

    // 1st operand
    auto analysis_1 = inferType(node->operands.at(0), ctx);
    TypeInfo analysis_2{};

    if (analysis_1.deduced_type == nullptr)
        return {};

    if (node->arity == 1) {
        switch (node->op_type) {
            case Op::ADDRESS_TAKING: {
                // take a reference
                if (ctx.bound_type && ctx.bound_type->getTypeTag() == Type::REFERENCE) {
                    node->is_mutable = ctx.bound_type->is_mutable;
                }

                if (analysis_1.deduced_type->getTypeTag() == Type::REFERENCE) {
                    if (!analysis_1.deduced_type->is_mutable && node->is_mutable) {
                        reportError(ErrCode::IMMUTABILITY_VIOLATION, {});
                    }
                } ret.deduced_type = SymMan.getReferenceType(analysis_1.deduced_type, node->is_mutable);
                break;
            }

            case Op::DEREFERENCE:
                if (!analysis_1.deduced_type->isPointerType()) {
                    reportError(ErrCode::NOT_DEREFERENCE_ABLE, {
                        .type_1 = analysis_1.deduced_type});
                } else ret.deduced_type = analysis_1.deduced_type->to<PointerType>()->of_type;
                break;

            case Op::BITWISE_NOT:
                if (analysis_1.deduced_type && !analysis_1.deduced_type->isIntegral()) {
                    reportError(ErrCode::ONLY_INTEGRAL_BITWISE, {});
                    return {};
                }
            default:
                ret = analysis_1;
                break;
        }
    } else {
        // 2nd operand
        analysis_2 =
            node->op_type != Op::CAST_OP         &&
            node->op_type != Op::DOT             &&
            node->op_type != Op::ASSIGNMENT      ?
            inferType(node->operands.at(1), {.bound_type = analysis_1.deduced_type}) :
            TypeInfo{};

        switch (node->op_type) {
            case Op::DIV: {
                if (analysis_2.deduced_type == nullptr)
                    return {};

                if (analysis_1.deduced_type->isIntegral() && analysis_2.deduced_type->isIntegral()) {
                    ret.deduced_type = unify(analysis_1.deduced_type, analysis_2.deduced_type);
                    break;
                } if (analysis_1.deduced_type->isFloatingPoint() && analysis_2.deduced_type->isFloatingPoint()) {
                    ret.deduced_type = &GlobalTypeF64;
                    break;
                } reportError(ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = analysis_1.deduced_type,
                    .type_2 = analysis_2.deduced_type,
                });
                break;
            }

            case Op::MOD: {
                if (analysis_2.deduced_type == nullptr)
                    return {};

                if (analysis_1.deduced_type->isIntegral() && analysis_2.deduced_type->isIntegral()) {
                    ret.deduced_type = unify(analysis_1.deduced_type, analysis_2.deduced_type);
                    break;
                }

                reportError(ErrCode::INCOMPATIBLE_TYPES, {
                    .type_1 = analysis_1.deduced_type,
                    .type_2 = analysis_2.deduced_type,
                });
                break;
            }

            case Op::ASSIGNMENT: {
                ret.deduced_type = &GlobalTypeVoid;

                // if (!analysis_1.deduced_type->is_mutable) {
                //     reportError(ErrCode::CANNOT_ASSIGN_TO_CONST, {});
                // }

                analysis_2 = inferType(node->operands.at(1), {.bound_type = analysis_1.deduced_type});

                checkTypeCompatibility(analysis_2.deduced_type, analysis_1.deduced_type);
                ret.deduced_type = unify(analysis_1.deduced_type, analysis_2.deduced_type);
                break;
            }

            case Op::INDEXING_OP: {
                if (analysis_2.deduced_type && !analysis_2.deduced_type->isIntegral()) {
                    reportError(
                        ErrCode::NON_INTEGRAL_INDICES, {
                            .type_1 = analysis_2.deduced_type,
                        });
                    break;
                }

                ret.deduced_type = analysis_1.deduced_type->getWrappedType();
                node->common_type = ret.deduced_type;
                break;
            }

            case Op::CAST_OP: {
                inferType(node->operands.at(1), ctx);
                ret.deduced_type = node->operands.at(1)->getSwType();
                break;
            }

            case Op::DOT: {
                DisableErrorCode _(*this, ErrCode::UNDEFINED_IDENTIFIER);

                auto* accessed_id = node->getRHS()->getWrappedNodeOrInstance()->getIdent();
                if (!accessed_id) {
                    reportError(ErrCode::SYNTAX_ERROR, {
                        .msg = "Expected a named expression."});
                    return {};
                }

                const std::string_view accessed_member = accessed_id->full_qualification.front().name;

                // check whether the LHS is an operator (DOT is left-associative)
                if (node->getLHS()->getWrappedNodeOrInstance()->getNodeType() == ND_OP) {
                    // the analysis result's `computed_namespace` field would contain
                    // the referenced namespace, if it doesn't, an attempt is made to retrieve
                    // it via the deduced type's identifier
                    auto analysis_result = inferType(node->getLHS(), ctx);
                    Namespace* computed_namespace = analysis_result.computed_namespace;

                    if (!computed_namespace) {
                        computed_namespace = SymMan.lookupDecl(analysis_result.deduced_type->getIdent()).scope;
                    }

                    if (computed_namespace) {
                        // look for the RHS in the namespace
                        std::optional<IdentInfo*> id = computed_namespace->getIDInfoFor(accessed_member);

                        // indicates that no ID with the name exists
                        if (!id.has_value()) {
                            // TODO: put the namespace name in the error message
                            reportError(ErrCode::NO_SUCH_MEMBER, {.str_1 = accessed_member});
                            return {};
                        }

                        // now when the ID with the name does exist
                        const auto& member_tab_entry = SymMan.lookupDecl(*id);
                        accessed_id->value = *id;
                        ret.computed_namespace = member_tab_entry.scope;
                        ret.deduced_type = member_tab_entry.swirl_type;
                        node->common_type = ret.deduced_type;
                        return ret;
                    }

                    // TODO
                    reportError(ErrCode::NOT_A_NAMESPACE, {});
                    return {};
                }

                // now when the LHS isn't an OP:
                const auto accessed_type = getEvalType(node->getLHS())->getWrappedTypeOrInstance();
                auto lhs_id_info = accessed_type->getIdent();


                if (lhs_id_info != nullptr)
                {
                    const auto& lhs_tab_entry = SymMan.lookupDecl(lhs_id_info);
                    const Namespace* lhs_scope = lhs_tab_entry.scope;

                    if (!lhs_scope) {
                        reportError(ErrCode::NOT_A_NAMESPACE, {
                            .str_1 = lhs_id_info->toString()});
                        return {};
                    }

                    // look for the member in the scope
                    auto id = lhs_scope->getIDInfoFor(accessed_member);

                    if (!id.has_value()) {
                        reportError(ErrCode::NO_SUCH_MEMBER, {
                            .str_1 = accessed_member, .str_2 = lhs_id_info->toString()});
                        return {};
                    }


                    const auto& member_tab_entry = SymMan.lookupDecl(*id);
                    accessed_id->value = *id;

                    auto deduced_type = member_tab_entry.swirl_type;
                    auto computed_namespace = member_tab_entry.scope;

                    if (node->getRHS()->getWrappedNodeOrInstance()->getNodeType() == ND_CALL) {
                        const auto analysis_res = inferType(node->getRHS(), {
                            .is_method_call = true,
                            .bound_type = ctx.bound_type,
                            .method_id = *id,
                        });

                        deduced_type = analysis_res.deduced_type;

                        if (deduced_type && deduced_type->getIdent()) {
                            computed_namespace = SymMan.lookupDecl(deduced_type->getIdent()).scope;
                        }
                    }

                    ret.deduced_type = deduced_type;
                    ret.computed_namespace = computed_namespace;
                    node->common_type = ret.deduced_type;
                    return ret;
                } assert(0);
                return {};
            }


            case Op::EXP:
            case Op::EXP_ASSIGN:
                if (analysis_2.deduced_type && !analysis_2.deduced_type->isIntegral()) {
                    reportError(ErrCode::EXPONENTIAL_RHS_INTEGRAL, {});
                    return {};
                }

                ret.deduced_type = analysis_1.deduced_type;
                break;

            case Op::LOGICAL_AND:
            case Op::LOGICAL_OR:
            case Op::LOGICAL_EQUAL:
            case Op::LOGICAL_NOTEQUAL:
            case Op::LOGICAL_NOT:
            case Op::GREATER_THAN:
            case Op::GREATER_THAN_OR_EQUAL:
            case Op::LESS_THAN:
            case Op::LESS_THAN_OR_EQUAL:
                ret.deduced_type = &GlobalTypeBool;
                break;

            case Op::BITWISE_AND_ASSIGN:
            case Op::BITWISE_OR_ASSIGN:
            case Op::BITWISE_XOR_ASSIGN:
            case Op::BITWISE_LSHIFT_ASSIGN:
            case Op::BITWISE_RSHIFT_ASSIGN:

            case Op::BITWISE_XOR:
            case Op::BITWISE_AND:
            case Op::BITWISE_OR:
            case Op::BITWISE_LSHIFT:
            case Op::BITWISE_RSHIFT:
                if (analysis_1.deduced_type && analysis_2.deduced_type) {
                    if (!analysis_1.deduced_type->isIntegral() || !analysis_2.deduced_type->isIntegral()) {
                        reportError(ErrCode::ONLY_INTEGRAL_BITWISE, {});
                        return {};
                    }
                }

            default: {
                ret.deduced_type = unify(analysis_1.deduced_type, analysis_2.deduced_type);
            }
        }
    }

    if (node->arity == 2 && !node->common_type)
        node->common_type = unify(analysis_1.deduced_type, analysis_2.deduced_type);
    else node->common_type = analysis_1.deduced_type;
    return ret;
}
