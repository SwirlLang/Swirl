#pragma once
#include <mutex>
#include <shared_mutex>
#include <string>
#include <ranges>
#include <unordered_map>

#include <definitions.h>    
#include <types/SwTypes.h>
#include <types/TypeManager.h>
#include <managers/IdentManager.h>
#include <llvm/IR/Value.h>


struct TableEntry {
    bool is_type     = false;
    bool is_const    = false;
    bool is_param    = false;
    bool is_volatile = false;
    bool is_exported = false;

    llvm::Value* ptr        = nullptr;
    llvm::Type*  llvm_type  = nullptr;
    Type*        swirl_type = nullptr;
};


class Scope {
    std::unordered_map<IdentInfo*, TableEntry> m_SymT;
    IdentManager m_IDMan;
    std::size_t m_ModuleUID;

public:
    Scope(std::size_t mod_uid): m_ModuleUID(mod_uid) {}

    TableEntry& get(IdentInfo* id) {
        return m_SymT.at(id);
    }

    template <bool add_mod_id = true>
    IdentInfo* getNewIDInfo(const std::string& name) {
        // if constexpr (add_mod_id)
        //     return m_IDMan.createNew(std::to_string(m_ModuleUID) + '@' + name);
        return m_IDMan.createNew(name);
    }

    IdentInfo* registerNew(const std::string& name, const TableEntry& entry) {
        const auto ret = m_IDMan.createNew(name);
        m_SymT[ret] = entry;
        return ret;
    }

    std::optional<IdentInfo*> getIDInfoFor(const std::string& name) const {
        return m_IDMan.contains(name) ? std::optional{m_IDMan.fetch(name)} : std::nullopt;
    }

    bool contains(IdentInfo* id) const {
        return m_SymT.contains(id);
    }
};


class SymbolManager {
    using Module_t = std::vector<Scope>;
     
    bool m_LockEmplace = false;
    std::size_t m_ScopeInt = 0;
    
    inline static IdentManager m_BuiltinIDMan; // specifically for built-in symbols

    inline static TypeManager m_TypeManager;
    
    inline static std::vector<Module_t> m_TypeTable;
    inline static std::vector<Module_t> m_DeclTable;

    inline static std::recursive_mutex m_SymMutex;
    inline static bool m_InstanceExists = false;

public:
    std::size_t ModuleUID = 0;
    SymbolManager() {
        std::lock_guard guard(m_SymMutex);

        // global scope
        m_TypeTable.emplace_back();
        m_TypeTable.front().emplace_back(ModuleUID);

        m_DeclTable.emplace_back();
        m_DeclTable.front().emplace_back(ModuleUID);

        if (!m_InstanceExists) {
            m_InstanceExists = true;

            // register built-in types in the global scope
            registerType(m_BuiltinIDMan.createNew("i8"),   new TypeI8{});
            registerType(m_BuiltinIDMan.createNew("i16"),  new TypeI16{});
            registerType(m_BuiltinIDMan.createNew("i32"),  new TypeI32{});
            registerType(m_BuiltinIDMan.createNew("i64"),  new TypeI64{});
            registerType(m_BuiltinIDMan.createNew("i128"), new TypeI128{});

            registerType(m_BuiltinIDMan.createNew("u8"),   new TypeU8{});
            registerType(m_BuiltinIDMan.createNew("u16"),  new TypeU16{});
            registerType(m_BuiltinIDMan.createNew("u32"),  new TypeU32{});
            registerType(m_BuiltinIDMan.createNew("u64"),  new TypeU64{});
            registerType(m_BuiltinIDMan.createNew("u128"), new TypeU128{});

            registerType(m_BuiltinIDMan.createNew("f32"),  new TypeF32{});
            registerType(m_BuiltinIDMan.createNew("f64"),  new TypeF64{});
            registerType(m_BuiltinIDMan.createNew("bool"), new TypeBool{});
        }
    }

    TableEntry& lookupDecl(IdentInfo* id) {
        std::lock_guard guard(m_SymMutex);
        return lookup(id, m_DeclTable[ModuleUID]);
    }

    Type* lookupType(IdentInfo* id) {
        std::lock_guard guard(m_SymMutex);
        return m_TypeManager.getFor({.ident = id});
    }

    Type* lookupType(const std::string& id) {
        std::lock_guard guard(m_SymMutex);
        return m_TypeManager.getFor({.ident = getIDInfoFor(id)});
    }

    void registerType(IdentInfo* id, Type* type) {
        std::lock_guard guard(m_SymMutex);
        m_TypeManager.registerType({.ident = id}, type);
    }

    IdentInfo* registerDecl(const std::string& name, const TableEntry& entry) {
        std::lock_guard guard(m_SymMutex);
        return m_DeclTable[ModuleUID].at(m_ScopeInt).registerNew(name, entry);
    }

    bool typeExists(IdentInfo* id) const {
        std::lock_guard guard(m_SymMutex);
        return checkExistence(id, m_TypeTable[ModuleUID]);
    }

    bool declExists(IdentInfo* id) const {
        std::lock_guard guard(m_SymMutex);
        return checkExistence(id, m_DeclTable[ModuleUID]);
    }

    template <bool create_new = false>
    IdentInfo* getIDInfoFor(const std::string& id)  {
        std::lock_guard guard(m_SymMutex);

        if (m_BuiltinIDMan.contains(id))
            return m_BuiltinIDMan.fetch(id);

        if constexpr (!create_new) {
            for (const auto& [decls, type] : std::views::zip(m_DeclTable[ModuleUID], m_TypeTable[ModuleUID]) | std::views::take(m_ScopeInt + 1)) {
                if (auto decl_id = decls.getIDInfoFor(id); decl_id.has_value())
                    return decl_id.value();
                if (auto type_id = type.getIDInfoFor(id); type_id.has_value())
                    return type_id.value();
            } throw std::runtime_error("SymbolManager::getIDInfoFor: not found for: " + id);
        } return m_TypeTable[ModuleUID].at(m_ScopeInt).getNewIDInfo(id);
    }

    void newScope() {
        std::lock_guard guard(m_SymMutex);

        m_ScopeInt++;
        if (!m_LockEmplace) {
            m_DeclTable[ModuleUID].emplace_back(ModuleUID);
            m_TypeTable[ModuleUID].emplace_back(ModuleUID);
        }
    }

    void lockNewScpEmplace() {
        m_LockEmplace = true;
    }

    void unlockNewScpEmplace() {
        m_LockEmplace = false;
    }

    void destroyLastScope() {
        std::lock_guard guard(m_SymMutex);
        m_ScopeInt--;
    }

private:
    TableEntry& lookup(IdentInfo* id, std::vector<Scope>& at) {
        if (at.front().contains(id))
            return at.front().get(id);

        // traverse in reverse
        for (auto& scope : at | std::views::drop(1) | std::views::take(m_ScopeInt) | std::views::reverse)
            if (scope.contains(id))
                return scope.get(id);

        throw std::runtime_error("SymbolManager::lookup: id not found");
    }

    bool checkExistence(IdentInfo* id, const std::vector<Scope>& at) const {
        if (at.front().contains(id))
            return true;

        return std::ranges::any_of(
            at | std::views::drop(1) | std::views::take(m_ScopeInt) | std::views::reverse,
            [id](auto& scope) {
                return scope.contains(id);
            }
        );
    }
};