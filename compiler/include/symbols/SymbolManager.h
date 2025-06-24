#pragma once
#include <filesystem>
#include <string>
#include <ranges>
#include <unordered_map>
#include <utility>

#include "metadata.h"

#include <types/SwTypes.h>
#include <types/TypeManager.h>
#include <symbols/IdentManager.h>
#include <llvm/IR/Value.h>


class ModuleManager;


class Scope {
    IdentManager m_IDMan;

public:
    explicit Scope(std::filesystem::path mod_path): m_IDMan(std::move(mod_path)) {}

    IdentInfo* getNewIDInfo(const std::string& name) {
        return m_IDMan.createNew(name);
    }
    
    constexpr std::optional<IdentInfo*> getIDInfoFor(const std::string& name) {
        return m_IDMan.contains(name) ? std::optional{m_IDMan.fetch(name)} : std::nullopt;
    }
};


class SymbolManager {
    bool m_LockEmplace = false;
    std::size_t m_ScopeInt = 0;

    TypeManager m_TypeManager;
    ModuleManager& m_ModuleMap;
    std::vector<Scope> m_IdScopes;

    std::unordered_map<IdentInfo*, TableEntry> m_IdToTableEntry;

    std::filesystem::path m_ModulePath;
    std::unordered_map<std::string, IdentInfo*> m_ImportedSymsIDTable;

    // tracks the exported symbols of the mod
    std::unordered_map<std::string, ExportedSymbolMeta_t> m_ExportedSymbolTable;

    // maps qualifier-names to their paths
    std::unordered_map<std::string, fs::path> m_QualifierTable;


public:
    explicit SymbolManager(std::filesystem::path uid, ModuleManager& module_man): m_ModuleMap(module_man), m_ModulePath{std::move(uid)} {

        // global scope
        m_IdScopes.emplace_back(m_ModulePath);

        // register built-in types in the global scope
        registerType(m_IdScopes.front().getNewIDInfo("i8"),   &GlobalTypeI8);
        registerType(m_IdScopes.front().getNewIDInfo("i16"),  &GlobalTypeI16);
        registerType(m_IdScopes.front().getNewIDInfo("i32"),  &GlobalTypeI32);
        registerType(m_IdScopes.front().getNewIDInfo("i64"),  &GlobalTypeI64);
        registerType(m_IdScopes.front().getNewIDInfo("i128"), &GlobalTypeI128);

        registerType(m_IdScopes.front().getNewIDInfo("u8"),   &GlobalTypeU8);
        registerType(m_IdScopes.front().getNewIDInfo("u16"),  &GlobalTypeU16);
        registerType(m_IdScopes.front().getNewIDInfo("u32"),  &GlobalTypeU32);
        registerType(m_IdScopes.front().getNewIDInfo("u64"),  &GlobalTypeU64);
        registerType(m_IdScopes.front().getNewIDInfo("u128"), &GlobalTypeU128);

        registerType(m_IdScopes.front().getNewIDInfo("f32"),  &GlobalTypeF32);
        registerType(m_IdScopes.front().getNewIDInfo("f64"),  &GlobalTypeF64);

        registerType(m_IdScopes.front().getNewIDInfo("bool"), &GlobalTypeBool);
        registerType(m_IdScopes.front().getNewIDInfo("str"),  &GlobalTypeStr);
    }

    TableEntry& lookupDecl(IdentInfo* id);

    Type* lookupType(IdentInfo* id);

    /// returns the `IdentInfo*` of a global symbol
    IdentInfo* getIdInfoOfAGlobal(const std::string& name, bool enforce_export = false) {
        if (const auto id = m_IdScopes.front().getIDInfoFor(name))
            return *id;

        // when this flag is true, look only in the exported id's rather than every foreign id
        if (!enforce_export) {
            if (m_ImportedSymsIDTable.contains(name))
                return m_ImportedSymsIDTable[name];
        } else if (m_ExportedSymbolTable.contains(name))
            return m_ExportedSymbolTable[name].id;
        return nullptr;
    }

