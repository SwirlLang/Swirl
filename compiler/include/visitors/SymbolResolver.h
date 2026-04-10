#pragma once
#include "visitors/SemaVisitor.h"


namespace sema {
struct SymbolResolver : SemaVisitor<SymbolResolver> {
    SymbolManager&    SymMan;

    struct Data {
        std::unordered_set<std::string> ignore_symbols{};
        std::unordered_map<std::string, Ident> generic_args;
    };


    explicit SymbolResolver(Parser& parser)
        : SemaVisitor(parser), SymMan(parser.SymbolTable) {}


    void handle(Function* node, Data data) {
        // when not being instantiated, do not attempt to resolve symbols which use generics
        if (!node->generic_params.empty() && data.generic_args.empty()) {
            for (auto& param : node->generic_params) {
                data.ignore_symbols.insert(param.name);
            }
        }
    }


    void handle(FuncCall* node, Data data) {
        std::vector<GenericParam>* generic_params = nullptr;
        if (node->ident.value) {
            auto fn_node = SymMan.lookupDecl(node->ident.value).node_ptr->to<Function>();
            generic_params = &fn_node->generic_params;
        }

        for (const auto& [i, arg] : std::views::enumerate(node->generic_args)) {
            // Id to node table required
            if (!generic_params) {
                reportError(ErrCode::NOT_A_GENERIC, {
                    .str_1 = node->ident.full_qualification.back().name});
                break;
            }

            if (i >= generic_params->size()) {
                // TODO: report error
                assert("too many generic args");
                break;
            }

            data.generic_args.insert({generic_params->at(i).name, arg.type_id}); // TODO
        }
    }


    bool preVisit(Ident* node, const Data&) {
        if (node->full_qualification.empty()) {
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