#pragma once
#include <print>

#include "ast/Visitor.h"
#include "comptime/ComptimeEvaluator.h"
#include "generics/GenericInstantiator.h"
#include "modules/ModuleManager.h"
#include "types/definitions.h"
#include "sema/SemaVisitor.h"


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
    std::unordered_map<IdentInfo*, Node*> GlobalNodeJmpTable;

    sw::GenericInstantiator GenericInstantiator;

    bool IsMonomorphization = false;


    inline static GlobalCache VisitedNodes;

    explicit TypeResolver(const SemaContext& context)
        : SemaVisitor(context.module, context.error_callback)
        , SymMan(context.module->symbol_table)
        , GlobalNodeJmpTable(context.module->node_jmp_table)
        , GenericInstantiator(m_Module, context.error_callback)
        , IsMonomorphization(context.is_monomorphization) {}


    /// Computation result of type evaluation
    struct TypeInfo {
        Type* deduced_type = nullptr;
        Namespace* computed_namespace = nullptr;
    };


    /// Computes a type which is compatible with both `type1` and `type2`
    Type* unify(Type* type1, Type* type2);

    /// Checks whether `from` can be converted to `to`
    bool checkTypeCompatibility(Type* from, Type* to, bool report_errors = true);


    Function* getCurrentParentFunc() const {
        assert(!CurrentParentFunction.empty());
        return CurrentParentFunction.back();
    }


    bool preVisit(Function* node) {
        if (VisitedNodes.contains(node)) {
            return false;
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


    TypeInfo evaluateType(StrLit* node, const TypeContext& ctx) {
        return {.deduced_type = SymMan.lookupType(SymMan.getIdInfoOfAGlobal("str"))};
    }


    TypeInfo evaluateType(ArrayLit* node, const TypeContext& ctx) {
        Type* common_type = nullptr;

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
        else if (!std::holds_alternative<std::monostate>(node->array_size)) {
            const auto arr_of_type = evaluateType(node->of_type, ctx).deduced_type;
            std::size_t array_size  = 0;

            // if array size is hardcoded
            if (std::holds_alternative<std::size_t>(node->array_size)) {
                array_size = std::get<std::size_t>(node->array_size);
            }

            // if array size is a comptime expression
            if (Node** size = std::get_if<Node*>(&node->array_size)) {
                if (const auto nd = *size; nd->getNodeType() == ND_EXPR) {
                    // evaluate the comptime expression and use it as the array size
                    array_size = sw::ComptimeEvaluator::toUInt64(
                        nd->to<Expression>()->expr->to<IntLit>()->value);
                }
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
        assert(node->value);
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
        } visit(node->ident);
        // assuming `SymbolResolver` has resolved the ID otherwise

        assert(node->ident->getIdentInfo());
        monomorphize(node->ident);

        auto* id = node->ident->getIdentInfo();

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

            analyzeNodeWithID(id);
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

        // when recursion is involved and return type of the function is not specified, report an error
        if (fn_type->ret_type == nullptr && getCurrentParentFunc() && id == getCurrentParentFunc()->getIdentInfo()) {
            reportError(ErrCode::RET_TYPE_REQUIRED, {});
            return {};
        }

        // check the type compatibility between the function signature and the arguments
        for (std::size_t i = 0; i < node->args.size(); i++) {
            const std::size_t index = i + (ctx.is_method_call ? 1 : 0);
            const auto arg_type = inferType(node->args.at(i), {
                .bound_type = fn_type->param_types.at(index)
            });

            checkTypeCompatibility(arg_type.deduced_type, fn_type->param_types.at(index));
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
        for (Var* param : node->params) {
            handle(param);

            auto param_type = param->var_type->type;
            fn_type->param_types.push_back(param_type);
            SymMan.lookupDecl(param->var_ident).swirl_type = param_type;
        }

        // visit children
        visit(node->children);
    }


    bool preVisit(TypeWrapper* node) {
        node->type = evaluateType(node, {}).deduced_type;
        return true;
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


    void handle(Var* node) {
        if (node->var_type) {
            visit(node->var_type);
            inferType(node->var_type, {});
        }

        if (!node->initialized && (!node->is_param && node->is_const)) {
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


    void handle(WhileLoop* node) {
        const auto condition_ty = inferType(node->condition, {}).deduced_type;
        if (!checkTypeCompatibility(condition_ty, &GlobalTypeBool, false)) {
            reportError(ErrCode::CONDITION_NOT_BOOL, {});
        }

        visit(node->children);
    }


    void postVisit(Struct* node) {
        const auto ty = SymMan.lookupType(node->ident)->to<StructType>();
        ty->field_types.clear();

        std::unordered_set<Protocol::MemberSignature> member_lookup;
        std::unordered_set<Protocol::MethodSignature> method_lookup;

        for (const auto& member : node->members->children) {
            if (member->getNodeType() == ND_VAR) {
                const auto var_node = member->to<Var>();

                ty->field_types.push_back(var_node->var_type->type);
                member_lookup.insert({
                    .name = var_node->var_ident->toString(),
                    .type = var_node->var_type});
            }

            else if (member->getNodeType() == ND_FUNC) {
                const auto fn_node = member->to<Function>();

                std::vector<TypeWrapper*> param_types;
                param_types.reserve(fn_node->params.size());
                for (const Var* param : fn_node->params) {
                    param_types.push_back(param->var_type);
                }

                method_lookup.insert({
                    .name = fn_node->ident->toString(),
                    .return_type = fn_node->return_type,
                    .params = internArray<TypeWrapper*>(param_types)
                });
            }
        }

        // enforce protocols
        for (Ident* proto_id : node->protocols) {
            if (proto_id->value) {
                const auto protocol = SymMan.lookupDecl(proto_id->value).node_ptr->to<Protocol>();
                if (!protocol) {
                    reportError(ErrCode::NO_SUCH_PROTOCOL, {.location = proto_id->location});
                    continue;
                }

                for (auto& mem : protocol->members) {
                    if (!member_lookup.contains(mem)) {
                        reportError(ErrCode::PROTOCOL_VIOLATED, {
                            .str_1 =   node->ident->toString(),
                            .str_2 =   mem.name
                        });
                    }
                }

                for (auto& method : protocol->methods) {
                    if (!method_lookup.contains(method)) {
                        reportError(ErrCode::PROTOCOL_VIOLATED, {
                            .str_1 = proto_id->toString(),
                            .str_2 = "<placeholder>"  // TODO
                        });
                    }
                }
            }
        }
    }


    /// Checks whether the identifier contains any generic arguments and
    /// initiates monomorphization if it does.
    void monomorphize(Ident* ident)  {
        if (ident->has_generic_args) {
            // resolve all generic arguments (types)
            for (auto& [name, args] : ident->full_qualification) {
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


    void analyzeNodeWithID(IdentInfo* id) {
        if (!GlobalNodeJmpTable.contains(id)) { return; }
        this->visit(GlobalNodeJmpTable[id]);
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
