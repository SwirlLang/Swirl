#include <parser/Parser.h>

extern ModuleMap_t ModuleMap;

IdentInfo* SymbolManager::getIdInfoFromModule(const std::filesystem::path& mod_path, const std::string& name) {
    return ModuleMap.get(mod_path).SymbolTable.getIdInfoOfAGlobal(name);
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
    throw std::runtime_error("SymbolManager::registerImportedSymbol: deprecated");
}