#pragma once
#include <list>
#include <string>
#include <utility>
#include <unordered_map>

#include "utils/FileSystem.h"

class IdentInfo {
    std::string id;
    sw::FileHandle* handle = nullptr;

    friend class IdentManager;

public:
    IdentInfo() = delete;

    explicit IdentInfo(std::string ident, sw::FileHandle* mod_handle)
        : id(std::move(ident))
        , handle(mod_handle) {}

    [[nodiscard]]
    const std::string& toString() const {
        return id;
    }

    [[nodiscard]]
    sw::FileHandle* getModuleFileHandle() const {
        return handle;
    }
};


class IdentManager {
    std::unordered_map<std::string, std::unique_ptr<IdentInfo>> m_IdentTable;
    sw::FileHandle* m_ModuleHandle{};

    friend class SymbolManager;

public:
    explicit IdentManager(sw::FileHandle* mod_handle): m_ModuleHandle(mod_handle) {}

    /// registers a new IdentInfo and returns its pointer
    IdentInfo* createNew(const std::string& id) {
        m_IdentTable.emplace(id, new IdentInfo(id, m_ModuleHandle));
        return m_IdentTable.at(id).get();
    }

    /// fetches `id`
    IdentInfo* fetch(const std::string& id) const {
        return m_IdentTable.at(id).get();
    }

    bool contains(const std::string& id) const {
        return m_IdentTable.contains(id);
    }

    const sw::FileHandle* getModuleFileHandle() const {
        return m_ModuleHandle;
    }

    auto begin() const {
        return m_IdentTable.begin();
    }

    auto end() const  {
        return m_IdentTable.end();
    }
};
