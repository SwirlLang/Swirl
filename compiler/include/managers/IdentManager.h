#pragma once
#include <list>
#include <string>
#include <utility>
#include <unordered_map>


class IdentInfo {
    std::string id;
    std::filesystem::path mod_path;
    friend class IdentManager;

public:
    IdentInfo() = delete;

    explicit IdentInfo(std::string id, std::filesystem::path mod_path)
        : id(std::move(id))
        , mod_path(std::move(mod_path)) {}

    [[nodiscard]]
    const std::string& toString() const {
        return id;
    }

    [[nodiscard]]
    const std::filesystem::path& getModulePath() const {
        return mod_path;
    }
};


class IdentManager {
    std::unordered_map<std::string, std::list<IdentInfo>> m_IdentTable;
    std::filesystem::path m_ModPath;

public:
    explicit IdentManager(std::filesystem::path mod_path): m_ModPath(std::move(mod_path)) {}

    /// registers a new IdentInfo and returns its pointer
    IdentInfo* createNew(const std::string& id) {
        m_IdentTable[id].emplace_back(id, m_ModPath);
        return &m_IdentTable[id].back();
    }

    /// fetches `id`
    IdentInfo* fetch(const std::string& id) {
        return &m_IdentTable.at(id).back();
    }

    bool contains(const std::string& id) const {
        return m_IdentTable.contains(id);
    }
};
