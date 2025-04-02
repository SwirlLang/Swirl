#pragma once
#include "utils/utils.h"
#include <filesystem>
#include <future>
#include <string>
#include <ranges>
#include <unordered_map>

#include <types/SwTypes.h>
#include <types/TypeManager.h>
#include <managers/IdentManager.h>
#include <llvm/IR/Value.h>
#include <managers/ErrorManager.h>


struct TableEntry {
    bool is_type     = false;
    bool is_const    = false;
    bool is_param    = false;
    bool is_volatile = false;

    llvm::Value* ptr        = nullptr;
    llvm::Type*  llvm_type  = nullptr;
    Type*        swirl_type = nullptr;
};


class Scope {
    IdentManager m_IDMan;

public:
    Scope() = default;

    IdentInfo* getNewIDInfo(const std::string& name) {
        return m_IDMan.createNew(name);
    }
    
    std::optional<IdentInfo*> getIDInfoFor(const std::string& name) const {
        return m_IDMan.contains(name) ? std::optional{m_IDMan.fetch(name)} : std::nullopt;
    }
};


class SymbolManager {
    bool m_LockEmplace = false;
    std::size_t m_ScopeInt = 0;

    TypeManager m_TypeManager;
    std::vector<Scope> m_IdScopes;

    std::unordered_map<IdentInfo*, TableEntry> m_IdToTableEntry;

    // for mapping the modules' aliases to their file paths
    std::unordered_map<std::string, std::filesystem::path> m_ModuleAliasTable;

    std::size_t m_ModuleUID;

    std::unordered_map<
        std::string,
        std::vector<
            std::promise<std::pair<IdentInfo*, TableEntry*>
                >>> m_TESubscribers; // TE = TableEntry

public:
    ErrorManager* ErrMan = nullptr;

    explicit SymbolManager(std::size_t uid): m_ModuleUID{uid} {
        // global scope
        m_IdScopes.emplace_back();

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

    TableEntry& lookupDecl(IdentInfo* id) {
        return m_IdToTableEntry.at(id);
    }

    Type* lookupType(IdentInfo* id) {
        return m_TypeManager.getFor(id);
    }

    /// returns the IdentInfo* of a global
    IdentInfo* getIdInfoOfAGlobal(const std::string& name) const {
        if (const auto id = m_IdScopes.front().getIDInfoFor(name))
            return *id;
        return nullptr;
    }

    /// returns the IdentInfo* of a global name from the module `mod_path`
    static IdentInfo* getIdInfoFromModule(const std::filesystem::path& mod_path, const std::string& name) ;

    Type* lookupType(const std::string& id) {
        return m_TypeManager.getFor(getIDInfoFor(id));
    }

    void registerType(IdentInfo* id, Type* type) {
        m_TypeManager.registerType(id, type);
    }

    Type* getReferenceType(Type* of_type) {
        return m_TypeManager.getReferenceType(of_type);
    }


    Type* getPointerType(Type* of_type, const uint16_t ptr_level) {
        return m_TypeManager.getPointerType(of_type, ptr_level);
    }

    IdentInfo* registerDecl(const std::string& name, const TableEntry& entry) {
        auto id = m_IdScopes.at(m_ScopeInt).getNewIDInfo(name);
        m_IdToTableEntry.insert({id, entry});

        if (m_TESubscribers.contains(name)) {
            for (auto& subscriber : m_TESubscribers[name]) {
                subscriber.set_value({id, &m_IdToTableEntry.at(id)});
            }
        } m_TESubscribers.erase(name);
        return id;
    }

    bool typeExists(IdentInfo* id) {
        return m_TypeManager.contains(id);
    }

    bool declExists(IdentInfo* id) const {
        return m_IdToTableEntry.contains(id);
    }


    std::future<std::pair<IdentInfo*, TableEntry*>> subscribeForTableEntry(const std::string& id) {
        static std::mutex function_guard;

        auto bound = [this, &id] {
            fulfillPromisesIfExists(id);
        };

        LockGuard_t _{function_guard, bound};
        
        if (m_TESubscribers.contains(id)) {
            m_TESubscribers[id].emplace_back();
            return m_TESubscribers[id].back().get_future();
        } 

        m_TESubscribers.insert({id, {}});
        m_TESubscribers[id].emplace_back();
        return m_TESubscribers[id].back().get_future();
    }


    template <bool create_new = false>
    IdentInfo* getIDInfoFor(const std::string& id) {
        if constexpr (!create_new) {
            for (const auto& scope : m_IdScopes | std::views::take(m_ScopeInt + 1)) {
                if (const auto decl_id = scope.getIDInfoFor(id))
                    return decl_id.value();
            } return nullptr;
        } return m_IdScopes.at(m_ScopeInt).getNewIDInfo(id);
    }


    void registerModuleAlias(std::string_view name, const fs::path& path) {
        m_ModuleAliasTable[std::string(name)] = path;
    }

    fs::path& getModuleFromAlias(std::string_view name) {
        return m_ModuleAliasTable.at(std::string(name));
    }


    void newScope() {
        m_ScopeInt++;

        if (m_LockEmplace)
            return;
        
        m_IdScopes.emplace_back();
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


private:
    void fulfillPromisesIfExists(const std::string& name) {
        if (auto id = m_IdScopes.front().getIDInfoFor(name)) {
            for (auto& subscriber : m_TESubscribers[name]) {
                subscriber.set_value({id.value(), &m_IdToTableEntry.at(id.value())});
            } m_TESubscribers.erase(name);
        }
    }
};