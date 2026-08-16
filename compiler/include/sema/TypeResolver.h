#pragma once
#include "ast/Nodes.h"
#include "comptime/ComptimeEvaluator.h"
#include "transformers/GenericInstantiator.h"
#include "transformers/ProtocolSubstitutor.h"
#include "types/definitions.h"
#include "sema/SemaVisitor.h"
#include "transformers/VariadicGenerator.h"


namespace sw {
class ComptimeEvaluator;
}

namespace sema {
/// Used to pass context down the call graph
struct TypeContext {
    bool       is_method_call = false;
    Type*      bound_type     = nullptr;
    IdentInfo* method_id      = nullptr;
};


class TypeResolver : public SemaVisitor<TypeResolver> {
public:
    SymbolManager&     SymMan;
    Type*              CommonFunctionType = nullptr;
    std::size_t        ReturnStmtCounter  = 0;

    std::vector<Function*> CurrentParentFunction = {nullptr};

    sw::GenericInstantiator GenericInstantiator;
    sw::ComptimeEvaluator   ComptimeEvaluator;
    sw::VariadicGenerator   VariadicExpander;

    std::unordered_set<std::string_view> GenericParameters;


    bool IsMonomorphization = false;


    inline static GlobalCache VisitedNodes;

    explicit TypeResolver(const SemaContext& context)
        : SemaVisitor(context.module, context.error_callback)
        , SymMan(context.module->symbol_table)
        , GenericInstantiator(m_Module, context.error_callback)
        , ComptimeEvaluator(context.module, context.error_callback, &GenericParameters)
        , IsMonomorphization(context.is_monomorphization)
        , VariadicExpander(
                m_Module,
                [this](const ErrCode code, const ErrorContext& ctx) {
                reportError(code, ctx);
            }) {}


    /// Computation result of type evaluation
    struct TypeInfo {
        Type* deduced_type = nullptr;
        Namespace* computed_namespace = nullptr;
    };


    /// Computes a type which is compatible with both `type1` and `type2`
    Type* unify(Type* type1, Type* type2);

    /// Checks whether `from` can be converted to `to`
    bool checkTypeCompatibility(
        Type* from,
        Type* to,
        bool report_errors = true,
        const std::optional<SourceLocation>& loc = std::nullopt);


    Function* getCurrentParentFunc() const {
        assert(!CurrentParentFunction.empty());
        return CurrentParentFunction.back();
    }


    std::optional<Module::ProtocolImplInfo> getProtocolInfo(Type* for_type, ProtocolConstraint* protocol) {
        if (const auto lookup = m_Module->lookupProtocolImpl(for_type, protocol)) {
            if (!lookup->is_exported && m_Module != lookup->parent_module) {
                reportError(ErrCode::PROTO_IMPL_NOT_EXPORTED, {
                    .str_1 = protocol->toString(),
                    .str_2 = for_type->toString()
                });
            } return lookup;
        } return std::nullopt;
    }


    /// Resolves `member` as a member of `primary_scope` and when `owner_type`
    /// is a type -- of its protocol impl-scopes. Reports NO_SUCH_MEMBER or
    /// AMBIGUOUS_MEMBER. Returns `nullptr` on failure.
    IdentInfo* resolveMember(const Namespace* primary_scope, Type* owner_type, const std::string_view member) {
        std::vector<const Namespace*> scopes;
        std::vector<Module::ImplScopeRef> impl_refs;

        if (primary_scope) scopes.push_back(primary_scope);
        if (owner_type) {
            for (const auto& ref : m_Module->getImplScopesFor(owner_type)) {
                scopes.push_back(ref.info->scope);
                impl_refs.push_back(ref);
            }
        }

        const auto matches = SymMan.resolveMember(scopes, member);
        if (matches.empty()) {
            reportError(ErrCode::NO_SUCH_MEMBER, {.str_1 = member});
            return nullptr;
        }
        if (matches.size() > 1) {
            reportError(ErrCode::AMBIGUOUS_MEMBER, {.str_1 = member});
            return nullptr;
        }

        // impl members are visible within the module that declared the impl;
        // from other modules they require `export impl`
        if (matches[0].found_in != primary_scope) {
            for (const auto& ref : impl_refs) {
                if (ref.info->scope == matches[0].found_in
                    && ref.info->parent_module != m_Module
                    && !ref.info->is_exported)
                {
                    reportError(ErrCode::PROTO_IMPL_NOT_EXPORTED, {
                        .str_1 = ref.protocol->toString(),
                        .str_2 = owner_type ?
                                 owner_type->toString() : "???"
                    });
                    return nullptr;
                }
            }
        }

        return matches[0].id;
    }