    /// returns the IdentInfo* of a global name from the module `mod_path`
    IdentInfo* getIdInfoFromModule(const std::filesystem::path& mod_path, const std::string& name) const;


    Type* lookupType(const std::string& id) {
        return m_TypeManager.getFor(getIDInfoFor(id));
    }

    void registerType(IdentInfo* id, Type* type) {
        m_TypeManager.registerType(id, type);
    }

    /// makes the symbol manager aware of the IDs of foreign (imported) symbols
    void registerForeignID(const std::string& name, IdentInfo* id, const bool is_exported = false) {
        m_ImportedSymsIDTable.emplace(name, id);
        if (is_exported)
            registerExportedSymbol(name, {.id = id});
    }

    void registerExportedSymbol(const std::string& name, const ExportedSymbolMeta_t& meta) {
        m_ExportedSymbolTable.insert(std::make_pair(name, meta));
    }


    void registerQualifier(const std::string& name, const fs::path& path, const bool is_exported = false) {
        m_QualifierTable.insert(std::make_pair(name, path));
        if (is_exported)
            registerExportedSymbol(name, {.path = path});
    }

    std::optional<fs::path> getQualifier(const std::string& name, const bool enforce_export = false) {
        if (!enforce_export) {
            if (m_QualifierTable.contains(name)) {
                return m_QualifierTable[name];
            }
        } else if (m_ExportedSymbolTable.contains(name)) {
            auto sym = m_ExportedSymbolTable[name];
            if (!sym.path.empty()) {
                return sym.path;
            }
        } return std::nullopt;
    }

    std::optional<ExportedSymbolMeta_t> getExportedSymbolMeta(const std::string& name) {
        if (m_ExportedSymbolTable.contains(name))
            return { m_ExportedSymbolTable[name] };
        return std::nullopt;
    }

    Type* getReferenceType(Type* of_type) {
        return m_TypeManager.getReferenceType(of_type);
    }

    Type* getArrayType(Type* of_type, const std::size_t size) {
        return m_TypeManager.getArrayType(of_type, size);
    }

    Type* getPointerType(Type* of_type, const uint16_t ptr_level) {
        return m_TypeManager.getPointerType(of_type, ptr_level);
    }

    IdentInfo* registerDecl(const std::string& name, const TableEntry& entry) {
        auto id = m_IdScopes.at(m_ScopeInt).getNewIDInfo(name);
        if (m_IdToTableEntry.contains(id))
            throw std::runtime_error("SymbolManager::registerDecl: duplicate declaration");
        m_IdToTableEntry.insert({id, entry});

        if (entry.is_exported) {
            registerExportedSymbol(name, {.id = id});
        } return id;
    }

    void registerDecl(IdentInfo* id, const TableEntry& entry) {
        if (m_IdToTableEntry.contains(id))
            throw std::runtime_error("SymbolManager::registerDecl: duplicate declaration");
        m_IdToTableEntry.insert({id, entry});
    }

    bool typeExists(IdentInfo* id) const {
        return m_TypeManager.contains(id);
    }

    bool declExists(IdentInfo* id) const {
        return m_IdToTableEntry.contains(id);
    }


    template <bool create_new = false>
    IdentInfo* getIDInfoFor(const std::string& id) {
        if constexpr (!create_new) {
            for (auto& scope : m_IdScopes | std::views::take(m_ScopeInt + 1)) {
                if (const auto decl_id = scope.getIDInfoFor(id))
                    return decl_id.value();
            } return nullptr;
        } return m_IdScopes.at(m_ScopeInt).getNewIDInfo(id);
    }


    void newScope() {
        m_ScopeInt++;

        if (m_LockEmplace)
            return;
        
        m_IdScopes.emplace_back(m_ModulePath);
    }

    void lockNewScpEmplace() {
        m_LockEmplace = true;
    }

    void unlockNewScpEmplace() {
        m_LockEmplace = false;
    }

    void destroyLastScope() {
        m_ScopeInt--;
    }
};