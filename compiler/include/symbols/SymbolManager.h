#pragma once
#include <span>
#include <list>
#include <string>
#include <ranges>
#include <utility>
#include <filesystem>
#include <unordered_map>

#include "metadata.h"

#include "types/definitions.h"
#include "symbols/IdentManager.h"
#include "errors/ErrorManager.h"


namespace sw { class TypeManager; }

struct ErrorContext;
class  ModuleManager;
struct Module;
using ErrorCallback_t = std::function<void (ErrCode, ErrorContext)>;


class Namespace {
    IdentManager m_IDMan;

public:
    explicit Namespace(sw::FileHandle* mod_handle): m_IDMan(mod_handle) {}

    IdentInfo* getNewIDInfo(const std::string& name, const bool is_fictitious = false) {
        return m_IDMan.createNew(name, is_fictitious);
    }

    // NOTE: to be removed
    IdentInfo* getNewIDInfo(const std::string_view name, const bool is_fictitious = false) {
        return getNewIDInfo(std::string(name), is_fictitious);
    }

    auto begin() const {
        return m_IDMan.begin();
    }

    auto end() const {
        return m_IDMan.end();
    }

    const sw::FileHandle* getModuleFileHandle() const {
        return m_IDMan.getModuleFileHandle();
    }

    constexpr std::optional<IdentInfo*> getIDInfoFor(const std::string& name) const {
        return m_IDMan.contains(name) ? std::optional{m_IDMan.fetch(name)} : std::nullopt;
    }

    // NOTE: to be removed
    constexpr std::optional<IdentInfo*> getIDInfoFor(const std::string_view name) const {
        return getIDInfoFor(std::string(name));
    }
};


class SymbolManager {
    ModuleManager* m_ModuleMap{};

    std::list<Namespace>    m_Scopes;       // for the stable-addressing of the namespaces
    std::vector<Namespace*> m_ScopeTrack;  // for tracking the insert-points

    std::unordered_map<IdentInfo*, TableEntry> m_IdToTableEntry;

    std::filesystem::path m_ModulePath;
    std::unordered_map<std::string, IdentInfo*> m_ImportedSymIDTable;

    // tracks the exported symbols of the mod
    std::unordered_map<std::string, ExportedSymbolMeta_t> m_ExportedSymbolTable;

    // maps qualifier-names to their namespace
    std::unordered_map<std::string, Namespace*> m_QualifierTable;

    // maps fictitious IDs to parent enum nodes
    std::unordered_map<IdentInfo*, Enum*> m_FictitiousIDTable;

    ErrorCallback_t m_ErrorCallback;
    sw::FileHandle* m_ModuleHandle{};

    friend class sw::TypeManager;

public:
    inline static const std::unordered_map<Intrinsic::Kind, IntrinsicDef> IntrinsicTable = {
        {Intrinsic::TYPEOF,  IntrinsicDef{}},
        {Intrinsic::SIZEOF,  IntrinsicDef{.return_type = &GlobalTypeI64}},
        {Intrinsic::MEMCPY,  IntrinsicDef{.return_type = &GlobalTypeVoid}},
        {Intrinsic::MEMSET,  IntrinsicDef{.return_type = &GlobalTypeVoid}},
        {Intrinsic::ADV_PTR, IntrinsicDef{}}
    };

    static std::unordered_map<Type*, std::function<void(Namespace*, SymbolManager&)>> DefaultTypeMethods;


    explicit SymbolManager(const Module*);

    TableEntry& lookupDecl(IdentInfo* id);
    TableEntry* searchDecl(IdentInfo* id);

    /// returns the IdentInfo* of a global name from the module `mod_handle`
    IdentInfo* getIdInfoFromModule(sw::FileHandle* mod_path, const std::string& name) const;

    IdentInfo* getIDInfoFor(const Ident& id, const std::optional<ErrorCallback_t>& err_callback = std::nullopt);


    struct MemberLookup {
        IdentInfo* id = nullptr;
        const Namespace* found_in = nullptr;
    };

    /// Resolves `name` as a member of the given candidate namespaces, in order.
    /// Returns ALL matches so the caller can detect ambiguity.
    std::vector<MemberLookup> resolveMember(std::span<const Namespace*> scopes, std::string_view name);

