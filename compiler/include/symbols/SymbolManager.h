#pragma once
#include <filesystem>
#include <string>
#include <ranges>
#include <unordered_map>
#include <utility>

#include "metadata.h"

#include <types/definitions.h>
#include <types/TypeManager.h>
#include <symbols/IdentManager.h>
#include <llvm/IR/Value.h>


enum class ErrCode;
struct ErrorContext;
class ModuleManager;
using ErrorCallback_t = std::function<void (ErrCode, ErrorContext)>;


class Scope {
    IdentManager m_IDMan;

public:
    explicit Scope(std::filesystem::path mod_path): m_IDMan(std::move(mod_path)) {}

    IdentInfo* getNewIDInfo(const std::string& name) {
        return m_IDMan.createNew(name);
    }

    auto begin() const {
        return m_IDMan.begin();
    }

    auto end() const {
        return m_IDMan.end();
    }

    const fs::path& getModPath() const {
        return m_IDMan.getModulePath();
    }

    constexpr std::optional<IdentInfo*> getIDInfoFor(const std::string& name) const {
        return m_IDMan.contains(name) ? std::optional{m_IDMan.fetch(name)} : std::nullopt;
    }
};


class SymbolManager {
    bool m_LockEmplace = false;
    std::size_t m_ScopeInt = 0;

    TypeManager m_TypeManager;
    ModuleManager& m_ModuleMap;

    std::list<Scope>    m_Scopes;       // for stable-addressing of the scopes
    std::vector<Scope*> m_ScopeTrack;  // for tracking the insert-points

    std::unordered_map<IdentInfo*, TableEntry> m_IdToTableEntry;

    std::filesystem::path m_ModulePath;
    std::unordered_map<std::string, IdentInfo*> m_ImportedSymsIDTable;

    // tracks the exported symbols of the mod
    std::unordered_map<std::string, ExportedSymbolMeta_t> m_ExportedSymbolTable;

    // maps qualifier-names to their paths
    std::unordered_map<std::string, Scope*> m_QualifierTable;


public:
     explicit SymbolManager(std::filesystem::path uid, ModuleManager& module_man)
        : m_ModuleMap(module_man)
        , m_ModulePath(std::move(uid))
    {
        // Create the global scope
        m_ScopeTrack.push_back(&m_Scopes.emplace_back(m_ModulePath));

        // Register all built-in types in the global scope
        for (const auto [str, type] : BuiltinTypes) {
            const auto id = m_ScopeTrack.back()->getNewIDInfo(std::string(str));
            registerType(id, type);
        }
    }

    TableEntry& lookupDecl(IdentInfo* id);

    Type* lookupType(IdentInfo* id);

    /// returns the `IdentInfo*` of a global symbol
    IdentInfo* getIdInfoOfAGlobal(const std::string& name, const bool enforce_export = false) {
        if (const auto id = m_Scopes.front().getIDInfoFor(name))
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

    IdentInfo* getIDInfoFor(const std::string& id) {
        for (Scope* scope : m_ScopeTrack | std::views::reverse) {
            if (const auto ret = scope->getIDInfoFor(id)) {
                return *ret;
            }
        } return nullptr;
    }

    IdentInfo* getIDInfoFor(const Ident& id, const std::optional<ErrorCallback_t>& err_callback = std::nullopt);


    /// returns the global scope's pointer
    Scope* getGlobalScope() const {
        return m_ScopeTrack.front();
    }

    /// fetches the global scope of the module mapped to `path`
    Scope* getGlobalScopeFromModule(const fs::path& path) const;


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

    Type* getStrType(const std::size_t size) {
        return m_TypeManager.getStringType(size);
    }

    Type* getPointerType(Type* of_type, const uint16_t ptr_level) {
        return m_TypeManager.getPointerType(of_type, ptr_level);
    }


    /// Used to register a declaration, if `scope_index` is passed, registers the declaration at that scope rather than
    /// the one at the top.
    IdentInfo* registerDecl(const std::string& name, const TableEntry& entry, std::optional<std::size_t> scope_index = std::nullopt) {
        IdentInfo* id;
        if (scope_index.has_value())
            id = m_ScopeTrack.at(*scope_index)->getNewIDInfo(name);
        else id = m_ScopeTrack.back()->getNewIDInfo(name);

        if (m_IdToTableEntry.contains(id))
            throw std::runtime_error("SymbolManager::registerDecl: duplicate declaration");
        m_IdToTableEntry.insert({id, entry});

        if (entry.is_exported) {
            registerExportedSymbol(name, {.id = id});
        } return id;
    }

    void registerDecl(IdentInfo* id, const TableEntry& entry) {
        if (m_IdToTableEntry.contains(id))
            throw std::runtime_error(
                "SymbolManager::registerDecl: duplicate declaration of "
                "'" + id->toString() + "'");
        m_IdToTableEntry.insert({id, entry});
    }

    bool typeExists(IdentInfo* id) const {
        return m_TypeManager.contains(id);
    }

    bool declExists(IdentInfo* id) const {
        return m_IdToTableEntry.contains(id);
    }

    Scope* newScope() {
        Scope* ret = &m_Scopes.emplace_back(m_ModulePath);
        m_ScopeTrack.push_back(ret);
        return ret;
    }

    void lockNewScpEmplace() {
        m_LockEmplace = true;
    }

    void unlockNewScpEmplace() {
        m_LockEmplace = false;
    }

    void moveToPreviousScope() {
        m_ScopeTrack.pop_back();
    }


private:
    void registerExportedSymbol(const std::string& name, const ExportedSymbolMeta_t& meta) {
        m_ExportedSymbolTable.insert(std::make_pair(name, meta));
    }
};