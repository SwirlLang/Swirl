#include <cassert>
#include <types/SwTypes.h>
#include <parser/Nodes.h>
#include <parser/Parser.h>
#include <parser/SemanticAnalysis.h>
#include <unordered_set>


using SwNode = std::unique_ptr<Node>;
using NodesVec = std::vector<SwNode>;

extern ModuleMap_t ModuleMap;

Type* AnalysisContext::deduceType(Type* type1, Type* type2, StreamState location) const {
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

        ErrMan.newError("conversion between signed and unsigned types shall be explicit, "
                        "use the `as` operator for casting.", location);
        return nullptr;
    }

    if (type1->isFloatingPoint() && type2->isFloatingPoint()) {
        if (type1->getBitWidth() >= type2->getBitWidth()) {
            return type1;
        } return type2;
    }

    ErrMan.newError("incompatible types!", location);
    return nullptr;
}


void AnalysisContext::checkTypeCompatibility(Type* from, Type* to, StreamState location) const {
    if (!from || !to) return;

    if (from->isIntegral() && to->isFloatingPoint()) {
        ErrMan.newError("Cannot implicitly convert an integral type to a floating point!", location);
        return;
    }

    if (from->isFloatingPoint() && to->isIntegral()) {
        ErrMan.newError("Cannot implicitly convert a floating-point type to an integral type!", location);
        return;
    }

    if (from->isIntegral() && to->isIntegral()) {
        if ((from->isUnsigned() && !to->isUnsigned()) || (!from->isUnsigned() && to->isUnsigned())) {
            ErrMan.newError("Cannot implicitly convert between signed and unsigned types!", location);
            return;
        }

        if (to->getBitWidth() < from->getBitWidth()) {
            ErrMan.newError("Implicit narrowing conversions are not allowed.", location);
            return;
        }
    }

    if (from->isFloatingPoint() && to->isFloatingPoint()) {
        if (to->getBitWidth() < from->getBitWidth()) {
            ErrMan.newError("Implicit narrowing conversions are not allowed.", location);
            return;
        }
    }

    if (!(from->isIntegral() && to->isIntegral()) && !(from->isFloatingPoint() && to->isFloatingPoint())) {
        ErrMan.newError("No implicit type-conversion exists for this case.", location);
        return;
    }
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

AnalysisResult StrLit::analyzeSemantics(AnalysisContext&) {
    return {.deduced_type = &GlobalTypeStr};
}


AnalysisResult ImportNode::analyzeSemantics(AnalysisContext& ctx) {
    // specific-symbol imports
    if (!imported_symbols.empty()) {
        for (auto& symbol : imported_symbols) {
            IdentInfo* id = SymbolManager::getIdInfoFromModule(mod_path, symbol.actual_name);

            if (!id) {
                ctx.ErrMan.newError(std::format(
                    "No symbol '{}' in module '{}'",
                    symbol.actual_name,
                    mod_path.string()
                ), location);
                continue;
            }

            ctx.SymMan.registerIdInfoForImportedSym(symbol.assigned_alias.empty() ? symbol.actual_name : symbol.assigned_alias, id);
            ctx.GlobalNodeJmpTable.insert({id, ModuleMap.get(mod_path).GlobalNodeJmpTable.at(id)});
        } return {};
    }

    if (alias.empty()) {
        auto name = mod_path.filename().replace_extension();
        if (ctx.ModuleNamespaceTable.contains(name)) {
            ctx.ErrMan.newError(
                std::format("Imported symbol '{}' already exists.",
                    name.string()), location);
            return {};
        } ctx.ModuleNamespaceTable.insert({name, mod_path});
    } else {
        if (ctx.ModuleNamespaceTable.contains(alias)) {
            ctx.ErrMan.newError(
                std::format("Imported symbol '{}' already exists.",
                    alias), location);
            return {};
        }
        ctx.ModuleNamespaceTable.insert({alias, mod_path});
    }

    return {};
}


AnalysisResult Var::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (!initialized && !var_type) {
        ctx.ErrMan.newError("Cannot leave a variable without an explicit type uninitialized!", location);
        return {};
    }

    ctx.setBoundTypeState(var_type);
    auto val_analysis = value.analyzeSemantics(ctx);
    ctx.restoreBoundTypeState();

    if (var_type == nullptr) {
        var_type = val_analysis.deduced_type;
        ctx.SymMan.lookupDecl(var_ident).swirl_type = var_type;
    } else {
        ctx.checkTypeCompatibility(val_analysis.deduced_type, var_type, location);
        value.setType(var_type);
    }

    return ret;
}