    Enum* getFictitiousIDValue(IdentInfo* id);


    /// returns the `IdentInfo*` of a global symbol.
    IdentInfo* getIdInfoOfAGlobal(const std::string& name, bool enforce_export = false, bool report_error = true) {
        if (const auto id = m_Scopes.front().getIDInfoFor(name))
            return *id;

        // when this flag is true, look only in the exported ids rather than every foreign id
        if (!enforce_export) {
            if (m_ImportedSymIDTable.contains(name))
                return m_ImportedSymIDTable[name];
        } else if (m_ExportedSymbolTable.contains(name))
            return m_ExportedSymbolTable[name].id;

        if (report_error) {
            m_ErrorCallback(ErrCode::QUALIFIER_UNDEFINED, {.str_1 = name});
        } return nullptr;
    }


    IdentInfo* getIDInfoFor(const std::string& id) {
        if (const auto ret = getIdInfoOfAGlobal(id)) {
            return ret;
        }

        for (const Namespace* scope : m_ScopeTrack | std::views::reverse) {
            if (const auto ret = scope->getIDInfoFor(id)) {
                return *ret;
            }
        } return nullptr;
    }


    /// returns the global scope's pointer
    Namespace* getGlobalScope() const {
        return m_ScopeTrack.front();
    }


    /// fetches the global scope of the module
    Namespace* getGlobalScopeFromModule(sw::FileHandle* path) const;


    /// makes the symbol manager aware of the IDs of foreign (imported) symbols
    void registerForeignID(const std::string& name, IdentInfo* id, const bool is_exported = false) {
        m_ImportedSymIDTable.emplace(name, id);
        if (is_exported)
            registerExportedSymbol(name, {.id = id});
    }

    /// Used to register a declaration, if `scope_index` is passed, registers the declaration at that scope rather than
    /// the one at the top.
    IdentInfo* registerDecl(const std::string& name, const TableEntry& entry, std::optional<std::size_t> scope_index = std::nullopt) {
        IdentInfo* id;
        if (scope_index.has_value())
            id = m_ScopeTrack.at(*scope_index)->getNewIDInfo(name);
        else id = m_ScopeTrack.back()->getNewIDInfo(name);

        if (m_IdToTableEntry.contains(id)) {
            m_ErrorCallback(ErrCode::SYMBOL_ALREADY_EXISTS, {.str_1 = name});
            return nullptr;
        } m_IdToTableEntry.insert({id, entry});

        if (entry.is_exported) {
            registerExportedSymbol(name, {.id = id});
        } return id;
    }


    IdentInfo* registerDecl(IdentInfo* id, TableEntry& entry) {
        if (m_IdToTableEntry.contains(id)) {
            return nullptr;
        } m_IdToTableEntry.insert({id, entry});
        return id;
    }


    void registerDecl(IdentInfo* id, const TableEntry& entry) {
        if (m_IdToTableEntry.contains(id))
            return;
        m_IdToTableEntry.insert({id, entry});
    }

    void registerFictitiousID(IdentInfo* id, Enum* enum_node) {
        if (m_FictitiousIDTable.contains(id)) {
            throw std::runtime_error("SymbolTable::registerFictitiousIDValue: id already in the table");
        } m_FictitiousIDTable.insert({id, enum_node});
    }


    bool isForeignID(const IdentInfo* id) const {
        if (id->getModuleFileHandle() == m_ModuleHandle) {
            return true;
        } return false;
    }


    bool declExists(IdentInfo* id) const {
        return m_IdToTableEntry.contains(id);
    }


    Namespace* newScope() {
        Namespace* ret = &m_Scopes.emplace_back(m_ModuleHandle);
        m_ScopeTrack.push_back(ret);
        return ret;
    }


    void setErrorCallback(const ErrorCallback_t& err_callback) {
        m_ErrorCallback = err_callback;
    }

    ErrorCallback_t getErrorCallback() const {
        return m_ErrorCallback;
    }


private:
    void registerExportedSymbol(const std::string& name, const ExportedSymbolMeta_t& meta) {
        m_ExportedSymbolTable.insert(std::make_pair(name, meta));
    }
};