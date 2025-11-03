#include "parser/Parser.h"
#include "managers/ModuleManager.h"


TableEntry& SymbolManager::lookupDecl(IdentInfo* id) {
    if (const auto mod_path = id->getModulePath(); mod_path != m_ModulePath) {
        return m_ModuleMap.get(mod_path).SymbolTable.m_IdToTableEntry.at(id);
    } return m_IdToTableEntry.at(id);
}

Type* SymbolManager::lookupType(IdentInfo* id) {
    if (!id) return nullptr;
    if (const auto mod_path = id->getModulePath(); mod_path != m_ModulePath) {
        return m_ModuleMap.get(mod_path).SymbolTable.m_TypeManager.getFor(id);
    } return m_TypeManager.getFor(id);
}

IdentInfo* SymbolManager::getIdInfoFromModule(const std::filesystem::path& mod_path, const std::string& name) const {
    return m_ModuleMap.get(mod_path).SymbolTable.getIdInfoOfAGlobal(name, true);
}

IdentInfo* SymbolManager::instantiateGenerics(IdentInfo* id, const std::vector<TypeWrapper*>& args, const ErrorCallback_t& err) {
    const auto node = lookupDecl(id).node_ptr;
    assert(node != nullptr);

    // TODO: potential race condition
    Parser& parser_instance = m_ModuleMap.get(id->getModulePath());
    const auto cloned_node = parser_instance.cloneNode(id);
    auto glob_node = cloned_node->to<GlobalNode>();

    std::vector<Type*> type_values;
    type_values.reserve(args.size());

    for (auto& arg: args) {
        type_values.push_back(arg->type);
    }

    auto instantiated_node = glob_node->instantiate(parser_instance, type_values, err);
    parser_instance.AST.push_back(std::move(instantiated_node));
    return parser_instance.AST.back()->getIdentInfo();
}


IdentInfo* SymbolManager::getIDInfoFor(
    const Ident& id,
    const std::optional<ErrorCallback_t>& err_callback,
    const std::optional<ErrorCallback_t>& generic_err_callback) {

    auto report_error = [&err_callback](ErrCode code, const ErrorContext& ctx) {
        if (err_callback.has_value())
            (*err_callback)(code, ctx);
    };

    if (id.full_qualification.size() == 1) {
        auto glob_id = getIdInfoOfAGlobal(id.full_qualification.front().name);

        // handle generic arguments
        if (!id.full_qualification.front().generic_args.empty()) {
            assert(generic_err_callback.has_value());
            return instantiateGenerics(glob_id, id.full_qualification.front().generic_args, *generic_err_callback);
        }
        return glob_id;
    }

    const Namespace* look_at = nullptr;
    for (const auto& [counter, str] : llvm::enumerate(id.full_qualification)) {
        if (counter == id.full_qualification.size() - 1) break;
        if (counter == 0) {
            const auto qual_id = getIdInfoOfAGlobal(str.name);
            if (!qual_id)
                return nullptr;

            auto tmp = lookupDecl(qual_id);
            look_at = tmp.scope;
            continue;
        }

        if (!look_at) {
            report_error(
                ErrCode::NOT_A_NAMESPACE,
                {
                    .str_1 = id.full_qualification.at(counter - 1).name,
                    .location = id.location
                });
            return nullptr;
        }

        const auto& tmp = lookupDecl(look_at->getIDInfoFor(str.name).value());
        if (!tmp.is_exported) {
            report_error(
                ErrCode::SYMBOL_NOT_EXPORTED,
                {
                    .str_1 = str.name,
                    .location = id.location
                });
            return nullptr;
        }
        look_at = tmp.scope;
    }

    if (!look_at) {
        report_error(
            ErrCode::NOT_A_NAMESPACE,
            {
                .str_1 = id.full_qualification.at(id.full_qualification.size() - 2).name,
                .location = id.location
            });
        return nullptr;
    }

    auto value = look_at->getIDInfoFor(id.full_qualification.back().name).value();
    if (value && !lookupDecl(value).is_exported) {
        report_error(
            ErrCode::SYMBOL_NOT_EXPORTED,
            {
                .str_1 = value->toString(),
                .location = id.location
            });
        return nullptr;
    } return value;
}


Namespace* SymbolManager::getGlobalScopeFromModule(const fs::path& path) const {
    return m_ModuleMap.get(path).SymbolTable.getGlobalScope();
}