AnalysisResult FuncCall::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    ident.analyzeSemantics(ctx);

    IdentInfo* id = ident.getIdentInfo();

    if (!id) return {};

    if (ctx.getCurParentFunc()->getIdentInfo() != id)
        ctx.analyzeSemanticsOf(id);

    auto* fn_type = dynamic_cast<FunctionType*>(ctx.SymMan.lookupType(id));

    if (fn_type->ret_type == nullptr && id == ctx.getCurParentFunc()->getIdentInfo()) {
        ctx.ErrMan.newError("It is required to explicitly specify the return type "
                            "when calling the function recursively.", location);
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

    return ret;
}

AnalysisResult Ident::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (!value) {
        if (full_qualification.size() == 1) {
            value = ctx.SymMan.getIdInfoOfAGlobal(full_qualification.front());
        } else {
            if (full_qualification.size() == 2) {
                if (!ctx.ModuleNamespaceTable.contains(full_qualification.front())) {
                    ctx.ErrMan.newError("Undefined qualifier '" + full_qualification.front() + "'", location);
                    return {};
                } value = ctx.SymMan.getIdInfoFromModule(
                    ctx.ModuleNamespaceTable[full_qualification.front()],
                    full_qualification.back()
                    );
            }
        }
    }

    if (!value) {
        ctx.ErrMan.newError("Undefined identifier '" + full_qualification.back() + "'", location);
        return {};
    }

    auto decl = ctx.SymMan.lookupDecl(this->value);
    ret.deduced_type = decl.swirl_type;
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
    static std::recursive_mutex mtx;
    std::lock_guard lock(mtx);

    if (ctx.Cache.contains(this))
        return ctx.Cache[this];

    ctx.setCurParentFunc(this);

    AnalysisResult ret;
    Type* deduced_type = nullptr;

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
    return ret;
}


AnalysisResult Op::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;
    // 1st operand
    auto analysis_1 = operands.at(0)->analyzeSemantics(ctx);

    if (arity == 1) {
        if (value == "&") {
            uint16_t ptr_level = 1;
            if (analysis_1.deduced_type->getSwType() == Type::POINTER) {
                ptr_level = dynamic_cast<PointerType*>(analysis_1.deduced_type)->pointer_level + 1;
            }
            ret.deduced_type = ctx.SymMan.getPointerType(analysis_1.deduced_type, ptr_level);
        }

        else {
            ret.deduced_type = analysis_1.deduced_type;
        }

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
            ret.deduced_type = ctx.deduceType(analysis_1.deduced_type, analysis_2.deduced_type, location);
        }
    }
    
    return ret;
}

AnalysisResult Expression::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    assert(this->expr.size() == 1);
    auto val = this->expr.front()->analyzeSemantics(ctx);

    ret.deduced_type = val.deduced_type;
    setType(val.deduced_type);

    return ret;
}


AnalysisResult Assignment::analyzeSemantics(AnalysisContext& ctx) {
    AnalysisResult ret;

    if (l_value.getNodeType() == ND_IDENT) {
        if (ctx.SymMan.lookupDecl(l_value.getIdentInfo()).is_const) {
            ctx.ErrMan.newError("Cannot assign, " + l_value.getIdentInfo()->toString() + " is const.");
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
        ModuleMap.get(id->getModulePath()).GlobalNodeJmpTable.at(id)->analyzeSemantics(*this);
        return;
    } GlobalNodeJmpTable[id]->analyzeSemantics(*this);
}
