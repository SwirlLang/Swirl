#include <parser/Parser.h>

extern ModuleMap_t ModuleMap;

IdentInfo* SymbolManager::getIdInfoFromModule(const std::filesystem::path& mod_path, const std::string& name) {
    return ModuleMap.get(mod_path).SymbolTable.getIdInfoOfAGlobal(name);
}
