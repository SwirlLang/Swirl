#pragma once
#include <ranges>

// #include "GenericSubstitutor.h"
#include "modules/ModuleManager.h"
#include "SemaVisitor.h"


namespace sema {
struct SymbolResolver : SemaVisitor<SymbolResolver> {
    SymbolManager&    SymMan;
    ModuleManager&    ModuleMap;
    std::unordered_map<IdentInfo*, Node*>& GlobalNodeJmpTable;
    // GenericSubstitutor Substitutor;


    struct Data {
        std::unordered_set<std::string_view> ignore_symbols{};
        std::unordered_map<std::string_view, Ident*> generic_args;
    };


    explicit SymbolResolver(Module* module, const ErrorCallback_t& error_callback)
        : SemaVisitor(module, error_callback)
        , SymMan(module->symbol_table)
        , ModuleMap(module->getModuleManager())
        , GlobalNodeJmpTable(module->node_jmp_table)
        // , Substitutor(module)
        {}


    void handle(ImportNode* node, const Data&) {
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

                GlobalNodeJmpTable.insert({id, ModuleMap.get(node->mod_handle).node_jmp_table.at(id)});
            }
        }
    }


    void handle(Function* node, Data data) {
        // when not being instantiated, do not attempt to resolve symbols which use generics
        if (!node->generic_params.empty() && data.generic_args.empty()) {
            for (const GenericParam* param : node->generic_params) {
                data.ignore_symbols.insert(param->name);
            }
        }

        for (Var* param : node->params) {
            visit(param, data);
        } visit(node->return_type, data);

        visit(node->children, data);
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

        for (auto& arg : node->args) {
            visit(arg, data);
        }
    }


    bool preVisit(Ident* node, const Data&) {
        if (node->full_qualification.empty()) {
            return false;
        } return true;
    }


    bool preVisit(Op* node, const Data& data) {
        if (node->op_type == Op::DOT) {
            return false;
        } return true;
    }


    void handle(Ident* node, const Data& data) {
        assert(!node->full_qualification.empty());

        // do not attempt resolution if the symbol is ignored
        if (data.ignore_symbols.contains(node->full_qualification.front().name)) {
            return;
        }

        if (!node->value) {
            node->value = SymMan.getIDInfoFor(*node,
                [this](const ErrCode code, ErrorContext ctx) {
                    reportError(code, std::move(ctx));
           });
        }

        if (!node->value) {
            reportError(ErrCode::UNDEFINED_IDENTIFIER, {
                .str_1 = node->full_qualification.back().name
            });
            return;
        }

        // if (!node->has_generic_args) {
        //     return;
        // }
        //
        // --- instantiate generics if present --- //
        // std::vector<Ident::Qualifier> tmp;
        //
        // for (auto&& [i, qual] : std::views::enumerate(node->full_qualification)) {
        //     tmp.push_back(qual);
        //
        //     // handle the generic arguments of this particular qual and build the substitution map
        //     if (!qual.generic_args.empty()) {
        //         SubstitutionMap_t map;
        //         Node* current_node = nullptr;
        //
        //         for (GenericArg* arg : qual.generic_args) {
        //             // handle the IDs in generic arguments first
        //             if (arg->isType()) {
        //                 auto a = arg->getType();
        //                 if (a->type_id) {
        //                     handle(a->type_id, data);
        //                 }
        //             }
        //
        //             // build the substitution map
        //             auto original_id = SymMan.getIDInfoFor(Ident(tmp));
        //             const auto decl = SymMan.lookupDecl(original_id);
        //             current_node = decl.node_ptr;
        //             node->value = original_id;
        //
        //             GenericParam* param = nullptr;
        //             assert(current_node->isGlobal());
        //             param = current_node->to<GlobalNode>()->generic_params.at(i);
        //
        //             map.insert({param->name, arg});
        //         }
        //
        //         // compute the new identifier
        //         std::string new_qual_name{tmp.back().name};
        //         for (const auto& gen_arg : tmp.back().generic_args) {
        //             // TODO: comptime values
        //             if (gen_arg->isType()) {
        //                 new_qual_name += '_' + gen_arg->getType()->getIDStr();
        //             }
        //         }
        //
        //         // the generic arguments of the qual have been resolved
        //         qual.generic_args = {};
        //         qual.name = internString(new_qual_name);
        //
        //         // do not instantiate if an instance already exists
        //         if (SymMan.getIdInfoOfAGlobal(std::string(tmp.back().name), false, false)) {
        //             continue;
        //         }
        //
        //         // instantiate
        //         auto ctx = SubstitutionContext{.map = map};
        //         const auto transformed_node = Substitutor.run(current_node, ctx);
        //         // TODO: change the .swirl_type field to reflect the instantiation
        //     }
        // }
        //
        // node->has_generic_args = false;
        //
        // // reevaluate the identifier
        // node->value = nullptr;
        // handle(node, data);
    }
};
}