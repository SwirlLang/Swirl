#pragma once
#include <ranges>

#include "SemaVisitor.h"


namespace sema {
struct SymbolResolver : SemaVisitor<SymbolResolver> {
    SymbolManager&    SymMan;
    ModuleManager&    ModuleMap;

    struct Data {
        std::unordered_set<std::string_view> ignore_symbols{};
        std::unordered_map<std::string_view, Ident*> generic_args;
    };


    explicit SymbolResolver(const SemaContext& context)
        : SemaVisitor(context.module, context.error_callback)
        , SymMan(context.module->symbol_table)
        , ModuleMap(context.module->getModuleManager())
        {}


    void handle(const ImportNode* node, const Data&) {
        if (!node->imported_symbols.empty()) {
            for (auto& symbol : node->imported_symbols) {
                IdentInfo* id = SymMan.getIdInfoFromModule(
                    node->mod_handle, std::string(symbol.actual_name));

                if (!id) {
                    reportError(
                        ErrCode::SYMBOL_NOT_FOUND_IN_MOD,
                        {.path_1 = node->mod_handle->getPath(), .str_1 = symbol.actual_name}
                        );
                    continue;
                }

                if (!SymMan.lookupDecl(id).is_exported) {
                    reportError(
                        ErrCode::SYMBOL_NOT_EXPORTED,
                        {.str_1 = symbol.actual_name}
                        );
                }

                // make the symbol manager aware of the foreign symbol's `IdentInfo*`
                SymMan.registerForeignID(
                    symbol.assigned_alias.empty() ?
                          std::string(symbol.actual_name)
                        : std::string(symbol.assigned_alias),
                    id, node->is_exported
                    );
            }
        }
    }


    void handle(const Function* node, Data data) {
        // when not being instantiated, do not attempt to resolve symbols which use generics
        if (!node->generic_params.empty() && data.generic_args.empty()) {
            for (const GenericParam* param : node->generic_params) {
                data.ignore_symbols.insert(param->name);
            }
        }

        for (Parameter* param : node->params) {
            visit(param, data);
        } visit(node->return_type, data);

        visit(node->children, data);
    }


    void handle(const Protocol* node, Data data) {
        // mandated associated types are placeholders, add them to the ignored-symbols set
        for (TypeAlias* alias : node->type_aliases) {
            if (!alias->alias_for) {
                data.ignore_symbols.insert(alias->alias);
            }
        }

        for (const auto& method : node->methods) {
            for (TypeWrapper* ty : method.params) {
                visit(ty, data);
            }
            visit(method.return_type, data);
        }

        for (TypeAlias* alias : node->type_aliases) {
            visit(alias, data);
        }
    }

    void handle(FuncCall* node, Data data) {
        std::span<GenericParam*>* generic_params = nullptr;
        if (node->ident->value) {
            auto fn_node = SymMan.lookupDecl(node->ident->value).node_ptr->to<Function>();
            generic_params = &fn_node->generic_params;
        }

        visit(node->ident, data);

        for (const auto& [i, arg] : std::views::enumerate(node->generic_args)) {
            // Id to node table required
            if (!generic_params) {
                reportError(ErrCode::NOT_A_GENERIC, {
                    .str_1 = node->ident->full_qualification.back().name});
                break;
            }

            if (i >= generic_params->size()) {
                reportError(ErrCode::TOO_MANY_GENERIC_ARGS, {});
                break;
            }

            if (arg->isType()) {
                data.generic_args.insert({generic_params->at(i)->name, arg->getType()->type_id}); // TODO
            }
        }

        for (const auto& arg : node->args) {
            visit(arg, data);
        }
    }


    void handle(const Op* node, Data data) {
        if (node->op_type == Op::DOT) {
            // only visit LHS; RHS method-name Ident is handled by TypeResolver::evaluateType(Op*)
            visit(node->operands.at(0), std::move(data));
        } else {
            for (auto& operand : node->operands) {
                visit(operand, data);
            }
        }
    }


    void handle(Ident* node, const Data& data) {
        assert(!node->full_qualification.empty());

        // do not attempt resolution if the symbol is ignored
        if (data.ignore_symbols.contains(node->full_qualification.front().name)) {
            return;
        }

        // Type-qualified members (`Type::member`) may be provided by protocol
        // impls, which are only registered by the TypeResolver, i.e. after this
        // pass. Defer such lookups.
        const bool defer_to_type_resolver = [this](const Ident& id) {
            if (id.full_qualification.size() <= 1)
                return false;

            auto* first = SymMan.getIdInfoOfAGlobal(
                std::string(id.full_qualification.front().name), false, false);
            if (!first)
                return false;

            const auto& entry = SymMan.lookupDecl(first);
            return entry.scope != nullptr && !entry.is_mod_namespace;
        }(*node);

        if (!node->value) {
            node->value = SymMan.getIDInfoFor(*node,
                defer_to_type_resolver
                    ? std::nullopt
                    : std::optional<ErrorCallback_t>{[this](const ErrCode code, ErrorContext ctx) {
                        reportError(code, ctx);
                    }});
        }

        if (!node->value && !defer_to_type_resolver) {
            reportError(ErrCode::UNDEFINED_IDENTIFIER, {
                .str_1 = node->full_qualification.back().name
            });
        }
    }
};
}