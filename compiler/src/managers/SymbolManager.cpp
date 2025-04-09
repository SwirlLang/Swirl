#include <parser/Parser.h>

extern ModuleMap_t ModuleMap;

IdentInfo* SymbolManager::getIdInfoFromModule(const std::filesystem::path& mod_path, const std::string& name) {
    if (m_ImplicitlyIncludedSymbols.contains({name, mod_path}))
        return m_ImportedSyms[name].get().first;
    m_ImplicitlyIncludedSymbols[{name, mod_path}] = ModuleMap.get(mod_path).SymbolTable.subscribeForTableEntry(name);
    return m_ImplicitlyIncludedSymbols[{name, mod_path}].get().first;
}

TableEntry& SymbolManager::lookupDecl(IdentInfo* id) {
    if (const auto mod_path = id->getModulePath(); mod_path != m_ModulePath) {
        return ModuleMap.get(mod_path).SymbolTable.m_IdToTableEntry.at(id);
    } return m_IdToTableEntry.at(id);
}


Type* SymbolManager::lookupType(IdentInfo* id) {
    if (const auto mod_path = id->getModulePath(); mod_path != m_ModulePath) {
        return ModuleMap.get(mod_path).SymbolTable.m_TypeManager.getFor(id);
    } return m_TypeManager.getFor(id);
}

void SymbolManager::registerImportedSymbol(const fs::path& mod_path, const std::string& actual_name, const std::string& alias) {
    if (alias.empty()) {
        if (m_ImportedSyms.contains(actual_name)) {
            ErrMan->newError("Symbol already present!");
            return;
        }

        m_ImportedSyms[actual_name] = ModuleMap.get(mod_path).SymbolTable.subscribeForTableEntry(actual_name);
        return;
    }

    if (m_ImportedSyms.contains(alias)) {
        ErrMan->newError("Symbol already present!");
        return;
    }

    // register the symbol as the alias but subscribe with the actual name
    m_ImportedSyms[alias] = ModuleMap.get(mod_path).SymbolTable.subscribeForTableEntry(actual_name);
}