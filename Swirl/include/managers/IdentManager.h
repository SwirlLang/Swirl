#pragma once
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>


class IdentInfo {
    std::string id;
    friend class IdentManager;

public:
    IdentInfo() = delete;
    explicit IdentInfo(std::string id): id(std::move(id)) {}

    [[nodiscard]]
    const std::string& toString() const {
        return id;
    }
};


class IdentManager {
    std::unordered_map<std::string, std::unique_ptr<IdentInfo>> m_IdentTable;

public:
    IdentManager() = default;

    /// registers a new IdentInfo and returns its pointer
    IdentInfo* createNew(const std::string& id) {
        m_IdentTable[id] = std::make_unique<IdentInfo>(id);
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