    bool preVisit(Function* node) {
        if (VisitedNodes.contains(node) || !node->params.empty() && node->params.back()->is_variadic) {
            return false;
        }

        for (GenericParam* param : node->generic_params) {
            GenericParameters.insert(param->name);
        }

        VisitedNodes.insert(node);
        CurrentParentFunction.push_back(node);
        return true;
    }


    void postVisit(const Function* node) {
        auto* fn_type = SymMan.lookupType(node->ident)->to<FunctionType>();

        // early return if `postVisit` skips over
        if (CurrentParentFunction.back() != node) {
            ReturnStmtCounter = 0;
            CommonFunctionType = nullptr;
            return;
        }

        for (const GenericParam* param : node->generic_params) {
            GenericParameters.erase(param->name);
        }

        if (!fn_type->ret_type)
            fn_type->ret_type = CommonFunctionType;

        if (fn_type->ret_type == nullptr && ReturnStmtCounter == 0) {
            fn_type->ret_type       = &GlobalTypeVoid;
            node->return_type->type = &GlobalTypeVoid;
        }

        CurrentParentFunction.pop_back();
        ReturnStmtCounter = 0;
        CommonFunctionType = nullptr;
    }


    TypeInfo inferType(Node* node, const TypeContext& ctx) {
        #define SW_NODE(x, y) case x: return evaluateType(static_cast<y*>(node), ctx);
        switch (node->kind) {
            SW_NODE_LIST // NOLINT(*-pro-type-static-cast-downcast)
            default: throw std::runtime_error("TypeResolver::inferType: unknown kind");
        }
        #undef SW_NODE
    }


    TypeInfo evaluateType(Node*, const TypeContext&) {
        return {};
    }


    TypeInfo evaluateType(IntLit* node, const TypeContext& ctx) {
        if (ctx.bound_type && ctx.bound_type->isIntegral()) {
            return {.deduced_type = ctx.bound_type};
        } return {.deduced_type = &GlobalTypeI32};
    }


    TypeInfo evaluateType(FloatLit* node, const TypeContext& ctx) {
        if (ctx.bound_type && ctx.bound_type->isFloatingPoint()) {
            return {.deduced_type = ctx.bound_type};
        } return {.deduced_type = &GlobalTypeF64};
    }


    TypeInfo evaluateType(BoolLit* node, const TypeContext& ctx) {
        return {.deduced_type = &GlobalTypeBool};
    }


    TypeInfo evaluateType(CharLit* node, const TypeContext& ctx) {
        return {.deduced_type = &GlobalTypeChar};
    }


    TypeInfo evaluateType(StrLit* node, const TypeContext& ctx) {
        return {.deduced_type = SymMan.lookupType(SymMan.getIdInfoOfAGlobal("str"))};
    }


    TypeInfo evaluateType(ArrayLit* node, const TypeContext& ctx) {
        Type* common_type = nullptr;

        if (node->elements.empty()) {
            return {.deduced_type = &GlobalUniversalType};
        }

        // loop over the elements and compute a type which is compatible with all
        // the expressions inside
        for (const auto& element : node->elements) {
            if (common_type) {
                common_type = unify(inferType(element, ctx).deduced_type, common_type);
            } else common_type = inferType(element, ctx).deduced_type;
        }

        if (common_type) {
            return {.deduced_type = SymMan.getArrayType(common_type, node->elements.size())};
        } return {};
    }


