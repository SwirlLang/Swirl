#pragma once
#include <vector>
#include <ranges>

#include <unordered_map>
#include <parser/Parser.h>


class ModuleManager {
    std::unordered_map<std::filesystem::path, std::unique_ptr<Parser>> m_ModuleMap;
    std::vector<Parser*> m_ZeroDepVec;  // holds modules with zero dependencies
    std::vector<Parser*> m_BackBuffer;  // this will be swapped with the above vector after flushing

    friend class Parser;

public:
    Parser& get(const std::filesystem::path& m) const {
        return *m_ModuleMap.at(m);
    }

    Parser* popZeroDepVec() {
        if (m_ZeroDepVec.empty())
            return nullptr;

        const auto ret = m_ZeroDepVec.back();
        m_ZeroDepVec.pop_back();

        for (Parser* dependent : ret->m_Dependents) {
            dependent->decrementUnresolvedDeps();
        } return ret;
    }

    void swapBuffers() {
        m_ZeroDepVec = std::move(m_BackBuffer);
    }

    void insert(const std::filesystem::path& path, const ErrorCallback_t& error_callback) {
        m_ModuleMap.emplace(path, new Parser{path, error_callback, *this});
    }

    bool zeroVecIsEmpty() const {
        return m_ZeroDepVec.empty();
    }

    bool contains(const std::filesystem::path& mod) const {
        return m_ModuleMap.contains(mod);
    }

    std::size_t size() const {
        return m_ModuleMap.size();
    }

    auto begin()  {
        return m_ModuleMap.begin();
    }

    auto end() {
        return m_ModuleMap.end();
    }

    auto begin() const  {
        return m_ModuleMap.begin();
    }

    auto end() const  {
        return m_ModuleMap.end();
    }
};