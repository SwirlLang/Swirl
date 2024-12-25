#pragma once
#include <string>
#include <ranges>
#include <unordered_map>

#include <definitions.h>
#include <types/SwTypes.h>
#include <types/TypeManager.h>
#include <managers/IdentManager.h>


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
    std::unordered_map<IdentInfo*, TableEntry> m_SymT;
    IdentManager m_IDMan;

public:
    Scope() = default;

    TableEntry& get(IdentInfo* id) {
        return m_SymT.at(id);
    }

    IdentInfo* getNewIDInfo(const std::string& name) {
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
    bool m_LockEmplace = false;
    std::size_t m_ScopeInt = 0;

    TypeManager m_TypeManager;
    std::vector<Scope> m_TypeTable;
    std::vector<Scope> m_DeclTable;

public:
    SymbolManager() {
        // global scope
        m_TypeTable.emplace_back();
        m_DeclTable.emplace_back();

        // register default types in the global scope
        registerType(m_TypeTable.front().getNewIDInfo("i8"),   new TypeI8{});
        registerType(m_TypeTable.front().getNewIDInfo("i16"),  new TypeI16{});
        registerType(m_TypeTable.front().getNewIDInfo("i32"),  new TypeI32{});
        registerType(m_TypeTable.front().getNewIDInfo("i64"),  new TypeI64{});
        registerType(m_TypeTable.front().getNewIDInfo("i128"), new TypeI128{});

        registerType(m_TypeTable.front().getNewIDInfo("f32"),  new TypeF32{});
        registerType(m_TypeTable.front().getNewIDInfo("f64"),  new TypeF64{});
    }

    TableEntry& lookupDecl(IdentInfo* id) {
        return lookup(id, m_DeclTable);
    }

    Type* lookupType(IdentInfo* id) {
        return m_TypeManager.getFor({.ident = id});
    }

    Type* lookupType(const std::string& id) {
        return m_TypeManager.getFor({.ident = getIDInfoFor(id)});
    }

    void registerType(IdentInfo* id, Type* type) {
        m_TypeManager.registerType({.ident = id}, type);
    }

    IdentInfo* registerDecl(const std::string& name, const TableEntry& entry) {
        return m_DeclTable.at(m_ScopeInt).registerNew(name, entry);
    }

    bool typeExists(IdentInfo* id) const {
        return checkExistence(id, m_TypeTable);
    }

    bool declExists(IdentInfo* id) const {
        return checkExistence(id, m_DeclTable);
    }

    template <bool create_new = false>
    IdentInfo* getIDInfoFor(const std::string& id) {
        if constexpr (!create_new) {
            for (const auto& [decls, type] : std::views::zip(m_DeclTable, m_TypeTable) | std::views::take(m_ScopeInt + 1)) {
                if (auto decl_id = decls.getIDInfoFor(id); decl_id.has_value())
                    return decl_id.value();
                if (auto type_id = type.getIDInfoFor(id); type_id.has_value())
                    return type_id.value();
            } throw std::runtime_error("SymbolManager::getIDInfoFor: No decl found");
        } return m_TypeTable.at(m_ScopeInt).getNewIDInfo(id);
    }

    void newScope() {
        m_ScopeInt++;
        if (!m_LockEmplace) {
            m_DeclTable.emplace_back();
            m_TypeTable.emplace_back();
        }
    }

    void lockNewScpEmplace() {
        m_LockEmplace = true;
    }

    void destroyLastScope() {
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