    TypeInfo evaluateType(TypeWrapper* node, const TypeContext& ctx) {
        if (node->type) { return {.deduced_type = node->type}; }

        Type* ret = nullptr;

        // has id
        if (node->type_id && !node->type_id->full_qualification.empty()) {
            if (node->type_id->value == nullptr) {
                node->type_id->value = SymMan.getIDInfoFor(
                   *node->type_id, [this](auto a, auto b) {
                       reportError(a, std::move(b));
                   });
            }

            if (node->type_id->value != nullptr) {
                monomorphize(node->type_id);
                ret = SymMan.lookupType(node->type_id->value);
            }
        }

        // is an array
        else if (node->array_size != nullptr) {
            const auto arr_of_type = evaluateType(node->of_type, ctx).deduced_type;
            std::size_t array_size = 0;

            node->array_size->expr_type = &GlobalTypeI64;

            // evaluate the comptime expression and set it as the array size
            const auto trans_node = ComptimeEvaluator.transform(node->array_size);
            if (!ComptimeEvaluator.errorsOccurred() && trans_node != node->array_size) {
                array_size = sw::ComptimeEvaluator::toUInt64(
                    trans_node->to<Expression>()->expr->to<IntLit>()->value);
            }

            if (arr_of_type != nullptr) {
                ret = SymMan.getArrayType(arr_of_type, array_size);
            }
        }

        // is a slice
        else if (node->is_slice) {
            auto slice_of_type = evaluateType(node->of_type, ctx);
            if (slice_of_type.deduced_type != nullptr) {
                ret = SymMan.getSliceType(slice_of_type.deduced_type, node->is_mutable);
            }
        }

        // is a pointer
        else if (node->is_pointer) {
            auto pointer_of_ty = evaluateType(node->of_type, ctx);
            if (pointer_of_ty.deduced_type != nullptr) {
                ret = SymMan.getPointerType(pointer_of_ty.deduced_type, node->is_mutable);
            }
        }

        // is a reference
        else if (node->is_reference) {
            auto ref_of_type = evaluateType(node->of_type, ctx);
            if (ref_of_type.deduced_type != nullptr) {
                ret = SymMan.getReferenceType(ref_of_type.deduced_type, node->is_mutable);
            }
        }

        else return {.deduced_type = &GlobalTypeVoid};

        if (!ret) {
            reportError(ErrCode::NO_SUCH_TYPE, {});
            return {};
        }

        node->type = ret;
        return {.deduced_type = ret};
    }


    TypeInfo evaluateType(Intrinsic* node, const TypeContext& ctx) {
        Type* deduced_type = nullptr;

        if (node->intrinsic_type != Intrinsic::ADV_PTR) {
            for (auto& arg : node->args) {
                if (deduced_type == nullptr) {
                    deduced_type = inferType(arg, ctx).deduced_type;
                } else deduced_type = unify(deduced_type, inferType(arg, ctx).deduced_type);
            }
        }

        Type* res = SymbolManager::IntrinsicTable.at(node->intrinsic_type).return_type;
        if (res == nullptr) {
            switch (node->intrinsic_type) {
                case Intrinsic::ADV_PTR: {
                    assert(node->args.size() == 2);

                    inferType(node->args.at(0), ctx);
                    inferType(node->args.at(1), ctx);

                    if (node->args.size() < 2) {
                        reportError(ErrCode::TOO_FEW_ARGS, {});
                        return {};
                    } res = node->args.at(0)->expr_type;
                    break;
                }
                default: res = deduced_type;
            }
        } return {.deduced_type = res};
    }


    TypeInfo evaluateType(Ident* node, const TypeContext& ctx) {
        if (!node->value) {
            // idents deferred by the SymbolResolver (e.g. impl-provided members)
            node->value = SymMan.getIDInfoFor(*node,
                [this](const ErrCode code, ErrorContext e) {
                    reportError(code, std::move(e));
                });
            if (!node->value)
                return {};  // an error was already reported
        }
        Type* ret = nullptr;
        if (node->value->isFictitious()) {
            const auto enum_node = SymMan.getFictitiousIDValue(node->value);
            inferType(*enum_node->enum_type, ctx);

            // fetch the enum type
            const Enum* en = SymMan.getFictitiousIDValue(node->value);
            ret = SymMan.lookupType(en->ident);
        } else {
            // instantiate generics
            monomorphize(node);
            const auto decl = SymMan.lookupDecl(node->value);
            ret = decl.swirl_type;
        }

        return {.deduced_type = ret};
    }


    TypeInfo evaluateType(Expression* node, const TypeContext& ctx) {
        const auto ret = inferType(node->expr, ctx);
        node->setType(ret.deduced_type);
        return ret;
    }


