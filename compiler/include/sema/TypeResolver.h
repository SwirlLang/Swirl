#pragma once
#include <print>
#include "types/definitions.h"
#include "sema/SemaVisitor.h"


namespace sema {
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


    inline static GlobalCache VisitedNodes;

    explicit TypeResolver(Parser& parser)
        : SemaVisitor(parser)
        , SymMan(parser.SymbolTable)
        , GlobalNodeJmpTable(parser.NodeJmpTable) {}


    Type* unify(Type* type1, Type* type2);
    bool checkTypeCompatibility(Type* from, Type* to, bool report_errors = true);

    struct TypeInfo {
        Type* deduced_type = nullptr;
        Namespace* computed_namespace = nullptr;
    };

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


    void postVisit(Function* node) {
        auto* fn_type = SymMan.lookupType(node->ident)->to<FunctionType>();

        if (!fn_type->ret_type) {
            fn_type->ret_type = CommonFunctionType;
        }

        if (fn_type->ret_type == nullptr && ReturnStmtCounter == 0) {
            fn_type->ret_type      = &GlobalTypeVoid;
            node->return_type.type = &GlobalTypeVoid;
        }

        if (CurrentParentFunction.back() == node) {
            CurrentParentFunction.pop_back();
        }

        ReturnStmtCounter = 0;
        CommonFunctionType = nullptr;
    }



    TypeInfo inferType(Node* node, TypeContext ctx) {
        #define SW_NODE(x, y) case x: return evaluateType(static_cast<y*>(node), ctx);
        switch (node->kind) {
            SW_NODE_LIST // NOLINT(*-pro-type-static-cast-downcast)
            default: throw std::runtime_error("TypeResolver::inferType: unknown kind");
        }
        #undef SW_NODE
    }


    TypeInfo inferType(const SwNode& node, const TypeContext& ctx) {
        return inferType(node.get(), ctx);
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

        for (auto& element : node->elements) {
            if (common_type) {
                common_type = unify(inferType(&element, ctx).deduced_type, common_type);
            } else common_type = inferType(&element, ctx).deduced_type;
        }

        if (common_type) {
            return {.deduced_type = SymMan.getArrayType(common_type, node->elements.size())};
        } return {};
    }


    TypeInfo evaluateType(TypeWrapper* node, const TypeContext& ctx) {
        if (node->type) {
            return {.deduced_type = node->type};
        }

        Type* ret = nullptr;

        // has id
        if (!node->type_id.full_qualification.empty()) {
            const auto type_id_info = SymMan.getIDInfoFor(
                node->type_id, [this](auto a, auto b) {
                    reportError(a, std::move(b));
                });

            node->type_id.value = type_id_info;
            if (node->type_id.value != nullptr) {
                ret = SymMan.lookupType(node->type_id.value);
            }
        }

        // is an array
        else if (node->array_size) {
            auto arr_of_type = evaluateType(node->of_type.get(), ctx);
            if (arr_of_type.deduced_type != nullptr) {
                ret = SymMan.getArrayType(arr_of_type.deduced_type, node->array_size);
            }
        }

        // is a slice
        else if (node->is_slice) {
            auto slice_of_type = evaluateType(node->of_type.get(), ctx);
            if (slice_of_type.deduced_type != nullptr) {
                ret = SymMan.getSliceType(slice_of_type.deduced_type, node->is_mutable);
            }
        }

        else return {.deduced_type = &GlobalTypeVoid};

        if (!ret) {
            reportError(ErrCode::NO_SUCH_TYPE, {});
            return {};
        }

        // handle references and pointers
        for (const auto mod : node->modifiers | std::views::reverse) {
            if (mod == TypeWrapper::Reference) {
                ret = SymMan.getReferenceType(ret, node->is_mutable);
            } else if (mod == TypeWrapper::Pointer) {
                ret = SymMan.getPointerType(ret, node->is_mutable);
            }
        }

        node->type = ret;
        return {.deduced_type = ret};
    }


