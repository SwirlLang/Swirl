#include "parser/Parser.h"
#include "modules/ModuleManager.h"


TableEntry& SymbolManager::lookupDecl(IdentInfo* id) {
    static TableEntry fictitious_table_entry{.is_exported = true};
    if (id->isFictitious()) { return fictitious_table_entry; }
    if (sw::FileHandle* mod_path = id->getModuleFileHandle(); mod_path != m_ModuleHandle) {
        return m_ModuleMap.get(mod_path).symbol_table.m_IdToTableEntry.at(id);
    } return m_IdToTableEntry.at(id);
}

TableEntry* SymbolManager::searchDecl(IdentInfo* id) {
    static TableEntry fictitious_table_entry{.is_exported = true};
    if (id->isFictitious()) { return &fictitious_table_entry; }
    if (sw::FileHandle* mod_path = id->getModuleFileHandle(); mod_path != m_ModuleHandle) {
        auto& table = m_ModuleMap.get(mod_path).symbol_table.m_IdToTableEntry;
        return table.contains(id) ? &table[id] : nullptr;
    } return m_IdToTableEntry.contains(id) ? &m_IdToTableEntry[id] : nullptr;
}


Type* SymbolManager::lookupType(IdentInfo* id) {
    assert(id != nullptr);
    if (const auto mod_path = id->getModuleFileHandle(); mod_path != m_ModuleHandle) {
        return m_ModuleMap.get(mod_path).symbol_table.m_TypeManager.getFor(id);
    } return m_TypeManager.getFor(id);
}


IdentInfo* SymbolManager::getIdInfoFromModule(sw::FileHandle* mod_path, const std::string& name) const {
    return m_ModuleMap.get(mod_path).symbol_table.getIdInfoOfAGlobal(name, true);
}


Enum* SymbolManager::getFictitiousIDValue(IdentInfo* id) {
    auto& fictitious_id_table =
        id->getModuleFileHandle() == m_ModuleHandle
            ? m_FictitiousIDTable
            : m_ModuleMap.get(id->getModuleFileHandle()).symbol_table.m_FictitiousIDTable;

    if (fictitious_id_table.contains(id)) {
        return fictitious_id_table[id];
    }

    throw std::runtime_error("SymbolTable::getFictitiousIDValue: id not in the table");
}


IdentInfo* SymbolManager::getIDInfoFor(const Ident& id, const std::optional<ErrorCallback_t>& err_callback) {
    auto report_error = [&err_callback](const ErrCode code, const ErrorContext& ctx) {
        if (err_callback.has_value())
            (*err_callback)(code, ctx);
    };

    assert(!id.full_qualification.empty());

    if (id.full_qualification.size() == 1) {
        return getIdInfoOfAGlobal(std::string(id.full_qualification.front().name));
    }

    // walk the qualifiers (everything but the final segment) to arrive at the
    // namespace the final segment is a member of
    const Namespace* look_at = nullptr;
    Type* owner_type = nullptr;

    for (const auto& [counter, str] : std::views::enumerate(id.full_qualification)) {
        if (counter == id.full_qualification.size() - 1) break;

        if (counter == 0) {
            const auto qual_id = str.value ? str.value : getIdInfoOfAGlobal(std::string(str.name));

            if (!qual_id)
                return nullptr;

            const auto tmp = lookupDecl(qual_id);
            look_at = tmp.scope;
            owner_type = tmp.swirl_type;
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

        // intermediate hops resolve against exactly one namespace
        const auto matches = resolveMember({&look_at, 1}, str.name);
        if (matches.empty()) {
            report_error(
                ErrCode::NO_SYMBOL_IN_NAMESPACE,
                {
                    .str_1 = str.name,
                    .str_2 = id.full_qualification.at(counter - 1).name,
                    .location = id.location
                });
            return nullptr;
        }

        const auto& tmp = lookupDecl(matches[0].id);
        if (matches[0].id->getModuleFileHandle() != m_ModuleHandle && !tmp.is_exported) {
            report_error(
                ErrCode::SYMBOL_NOT_EXPORTED,
                {
                    .str_1 = str.name,
                    .location = id.location
                });
            return nullptr;
        }
        owner_type = tmp.swirl_type;
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

    // final hop: the qualifier's own scope, plus the impl-scopes of the owner
    // type (which make `Type::impl_method` resolve)
    std::vector<const Namespace*> scopes = {look_at};
    std::vector<Module::ImplScopeRef> impl_refs;
    if (owner_type) {
        for (const auto& ref : m_ModuleMap.get(m_ModuleHandle).getImplScopesFor(owner_type)) {
            scopes.push_back(ref.info->scope);
            impl_refs.push_back(ref);
        }
    }

    const auto matches = resolveMember(scopes, id.full_qualification.back().name);

    if (matches.empty()) {
        report_error(
            ErrCode::NO_SYMBOL_IN_NAMESPACE,
            {
                .str_1 = id.full_qualification.back().name,
                .str_2 = id.full_qualification.at(id.full_qualification.size() - 2).name,
                .location = id.location
            });
        return nullptr;
    }

    if (matches.size() > 1) {
        report_error(
            ErrCode::AMBIGUOUS_MEMBER,
            {
                .str_1 = id.full_qualification.back().name,
                .str_2 = id.full_qualification.at(id.full_qualification.size() - 2).name,
                .location = id.location
            });
        return nullptr;
    }

    if (matches[0].found_in == look_at) {
        // regular members are visible within the declaring module; from other
        // modules they require `export`
        if (matches[0].id->getModuleFileHandle() != m_ModuleHandle
            && !lookupDecl(matches[0].id).is_exported)
        {
            report_error(
                ErrCode::SYMBOL_NOT_EXPORTED,
                {
                    .str_1 = matches[0].id->toString(),
                    .location = id.location
                });
            return nullptr;
        }
    } else {
        // impl members are visible within the module that declared the impl;
        // from other modules they require `export impl`
        for (const auto& ref : impl_refs) {
            if (ref.info->scope == matches[0].found_in
                && ref.info->parent_module != &m_ModuleMap.get(m_ModuleHandle)
                && !ref.info->is_exported)
            {
                report_error(
                    ErrCode::PROTO_IMPL_NOT_EXPORTED,
                    {
                        .str_1 = ref.protocol->toString(),
                        .str_2 = owner_type->toString(),
                        .location = id.location
                    });
                return nullptr;
            }
        }
    }

    return matches[0].id;
}


std::vector<SymbolManager::MemberLookup> SymbolManager::resolveMember(
    const std::span<const Namespace*> scopes,
    const std::string_view name)
{
    std::vector<MemberLookup> matches;
    for (const Namespace* scope : scopes) {
        if (!scope) continue;
        if (const auto id = scope->getIDInfoFor(name)) {
            matches.push_back({.id = *id, .found_in = scope});
        }
    }
    return matches;
}


Namespace* SymbolManager::getGlobalScopeFromModule(sw::FileHandle* mod) const {
    return m_ModuleMap.get(mod).symbol_table.getGlobalScope();
}
