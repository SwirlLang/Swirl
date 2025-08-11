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

    explicit IdentInfo(std::string ident, std::filesystem::path path)
        : id(std::move(ident))
        , mod_path(std::move(path)) {}

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

    friend class SymbolManager;

public:
    explicit IdentManager(std::filesystem::path mod_path): m_ModPath(std::move(mod_path)) {}

    /// registers a new IdentInfo and returns its pointer
    IdentInfo* createNew(const std::string& id) {
        m_IdentTable.emplace(id, new IdentInfo(id, m_ModPath));
        return m_IdentTable.at(id).get();
    }

    /// fetches `id`
    IdentInfo* fetch(const std::string& id) const {
        return m_IdentTable.at(id).get();
    }

    bool contains(const std::string& id) const {
        return m_IdentTable.contains(id);
    }

    const fs::path& getModulePath() const {
        return m_ModPath;
    }

    auto begin() const {
        return m_IdentTable.begin();
    }

    auto end() const  {
        return m_IdentTable.end();
    }
};