    TypeInfo evaluateType(FuncCall* node, const TypeContext& ctx) {
        if (ctx.is_method_call) {
            assert(ctx.method_id);
            node->ident->value = ctx.method_id;
        } else if (!node->ident->value) {
            // generic-param targets are resolved during monomorphization, so
            // don't attempt to resolve them as globals here
            if (!GenericParameters.contains(node->ident->full_qualification.front().name)) {
                // idents deferred by the SymbolResolver (e.g. impl-provided members)
                node->ident->value = SymMan.getIDInfoFor(*node->ident,
                    [this](const ErrCode code, ErrorContext e) {
                        reportError(code, std::move(e));
                    });
                if (!node->ident->value)
                    return {};  // an error was already reported
            }
        } visit(node->ident);
        // assuming `SymbolResolver` has resolved the ID otherwise

        if (GenericParameters.contains(node->ident->full_qualification.front().name) &&
            !node->ident->getIdentInfo())
            return {};

        assert(node->ident->getIdentInfo());
        monomorphize(node->ident);

        auto* id = node->ident->getIdentInfo();

        const auto fn_node = SymMan.lookupDecl(id).node_ptr->to<Function>();
        assert(fn_node);

        // ---  check for variadics --- //
        if (!fn_node->params.empty() && fn_node->params.back()->is_variadic) {
            std::vector<Type*> variadic_types;
            auto start_index = fn_node->params.size() - 1;

            // account for the implicit instance parameter
            if (ctx.is_method_call && fn_node->params.front()->is_instance_param)
                start_index -= 1;

            while (start_index < node->args.size()) {
                auto expr_type = inferType(node->args[start_index], {}).deduced_type;
                variadic_types.push_back(expr_type);
                start_index++;
            }

            bool is_valid = true;

            // run a type compatibility check if the variadic specifies an explicit type
            if (const auto variadic_ty = fn_node->params.back()->type) {
                evaluateType(variadic_ty, {});
                if (variadic_ty->type) {
                    for (auto [i, ty] : std::views::enumerate(variadic_types)) {
                        auto arg_idx = i + (fn_node->params.size() - 1);

                        // account for the implicit instance parameter
                        if (ctx.is_method_call && fn_node->params.front()->is_instance_param)
                            arg_idx -= 1;

                        const auto arg_node = node->args.at(arg_idx);
                        is_valid &= checkTypeCompatibility(
                            ty, variadic_ty->type, true, arg_node->location);
                    }
                }
            }

            if (is_valid) {
                node->ident->value = expandVariadics(node, variadic_types);
                id = node->ident->value;
                assert(node->ident->value);
            } else return {};
        }  // --- --- --- --- --- --- //

        // if not a recursive-case, ensure the function is handled first
        if (getCurrentParentFunc()->getIdentInfo() != id) {
            const auto function = SymMan.lookupDecl(id).node_ptr;
            assert(function);

            if (function->getNodeType() != ND_FUNC) {
                reportError(ErrCode::NOT_CALLABLE, {
                    .str_1 = id->toString(),
                    .location = node->location
                });

                return {};
            }

            visit(function);
        }

        // fetch the corresponding Function's type
        auto* fn_type = SymMan.lookupType(id)->to<FunctionType>();

        // check whether the number of arguments is correct
        if (node->args.size() != fn_type->param_types.size()) {
            if (!ctx.is_method_call) {
                reportError(ErrCode::NOT_ENOUGH_ARGS, {
                    .type_1 = fn_type,
                    .str_1  = node->ident->toString(),
                    .str_2  = std::to_string(node->args.size())
                }); return {};
            }

            // is method call
            if (node->args.size() != (fn_type->param_types.size() - 1)) {
                reportError(ErrCode::NOT_ENOUGH_ARGS, {
                    .type_1 = fn_type,
                    .str_1 = id->toString(),
                    .str_2 = std::to_string(node->args.size())
                });
                return {};
            }
        }

        // when recursion is involved and return type of the function is not specified,
        // report an error
        if (fn_type->ret_type == nullptr && getCurrentParentFunc()
            && id == getCurrentParentFunc()->getIdentInfo())
            {
            reportError(ErrCode::RET_TYPE_REQUIRED, {});
            return {};
            }

        // check the type compatibility between the function signature and the arguments
        for (std::size_t i = 0; i < node->args.size(); i++) {
            const std::size_t index = i + (ctx.is_method_call ? 1 : 0);
            const auto arg_type = inferType(node->args.at(i), {
                .bound_type = fn_type->param_types.at(index)
            });

            checkTypeCompatibility(
                arg_type.deduced_type,
                fn_type->param_types.at(index),
                true, node->args.at(i)->location);
            node->args.at(i)->setType(fn_type->param_types.at(index));
        }

        return {.deduced_type = fn_type->ret_type};
    }

    TypeInfo evaluateType(Op* node, const TypeContext& ctx);


    void handle(Function* node) {
        if (node->return_type) {
            inferType(node->return_type, {});
        } else node->return_type = makeNode<TypeWrapper>();

        auto* fn_type = SymMan.lookupType(node->ident)->to<FunctionType>();
        fn_type->ret_type = node->return_type->type;

        CommonFunctionType = fn_type->ret_type;

        fn_type->param_types.clear();

        assert(fn_type->param_types.size() <= 1);
        for (Parameter* param : node->params) {
            handle(param);

            auto param_type = param->type->type;
            fn_type->param_types.push_back(param_type);
            SymMan.lookupDecl(param->ident).swirl_type = param_type;
        }

        // visit children
        visit(node->children);
    }

