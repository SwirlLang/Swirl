#include "parser/Parser.h"
#include "managers/ModuleManager.h"


TableEntry& SymbolManager::lookupDecl(IdentInfo* id) {
    static TableEntry fictitious_table_entry{.is_exported = true};
    if (id->isFictitious()) { return fictitious_table_entry; }
    if (sw::FileHandle* mod_path = id->getModuleFileHandle(); mod_path != m_ModuleHandle) {
        return m_ModuleMap.get(mod_path).SymbolTable.m_IdToTableEntry.at(id);
    } return m_IdToTableEntry.at(id);
}


Type* SymbolManager::lookupType(IdentInfo* id) {
    if (!id) return nullptr;
    if (const auto mod_path = id->getModuleFileHandle(); mod_path != m_ModuleHandle) {
        return m_ModuleMap.get(mod_path).SymbolTable.m_TypeManager.getFor(id);
    } return m_TypeManager.getFor(id);
}


IdentInfo* SymbolManager::getIdInfoFromModule(sw::FileHandle* mod_path, const std::string& name) const {
    return m_ModuleMap.get(mod_path).SymbolTable.getIdInfoOfAGlobal(name, true);
}


IdentInfo* SymbolManager::getIDInfoFor(const Ident& id, const std::optional<ErrorCallback_t>& err_callback) {
    auto report_error = [&err_callback](const ErrCode code, const ErrorContext& ctx) {
        if (err_callback.has_value())
            (*err_callback)(code, ctx);
    };

    if (id.full_qualification.empty()) {
        throw std::runtime_error("SymbolManager::getIDInfoFor: `Ident::full_qualification::size` is 0!");
    }

    if (id.full_qualification.size() == 1) {
        auto glob_id = getIdInfoOfAGlobal(id.full_qualification.front().name);
        return glob_id;
    }

    const Namespace* look_at = nullptr;
    for (const auto& [counter, str] : llvm::enumerate(id.full_qualification)) {
        if (counter == id.full_qualification.size() - 1) break;
        if (counter == 0) {
            const auto qual_id = getIdInfoOfAGlobal(str.name);

            if (!qual_id)
                return nullptr;

            const auto tmp = lookupDecl(qual_id);
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

    const auto value = look_at->getIDInfoFor(id.full_qualification.back().name);

    if (!value.has_value()) {
        report_error(
            ErrCode::NO_SYMBOL_IN_NAMESPACE,
            {
                .str_1 = id.full_qualification.back().name,
                .str_2 = id.full_qualification.at(id.full_qualification.size() - 2).name,
                .location = id.location
            }); return nullptr;
    }

    if (*value && !lookupDecl(*value).is_exported) {
        report_error(
            ErrCode::SYMBOL_NOT_EXPORTED,
            {
                .str_1 = (*value)->toString(),
                .location = id.location
            });
        return nullptr;
    } return *value;
}


Namespace* SymbolManager::getGlobalScopeFromModule(sw::FileHandle* mod) const {
    return m_ModuleMap.get(mod).SymbolTable.getGlobalScope();
}
