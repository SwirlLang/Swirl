#pragma once
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>

class IdentInfo {
    std::string id;
    std::filesystem::path& mod_path;
    friend class IdentManager;

public:
    IdentInfo() = delete;

    explicit IdentInfo(std::string id, std::filesystem::path& mod_path)
        : id(std::move(id))
        , mod_path(mod_path) {}

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
    std::unordered_map<std::string, std::unique_ptr<IdentInfo>> m_IdentTable;
    std::filesystem::path m_ModPath;

public:
    explicit IdentManager(std::filesystem::path mod_path): m_ModPath(std::move(mod_path)) {}

    /// registers a new IdentInfo and returns its pointer
    IdentInfo* createNew(const std::string& id) {
        m_IdentTable[id] = std::make_unique<IdentInfo>(id, m_ModPath);
        return m_IdentTable[id].get();
    }

    /// fetches `id`
    IdentInfo* fetch(const std::string& id) const {
        return m_IdentTable.at(id).get();
    }

    bool contains(const std::string& id) const {
        return m_IdentTable.contains(id);
    }
};