    bool preVisit(TypeWrapper* node) {
        node->type = evaluateType(node, {}).deduced_type;
        return true;
    }


    // resolve the type alias to a concrete type
    void handle(const TypeAlias* node) {
        if (node->alias_for) {
            visit(node->alias_for);
            if (node->alias_for->type) {
                assert(node->ident);
                SymMan.registerType(node->ident, node->alias_for->type);
            }
        }
    }


    void handle(Intrinsic* node) {
        evaluateType(node, {});
    }

    void handle(ReturnStatement* node) {
        ReturnStmtCounter++;

        Type* bound_type = !CommonFunctionType ? nullptr :
            (CommonFunctionType->isArrayType()
            ? CommonFunctionType->to<ArrayType>()->of_type
            : CommonFunctionType);

        const auto type = node->value->expr
            ? inferType(node->value, {.bound_type = bound_type}).deduced_type
            : &GlobalTypeVoid;

        if (CommonFunctionType) {
            if (checkTypeCompatibility(type, CommonFunctionType)) {
                node->value->setType(CommonFunctionType);
            }
        } else {
            CommonFunctionType = type;
        }

        assert(getCurrentParentFunc());
        node->parent_fn_type = SymMan.lookupType(getCurrentParentFunc()->getIdentInfo())->to<FunctionType>();
        assert(node->parent_fn_type != nullptr);
    }


    void handle(Enum* node) {
        if (node->enum_type.has_value()) {
            const auto ty = inferType(node->enum_type.value(), {}).deduced_type;
            if (ty && !ty->isIntegral()) {
                reportError(ErrCode::ENUM_TYPE_NOT_INTEGRAL, {});
            }
        } else {
            node->enum_type = makeNode<TypeWrapper>();
            node->enum_type.value()->type = &GlobalTypeI32;
        }

        const auto ty = SymMan.lookupType(node->ident)->to<EnumType>();
        ty->of_type = node->enum_type.value()->type;
    }


    void handle(const Parameter* node) {
        if (node->type) {
            visit(node->type);
            inferType(node->type, {});
        }

        if (node->type) {
            SymMan.lookupDecl(node->ident).swirl_type = node->type->type;
        }
    }


    void handle(Var* node) {
        if (node->var_type) {
            visit(node->var_type);
            inferType(node->var_type, {});
        }

        if (!node->initialized && node->is_const) {
            reportError(
                ErrCode::INITIALIZER_REQUIRED,
                {.ident = node->var_ident}
            ); return;
        }

        Type* bound_type = node->var_type ?
            ( node->var_type->type ? (node->var_type->type->isArrayType()
            ? node->var_type->type->to<ArrayType>()->of_type
            : node->var_type->type) : nullptr
            ) : nullptr;


        if (node->initialized) {
            const auto val_ty = inferType(node->value, {.bound_type = bound_type}).deduced_type;

            if (!node->var_type) {
                node->var_type = makeNode<TypeWrapper>();
                node->var_type->type = val_ty;
                node->value->setType(val_ty);
            } else {
                checkTypeCompatibility(val_ty, node->var_type->type);
                assert(node->value->getNodeType() == ND_EXPR);
                node->value->setType(val_ty);
            }
        }

        SymMan.lookupDecl(node->var_ident).swirl_type = node->var_type->type;
        if (node->var_type->type) {
            assert(node->var_type->type->getTypeTag() != Type::VOID);
        }
    }


    void handle(Expression* node) {
        const auto ty = inferType(node->expr, {}).deduced_type;
        node->setType(ty);
    }


    void handle(Condition* node) {
        const auto if_condition_ty = inferType(node->bool_expr, {}).deduced_type;

        if (!checkTypeCompatibility(if_condition_ty, &GlobalTypeBool, false)) {
            reportError(ErrCode::CONDITION_NOT_BOOL, {});
        }

        visit(node->if_children);

        for (auto& [condition, children] : node->elif_children) {
            const auto cond_ty = inferType(condition, {}).deduced_type;
            if (!checkTypeCompatibility(cond_ty, &GlobalTypeBool, false)) {
                reportError(ErrCode::CONDITION_NOT_BOOL, {});
            }

            visit(children);
        }

        visit(node->else_children);
    }


    bool preVisit(Protocol* node) {
        if (VisitedNodes.contains(node))
            return false;

        VisitedNodes.insert(node);
        return true;
    }


