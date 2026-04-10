#pragma once
#include <list>
#include <string>
#include <utility>
#include <unordered_map>

#include "utils/FileSystem.h"

class IdentInfo {
    std::string id;
    sw::FileHandle* handle = nullptr;
    bool is_fictitious = false;
    friend class IdentManager;

public:
    IdentInfo() = delete;

    explicit IdentInfo(std::string ident, sw::FileHandle* mod_handle, const bool is_fictitious = false)
        : id(std::move(ident))
        , handle(mod_handle)
        , is_fictitious(is_fictitious) {}

    [[nodiscard]]
    const std::string& toString() const {
        return id;
    }

    [[nodiscard]]
    sw::FileHandle* getModuleFileHandle() const {
        return handle;
    }

    [[nodiscard]]
    bool isFictitious() const {
        return is_fictitious;
    }
};


class IdentManager {
    std::unordered_map<std::string, std::unique_ptr<IdentInfo>> m_IdentTable;
    sw::FileHandle* m_ModuleHandle{};

    friend class SymbolManager;

public:
    explicit IdentManager(sw::FileHandle* mod_handle): m_ModuleHandle(mod_handle) {}

    /// registers a new IdentInfo and returns its pointer
    IdentInfo* createNew(const std::string& id, const bool is_fictitious = false) {
        m_IdentTable.emplace(id, new IdentInfo(id, m_ModuleHandle, is_fictitious));
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