    TypeInfo evaluateType(Intrinsic* node, const TypeContext& ctx) {
        Type* deduced_type = nullptr;

        if (node->intrinsic_type != Intrinsic::ADV_PTR) {
            for (auto& arg : node->args) {
                if (deduced_type == nullptr) {
                    deduced_type = inferType(&arg, ctx).deduced_type;
                } else deduced_type = unify(deduced_type, inferType(&arg, ctx).deduced_type);
            }
        }

        Type* res = SymbolManager::IntrinsicTable.at(node->intrinsic_type).return_type;
        if (res == nullptr) {
            switch (node->intrinsic_type) {
                case Intrinsic::ADV_PTR: {
                    assert(node->args.size() == 2);

                    inferType(&node->args.at(0), ctx);
                    inferType(&node->args.at(1), ctx);

                    if (node->args.size() < 2) {
                        reportError(ErrCode::TOO_FEW_ARGS, {});
                        return {};
                    } res = node->args.at(0).expr_type;
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
            inferType(&*enum_node->enum_type, ctx);
            ret = enum_node->enum_type->type;
        } else {
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
            node->ident.value = ctx.method_id;
        } // assuming `SymbolResolver` has resolved the ID otherwise

        assert(node->ident.getIdentInfo());
        auto* id = node->ident.getIdentInfo();

        // if not a recursive-case, ensure the function is handled first
        if (getCurrentParentFunc()->getIdentInfo() != id) {
            const auto function = SymMan.lookupDecl(id).node_ptr;
            assert(function && function->getNodeType() == ND_FUNC);
            analyzeNodeWithID(id);
        }

        // fetch the corresponding Function's type
        auto* fn_type = SymMan.lookupType(id)->to<FunctionType>();

        // check whether the number of arguments is correct
        if (node->args.size() != fn_type->param_types.size()) {
            if (!ctx.is_method_call) {
                reportError(ErrCode::NOT_ENOUGH_ARGS, {
                    .type_1 = fn_type,
                    .str_1  = node->ident.toString(),
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
            auto arg_type = inferType(&node->args.at(i), {
                .bound_type = fn_type->param_types.at(index)
            });

            checkTypeCompatibility(arg_type.deduced_type, fn_type->param_types.at(index));
            node->args.at(i).setType(fn_type->param_types.at(index));
        }

        return {.deduced_type = fn_type->ret_type};
    }

    TypeInfo evaluateType(Op* node, TypeContext ctx);

    void handle(Function* node) {
        inferType(&node->return_type, {});

        auto* fn_type = SymMan.lookupType(node->ident)->to<FunctionType>();
        fn_type->ret_type = node->return_type.type;

        CommonFunctionType = fn_type->ret_type;

        assert(fn_type->param_types.size() <= 1);
        for (auto& param : node->params) {
            visit(&param);
            auto param_type = param.var_type.value().type;
            fn_type->param_types.push_back(param_type);
            SymMan.lookupDecl(param.var_ident).swirl_type = param_type;
        }
    }


    void handle(ReturnStatement* node) {
        ReturnStmtCounter++;

        Type* bound_type = !CommonFunctionType ? nullptr :
            (CommonFunctionType->isArrayType()
            ? CommonFunctionType->to<ArrayType>()->of_type
            : CommonFunctionType);

        const auto type = node->value.expr
            ? inferType(&node->value, {.bound_type = bound_type}).deduced_type
            : &GlobalTypeVoid;

        if (CommonFunctionType) {
            if (checkTypeCompatibility(type, CommonFunctionType)) {
                node->value.setType(CommonFunctionType);
            }
        }

        if (!CommonFunctionType) {
            CommonFunctionType = type;
        }

        assert(getCurrentParentFunc());
        node->parent_fn_type = SymMan.lookupType(getCurrentParentFunc()->getIdentInfo())->to<FunctionType>();
        assert(node->parent_fn_type != nullptr);
    }


    void handle(Enum* node) {
        if (node->enum_type.has_value()) {
            const auto ty = inferType(&node->enum_type.value(), {}).deduced_type;
            if (ty && !ty->isIntegral()) {
                reportError(ErrCode::ENUM_TYPE_NOT_INTEGRAL, {});
            } else {
                node->enum_type = TypeWrapper{};
                node->enum_type->type = &GlobalTypeI32;
            }
        }

        const auto ty = SymMan.lookupType(node->ident)->to<EnumType>();
        ty->of_type = node->enum_type->type;
    }


    void handle(Var* node) {
        if (node->var_type.has_value()) {
            inferType(&*node->var_type, {});
        }


        if (!node->initialized && !node->is_param && (!node->var_type.has_value() || node->is_const)) {
            reportError(
                ErrCode::INITIALIZER_REQUIRED,
                {.ident = node->var_ident}
            ); return;
        }


        Type* bound_type = node->var_type.has_value()
            ? (node->var_type->type->isArrayType()
              ? node->var_type->type->to<ArrayType>()->of_type
              : node->var_type->type)
            : nullptr;


        if (node->initialized) {
            auto val_ty = inferType(&node->value, {.bound_type = bound_type}).deduced_type;

            if (!node->var_type.has_value() || node->var_type->type == nullptr) {
                node->var_type = TypeWrapper{};
                node->var_type->type = val_ty;
                node->value.setType(val_ty);
            } else {
                checkTypeCompatibility(val_ty, node->var_type->type);
                node->value.setType(val_ty);
            }
        }

        SymMan.lookupDecl(node->var_ident).swirl_type = node->var_type->type;
    }


    void handle(Condition* node) {
        const auto if_condition_ty = inferType(&node->bool_expr, {}).deduced_type;

        if (!checkTypeCompatibility(if_condition_ty, &GlobalTypeBool, false)) {
            reportError(ErrCode::CONDITION_NOT_BOOL, {});
        }

        for (auto& [condition, _] : node->elif_children) {
            const auto cond_ty = inferType(&condition, {}).deduced_type;
            if (!checkTypeCompatibility(cond_ty, &GlobalTypeBool, false)) {
                reportError(ErrCode::CONDITION_NOT_BOOL, {});
            }
        }
    }


    void handle(WhileLoop* node) {
        const auto condition_ty = inferType(&node->condition, {}).deduced_type;
        if (!checkTypeCompatibility(condition_ty, &GlobalTypeBool, false)) {
            reportError(ErrCode::CONDITION_NOT_BOOL, {});
        }
    }

    void postVisit(Struct* node) {
        const auto ty = SymMan.lookupType(node->ident)->to<StructType>();

        std::unordered_set<Protocol::MemberSignature> member_lookup;
        std::unordered_set<Protocol::MethodSignature> method_lookup;

        for (const auto& member : node->members) {
            if (member->getNodeType() == ND_VAR) {
                const auto var_node = member->to<Var>();

                ty->field_types.push_back(var_node->var_type->type);
                member_lookup.insert({
                    .name = var_node->var_ident->toString(),
                    .type = std::move(var_node->var_type.value())});
            }

            else if (member->getNodeType() == ND_FUNC) {
                const auto fn_node = member->to<Function>();

                std::vector<TypeWrapper> param_types;
                param_types.reserve(fn_node->params.size());
                for (auto& param : fn_node->params) {
                    param_types.push_back(std::move(param.var_type.value()));
                }

                method_lookup.insert({
                    .name = fn_node->ident->toString(),
                    .return_type = std::move(fn_node->return_type),
                    .params = std::move(param_types)
                });
            }
        }

        // enforce protocols
        for (auto& proto_id : node->protocols) {
            if (proto_id.value) {
                const auto protocol = SymMan.lookupDecl(proto_id.value).node_ptr->to<Protocol>();
                if (!protocol) {
                    reportError(ErrCode::NO_SUCH_PROTOCOL, {.location = proto_id.location});
                    continue;
                }

                for (auto& mem : protocol->members) {
                    if (!member_lookup.contains(mem)) {
                        reportError(ErrCode::PROTOCOL_VIOLATED, {
                            .str_1 =   node->ident->toString(),
                            .str_2 =   mem.toString()
                        });
                    }
                }

                for (auto& method : protocol->methods) {
                    if (!method_lookup.contains(method)) {
                        reportError(ErrCode::PROTOCOL_VIOLATED, {
                            .str_1 = proto_id.toString(),
                            .str_2 = method.toString()
                        });
                    }
                }
            }
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
                return &GlobalTypeStr;
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