    void handle(const Protocol* node) {
        const auto protocol_ty =
            SymMan.lookupDecl(node->ident).swirl_type->to<ProtocolConstraint>();

        // true if the type references one of the protocol-mandated associated types,
        // either directly or indirectly
        const auto references_mandated_alias = [](const auto& self, const TypeWrapper* ty,
                                                      const Protocol* protocol) -> bool {
            if (!ty) return false;

            if (ty->type_id && !ty->type_id->full_qualification.empty()) {
                const auto& name = ty->type_id->full_qualification.front().name;
                for (const TypeAlias* alias : protocol->type_aliases) {
                    if (alias->alias == name) return true;
                }

                for (const auto& qual : ty->type_id->full_qualification) {
                    for (const GenericArg* arg : qual.generic_args) {
                        if (arg->isType() && self(self, arg->getType(), protocol)) return true;
                    }
                }
            }

            if (ty->of_type && self(self, ty->of_type, protocol)) return true;
            return false;
        };

        for (auto& method : node->methods) {
            std::vector<Type*> type_constraints;

            for (auto [i, ty] : std::views::enumerate(method.params)) {
                // the instance parameter is a placeholder whose type is bound to a
                // reference of the implementing type during conformance checking
                if (method.is_instance_method && i == 0) {
                    type_constraints.push_back(nullptr);
                    continue;
                }

                if (references_mandated_alias(references_mandated_alias, ty, node)) {
                    type_constraints.push_back(nullptr);
                } else {
                    visit(ty);
                    type_constraints.push_back(ty->type);
                }
            }

            if (method.return_type) {
                if (references_mandated_alias(references_mandated_alias, method.return_type, node)) {
                    type_constraints.push_back(nullptr);
                } else {
                    visit(method.return_type);
                    type_constraints.push_back(method.return_type->type);
                }
            } else type_constraints.push_back(&GlobalTypeVoid);

            protocol_ty->method_constrains.insert({
                method.name, std::move(type_constraints)
            });
        }
    }


