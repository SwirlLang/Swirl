#pragma once
#include "SemaVisitor.h"


namespace sema {
class SymbolRegistrationPass : public SemaVisitor<SymbolRegistrationPass> {
public:
    explicit
    SymbolRegistrationPass(const SemaContext& context)
        : SemaVisitor(context.module, context.error_callback)
        , SymMan(context.module->symbol_table)
        , NodeJmpTable(context.module->node_jmp_table) {}


    // The scope being nullptr => the current scope is global
    std::vector<Scope*>      ScopeStack { nullptr };
    std::vector<StructType*> StructStack{ nullptr };

    // asks the immediate scope to use this namespace instead
    Namespace* PreCreatedScope = nullptr;

    SymbolManager& SymMan;
    std::unordered_map<IdentInfo*, Node*>& NodeJmpTable;


    void handle(Scope* node) {
        assert(!ScopeStack.empty());
        node->parent_scope = ScopeStack.back();

        if (!PreCreatedScope) {
            ScopeStack.push_back(node);

            if (!node->symbols)
                node->symbols = SymMan.newScope();

            if (StructStack.back() && !StructStack.back()->scope) {
                StructStack.back()->scope = node->symbols;
                SymMan.lookupDecl(StructStack.back()->ident).scope = node->symbols;
            }

            traverse(node);
            ScopeStack.pop_back();
        } else {
            // it is assumed that the setter has pushed the scope already
            node->symbols = PreCreatedScope;
            PreCreatedScope = nullptr;

            if (StructStack.back() && !StructStack.back()->scope) {
                StructStack.back()->scope = node->symbols;
                SymMan.lookupDecl(StructStack.back()->ident).scope = node->symbols;
            }

            traverse(node);
        }
    }


    void handle(Struct* node) {
        if (node->ident) return;
        node->ident = getNewIDInfo(node->name);

        const auto struct_ty = new StructType{};
        struct_ty->ident = node->ident;

        // build the field offsets
        std::size_t i = 0;
        for (Node* child : node->members->children) {
            if (child->kind == ND_VAR) {
                const auto field = static_cast<Var*>(child);
                struct_ty->field_offsets.insert({field->name.data(), i++});
            }
        }

        SymMan.registerType(node->ident, struct_ty);
        SymMan.registerDecl(node->ident, {
            .is_exported = node->is_exported,
            .swirl_type = struct_ty,
            .node_ptr = node,
        });

        node->members->symbols = SymMan.newScope();
        StructStack.push_back(struct_ty);

        registerGenericParameters(node->generic_params, node->members);
        traverse(node);

        StructStack.pop_back();


        if (isGlobalScope()) {
            NodeJmpTable.insert({node->ident, node});
        }
    }


    void handle(Ident* node) {
        if (node->value) return;

        // attempt to resolve possibly local symbols
        if (node->full_qualification.size() == 1 && node->full_qualification.at(0).generic_args.empty()) {
            if (const auto result = searchForSymbol(node->full_qualification.front().name)) {
                node->value = result;
            }
        }

        for (auto& [_, generic_args] : node->full_qualification ) {
            for (GenericArg* arg : generic_args) {
                visit(arg);
            }
        }
    }


    void handle(Function* node) {
        if (node->ident) return;
        node->ident = getNewIDInfo(node->name);

        const auto fn_type = new FunctionType();
        fn_type->ident = node->ident;

        assert(!StructStack.empty());
        TableEntry entry {
            .is_exported = node->is_exported,
            .is_static = node->is_static_method,
            .swirl_type = fn_type,
            .method_of = StructStack.back(),
            .node_ptr = node
        };

        SymMan.registerType(node->ident, fn_type);
        SymMan.registerDecl(node->ident, entry);

        ScopeStack.push_back(node->children);
        node->children->symbols = SymMan.newScope();
        PreCreatedScope = node->children->symbols;

        registerGenericParameters(node->generic_params, node->children);

        traverse(node);
        ScopeStack.pop_back();

        if (isGlobalScope()) {
            NodeJmpTable.insert({node->ident, node});
        }
    }


    bool preVisit(Var* node) {
        if (node->var_ident) return true;

        if (node->is_instance_param) {
            if (StructStack.empty() || !StructStack.back()) {
                reportError(ErrCode::NO_INSTANCE_PARAM_HERE, {});
                return false;
            }

            node->var_type = makeNode<TypeWrapper>(
                SymMan.getReferenceType(StructStack.back(), !node->is_const));
        }

        node->var_ident = getNewIDInfo(node->name);

        TableEntry entry;
        entry.is_const    = node->is_const;
        entry.is_volatile = node->is_volatile;
        entry.is_exported = node->is_exported;
        entry.is_param    = node->is_param;
        entry.node_ptr    = node;
        entry.is_comptime = node->is_comptime;

        SymMan.registerDecl(node->var_ident, entry);

        if (isGlobalScope()) {
            NodeJmpTable.insert({node->var_ident, node});
        }

        return true;
    }


    bool preVisit(Enum* node) const {
        node->ident = getNewIDInfo(node->name);

        TableEntry entry;
        entry.is_enum  = true;
        entry.node_ptr = node;
        entry.is_exported = node->is_exported;
        entry.scope = SymMan.newScope();

        const auto ty = new EnumType();
        ty->scope = entry.scope;
        ty->id = node->ident;

        // register all entries of the enumeration as fictitious ids
        for (const auto id : node->entries | std::views::keys) {
            const auto id_info = ty->scope->getNewIDInfo(id, true);
            SymMan.registerFictitiousID(id_info, node);
        }

        SymMan.registerDecl(node->ident, entry);
        SymMan.registerType(node->ident, ty);

        if (isGlobalScope()) {
            NodeJmpTable.insert({node->ident, node});
        }

        return true;
    }


    bool preVisit(Protocol* node) const {
        node->protocol_id = getNewIDInfo(node->name);

        TableEntry entry;
        entry.is_protocol = true;
        entry.node_ptr = node;
        entry.is_exported = node->is_exported;

        SymMan.registerDecl(node->protocol_id, entry);

        if (isGlobalScope()) {
            NodeJmpTable.insert({node->protocol_id, node});
        }

        return true;
    }


    IdentInfo* searchForSymbol(const std::string_view name) {
        if (const auto id = SymMan.getGlobalScope()->getIDInfoFor(name)) {
            assert(id.value());
            return id.value();
        }

        for (const Scope* scope : ScopeStack) {
            if (scope) {
                if (const auto id = scope->symbols->getIDInfoFor(name)) {
                    assert(id.value());
                    return id.value();
                }
            }
        }

        return nullptr;
    }

    IdentInfo* getNewIDInfo(const std::string_view name) const {
        assert(!ScopeStack.empty());

        if (const auto* scope = ScopeStack.back()) {
            assert(scope->symbols);
            return scope->symbols->getNewIDInfo(name);
        }

        return SymMan.getGlobalScope()->getNewIDInfo(name);
    }


    bool isGlobalScope() const {
        assert(!ScopeStack.empty());
        return !ScopeStack.back();
    }


private:
    void registerGenericParameters(std::span<GenericParam*> params, const Scope* scope) const {
        // register generic params as types and decls in the scope
        for (const auto& child : params) {
            const auto id = scope->symbols->getNewIDInfo(child->name);
            const auto gen_type = new GenericType();

            SymMan.registerDecl(id, {.swirl_type = gen_type});

            gen_type->id = id;
            SymMan.registerType(id, gen_type);
        }
    }
};
}