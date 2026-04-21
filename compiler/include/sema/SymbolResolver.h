#pragma once
#include "managers/ModuleManager.h"
#include "SemaVisitor.h"


namespace sema {
struct SymbolResolver : SemaVisitor<SymbolResolver> {
    SymbolManager&    SymMan;
    ModuleManager&    ModuleMap;
    std::unordered_map<IdentInfo*, Node*>& GlobalNodeJmpTable;

    struct Data {
        std::unordered_set<std::string> ignore_symbols{};
        std::unordered_map<std::string, Ident> generic_args;
    };


    explicit SymbolResolver(Parser& parser)
        : SemaVisitor(parser)
        , SymMan(parser.SymbolTable)
        , ModuleMap(parser.ModuleMap)
        , GlobalNodeJmpTable(parser.NodeJmpTable) {}


    void handle(ImportNode* node, const Data&) {
        if (!node->imported_symbols.empty()) {
            for (auto& symbol : node->imported_symbols) {
                IdentInfo* id = SymMan.getIdInfoFromModule(node->mod_handle, symbol.actual_name);

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
                    symbol.assigned_alias.empty() ? symbol.actual_name : symbol.assigned_alias,
                    id, node->is_exported
                    );

                GlobalNodeJmpTable.insert({id, ModuleMap.get(node->mod_handle).NodeJmpTable.at(id)});
            }
        }
    }


    void handle(Function* node, Data data) {
        // when not being instantiated, do not attempt to resolve symbols which use generics
        if (!node->generic_params.empty() && data.generic_args.empty()) {
            for (auto& param : node->generic_params) {
                data.ignore_symbols.insert(param.name);
            }
        }

        for (auto& param : node->params) {
            visit(&param, data);
        } visit(&node->return_type, data);

        for (auto& child : node->children) {
            visit(child.get(), data);
        }
    }


    void handle(FuncCall* node, Data data) {
        std::vector<GenericParam>* generic_params = nullptr;
        if (node->ident.value) {
            auto fn_node = SymMan.lookupDecl(node->ident.value).node_ptr->to<Function>();
            generic_params = &fn_node->generic_params;
        }

        visit(&node->ident, data);

        for (const auto& [i, arg] : std::views::enumerate(node->generic_args)) {
            // Id to node table required
            if (!generic_params) {
                reportError(ErrCode::NOT_A_GENERIC, {
                    .str_1 = node->ident.full_qualification.back().name});
                break;
            }

            if (i >= generic_params->size()) {
                reportError(ErrCode::TOO_MANY_GENERIC_ARGS, {});
                break;
            }

            data.generic_args.insert({generic_params->at(i).name, arg.type_id}); // TODO
        }

        for (auto& arg : node->args) {
            visit(&arg, data);
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

    void handle(Ident* node, const Data& data) const {
        assert(!node->full_qualification.empty());

        // do not attempt resolution if the symbol is ignored
        for (auto& symbol : node->full_qualification) {
            if (data.ignore_symbols.contains(symbol.name)) {
                return;
            }
        }

        // if generic arguments are involved, perform appropriate substitutions in the identifier
        if (!data.generic_args.empty()) {
            if (node->full_qualification.size() > 1) {
                for (auto& [name, _] : node->full_qualification) {
                    if (data.generic_args.contains(name)) {
                        assert(data.generic_args.at(name).full_qualification.size() == 1);
                        name = data.generic_args.at(name).full_qualification.front().name;
                    }
                }
            }
        }

        if (!node->value) {
            node->value = SymMan.getIDInfoFor(*node, [this](const ErrCode code, ErrorContext ctx) {
               reportError(code, std::move(ctx));
           });
        }

        if (!node->value) {
            reportError(ErrCode::UNDEFINED_IDENTIFIER, {
                .str_1 = node->full_qualification.back().name
            });
        }
    }

    void handle(TypeWrapper*, const Data&) {}
};
}