    void handle(const ProtocolImpl* node) {
        assert(node->protocol->value);

        const auto target_protocol_id = node->protocol->value;
        const auto target_protocol = SymMan.lookupDecl(target_protocol_id).node_ptr->to<Protocol>();

        visit(target_protocol);

        // resolve the implementation's declared aliases first; the resulting
        // bindings are used to substitute associated-type references in both the
        // implementation's own method signatures and the protocol's requirements
        ProtocolSubstitutor substitutor{m_Module};
        ProtocolSubstitutor::AliasMap_t alias_map;
        for (Node* member : node->children->children) {
            if (member->kind == ND_TYPE_ALIAS) {
                const auto* alias = member->to<TypeAlias>();
                visit(alias->alias_for);
                if (alias->alias_for && alias->alias_for->type) {
                    alias_map[alias->alias] = alias->alias_for->type;
                }
            }
        }

        // set of type aliases and methods that the implementation provides
        // the target protocol requirements will be checked against these
        std::unordered_set<std::string_view> type_aliases;
        std::unordered_map<std::string_view, Protocol::MethodSignature<true>> methods;
        std::unordered_map<std::string_view, SourceLocation> impl_locs;

        // the implementing type resolves through the type-table rather than the
        // symbol table so that builtin types (e.g. `impl P for i32`) -- which
        // have no decl entries -- can serve as the implementation target

        visit(node->impl_for);
        Type* impl_type = node->impl_for->type;
        if (!impl_type) return;

        for (Node* member : node->children->children) {
            switch (member->kind) {
                case ND_TYPE_ALIAS: {
                    const auto* alias = member->to<TypeAlias>();
                    type_aliases.insert(alias->alias);
                    break;
                }

                case ND_FUNC: {
                    auto* func = member->to<Function>();
                    SymMan.lookupDecl(func->ident).method_of = impl_type;
                    SymMan.lookupDecl(func->ident).protocol_of = node->protocol->getIdentInfo();

                    func->return_type = static_cast<TypeWrapper*>(
                        substitutor.substitute(func->return_type, alias_map));

                    for (auto& param : func->params) {
                        if (param->type) {
                            param->type = static_cast<TypeWrapper*>(
                                substitutor.substitute(param->type, alias_map));
                        }
                    }

                    const auto is_instance_method =
                        !func->params.empty() && func->params.front()->is_instance_param;

                    // bind the instance parameter's type to a reference of the
                    // implementing type, matching the instance parameter's mutability
                    if (is_instance_method) {
                        func->params.front()->type = makeNode<TypeWrapper>(
                            SymMan.getReferenceType(impl_type, !func->params.front()->is_const));
                    }

                    visit(func);

                    Protocol::MethodSignature<true> sign;
                    sign.name = func->name;
                    sign.return_type = func->return_type;
                    sign.is_instance_method = is_instance_method;

                    for (const Parameter* param : func->params) {
                      sign.params.push_back(param->type);
                    } methods.insert({sign.name, sign});
                    impl_locs[sign.name] = func->location;
                      break;
                } default: {}
            }
        }

        // check whether all required type aliases are defined
        bool all_aliases_present = true;
        for (const TypeAlias* alias : target_protocol->type_aliases) {
            if (!type_aliases.contains(alias->alias)) {
                reportError(ErrCode::TYPE_ALIAS_REQUIRED, {
                    .str_1 = target_protocol_id->toString(),
                    .str_2 = alias->alias,
                });
                all_aliases_present = false;
            }
        }

        // without the aliases we cannot normalize the protocol's signatures,
        // so only check the method bodies when every required alias is bound
        if (!all_aliases_present) return;

        // check whether all required methods are implemented.
        // the protocol-mandated associated types are substituted with the impl's
        // bindings first, so the protocol's declared signature and the impl's can
        // be compared structurally
        auto protocol_ty = SymMan.lookupDecl(target_protocol_id).swirl_type->to<ProtocolConstraint>();

        const auto protocol_str = target_protocol_id->toString();
        const auto report_mismatch = [this](const std::string& protocol, const std::string_view method,
                                            std::string&& msg, const SourceLocation loc) {
            reportError(ErrCode::PROTOCOL_METHOD_MISMATCH, {
                .msg = std::move(msg), .str_1 = protocol, .str_2 = method, .location = loc
            });
        };

        const auto check_satisfied = [&](Type* expected, Type* provided,
                                         const std::string& protocol, const std::string_view method,
                                         const std::string_view what, const SourceLocation loc) {
            if (provided == expected) return;

            if (expected && expected->getTypeTag() == Type::PROTOCOL && provided &&
                getProtocolInfo(provided, expected->to<ProtocolConstraint>()).has_value())
                return;

            report_mismatch(protocol, method,
                std::format("{}: expected `{}`, found `{}`.",
                            what,
                            expected ? expected->toString() : "<void>",
                            provided ? provided->toString() : "<void>"),
                loc);
        };

        for (const auto& sig : target_protocol->methods) {
            const auto impl_it = methods.find(sig.name);
            if (impl_it == methods.end()) {
                reportError(ErrCode::PROTOCOL_VIOLATED, {.str_1 = protocol_str, .str_2 = sig.name});
                continue;
            }

            const auto& impl_sig = impl_it->second;
            const auto impl_loc = impl_locs[sig.name];

            // instance vs static: the implementation must match the protocol's method
            // kind (the mutability of `&self` is not part of the contract)
            if (impl_sig.is_instance_method != sig.is_instance_method) {
                report_mismatch(protocol_str, sig.name,
                    std::format("expected {}, found {}.",
                                sig.is_instance_method ? "an instance method (taking `&self`)"
                                                       : "a static method",
                                impl_sig.is_instance_method ? "an instance method (taking `&self`)"
                                                            : "a static method"),
                    impl_loc);
                continue;
            }

            if (impl_sig.params.size() != sig.params.size()) {
                report_mismatch(protocol_str, sig.name,
                    std::format("expected {} parameter(s), found {}.",
                                sig.params.size(), impl_sig.params.size()),
                    impl_loc);
                continue;
            }

            // drop the instance parameter from both sides: it is a placeholder on the
            // protocol side and binds to a reference of the implementing type on the
            // impl side, so there is nothing to compare
            std::vector<TypeWrapper*> expected_params;
            for (auto [i, ty] : std::views::enumerate(sig.params)) {
                if (!(sig.is_instance_method && i == 0))
                    expected_params.push_back(ty);
            }

            std::vector<TypeWrapper*> provided_params;
            for (auto [i, ty] : std::views::enumerate(impl_sig.params)) {
                if (!(impl_sig.is_instance_method && i == 0))
                    provided_params.push_back(ty);
            }

            const auto substituted = substitutor.substitute(expected_params, alias_map);
            for (auto [i, ty] : std::views::enumerate(substituted)) {
                visit(ty);
                check_satisfied(ty->type, provided_params[i]->type, protocol_str, sig.name,
                                std::format("parameter #{}", sig.is_instance_method ? i + 1 : i),
                                impl_loc);
            }

            auto* provided_ret =
                impl_sig.return_type ? impl_sig.return_type->type : &GlobalTypeVoid;

            if (sig.return_type) {
                auto* expected_ret = static_cast<TypeWrapper*>
                    (substitutor.substitute(sig.return_type, alias_map));
                visit(expected_ret);
                check_satisfied(expected_ret->type, provided_ret, protocol_str, sig.name,
                                "return type", impl_loc);
            } else {
                check_satisfied(&GlobalTypeVoid, provided_ret, protocol_str, sig.name,
                                "return type", impl_loc);
            }
        }

        // Check whether the impl already exists and insert it into the table
        if (!m_Module->insertProtocolImpl(
            impl_type, protocol_ty,
            {   .is_exported = node->is_exported,
                .scope = node->children->symbols,
                .parent_module = m_Module}))
        {
            reportError(ErrCode::DUPLICATE_PROTO_IMPL, {
                .str_1 = protocol_ty->toString(),
                .str_2 = node->impl_for ? node->impl_for->type->toString() : "???"
            });
        }
    }


    void handle(const WhileLoop* node) {
        const auto condition_ty = inferType(node->condition, {}).deduced_type;
        if (!checkTypeCompatibility(condition_ty, &GlobalTypeBool, false)) {
            reportError(ErrCode::CONDITION_NOT_BOOL, {});
        }

        visit(node->children);
    }


    bool preVisit(const Struct* node) {
        for (const GenericParam* param : node->generic_params) {
            GenericParameters.insert(param->name);
        } return true;
    }


    void postVisit(Struct* node) {
        const auto ty = SymMan.lookupType(node->ident)->to<StructType>();
        ty->field_types.clear();

        for (const auto& member : node->members->children) {
            if (member->getNodeType() == ND_VAR) {
                const auto var_node = member->to<Var>();

                ty->field_types.push_back(var_node->var_type->type);
            }

            else if (member->getNodeType() == ND_FUNC) {
                const auto fn_node = member->to<Function>();

                std::vector<TypeWrapper*> param_types;
                param_types.reserve(fn_node->params.size());
                for (const Parameter* param : fn_node->params) {
                    param_types.push_back(param->type);
                }
            }

            for (const GenericParam* param : node->generic_params) {
                GenericParameters.erase(param->name);
            }
        }
    }


    /// Checks whether the identifier contains any generic arguments and
    /// initiates monomorphization if it does.
    void monomorphize(Ident* ident)  {
        if (ident->has_generic_args) {
            // resolve all generic arguments (types)
            for (auto& [name, args, _] : ident->full_qualification) {
                for (const GenericArg* arg : args) {
                    if (arg->isType()) {
                        visit(arg->getType());
                    }
                }
            }

            GenericInstantiator.handle(ident);
            assert(ident->getIdentInfo());
            assert(SymMan.lookupDecl(ident->getIdentInfo()).node_ptr);
            visit(SymMan.lookupDecl(ident->getIdentInfo()).node_ptr);
        }
    }


    [[nodiscard]] /// Expands the variadic
    IdentInfo* expandVariadics(const FuncCall* node, std::span<Type*> types) {
        assert(node->ident && node->ident->value);

        const auto fn_node = SymMan.lookupDecl(node->ident->value).node_ptr->to<Function>();
        assert(fn_node != nullptr);

        if (const auto last_param = fn_node->params.back(); last_param->is_variadic) {
            sw::VariadicGenerator::Context ctx {
                .types = types,
                .variadic_name = last_param->name
            };

            const auto new_node = VariadicExpander.transform(fn_node, ctx);
            visit(new_node);
            return new_node->to<Function>()->ident;
        } return node->ident->value;
    }


    Type* getEvalType(Node* node) const {
        switch (node->getNodeType()) {
            case ND_INT:
                return &GlobalTypeI32;
            case ND_FLOAT:
                return &GlobalTypeF64;
            case ND_BOOL:
                return &GlobalTypeBool;
            case ND_STR:
                return SymMan.lookupType("str");
            case ND_IDENT:
                return SymMan.lookupDecl(node->getIdentInfo()).swirl_type;
            case ND_CALL:
                return dynamic_cast<FunctionType*>(
                    SymMan.lookupType(node->getIdentInfo()))->ret_type;
            case ND_EXPR:
            case ND_ARRAY:
            case ND_OP:
                return node->getSwType();
            default:
                throw std::runtime_error("TypeResolver::fetchSwType: failed to fetch type");
        }
    }
};
}
