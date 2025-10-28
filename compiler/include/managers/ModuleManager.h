#pragma once
#include <vector>

#include <unordered_map>
#include <parser/Parser.h>


class ModuleManager {
    std::unordered_map<std::filesystem::path, std::unique_ptr<Parser>> m_ModuleMap;
    std::vector<Parser*> m_ZeroDepVec;  // holds modules with zero dependencies
    std::vector<Parser*> m_BackBuffer;  // this will be swapped with the above vector after flushing
    std::vector<Parser*> m_OrderedMods; // keeps parsers in dependencies-dependent order, left-to-right

    std::size_t m_ModCount = 0;
    std::unordered_map<fs::path, std::size_t> m_ModuleUIDTable;

    Parser* m_MainModule = nullptr;
    friend class Parser;

public:
    Parser& get(const std::filesystem::path& m) const {
        return *m_ModuleMap.at(m);
    }

    /// pops and returns a Parser instance which has no dependency
    Parser* popZeroDepVec() {
        if (m_ZeroDepVec.empty())
            return nullptr;

        const auto ret = m_ZeroDepVec.back();
        m_ZeroDepVec.pop_back();
        m_OrderedMods.push_back(ret);

        // decrement the dependency counters of all parser instances which
        // depend on this one
        for (Parser* dependent : ret->m_Dependents) {
            dependent->decrementUnresolvedDeps();
        } return ret;
    }

    void swapBuffers() {
        m_ZeroDepVec = std::move(m_BackBuffer);
    }

    void insert(const std::filesystem::path& path, const ErrorCallback_t& error_callback) {
        m_ModuleMap.emplace(path, new Parser{path, error_callback, *this});
        m_ModuleUIDTable.emplace(path, m_ModCount);
        m_ModCount += 1;
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

    void setMainModParser(Parser* parser) {
        assert(parser != nullptr);
        m_MainModule = parser;
        m_ModuleMap[parser->m_FilePath] = std::unique_ptr<Parser>(m_MainModule);
        m_ModuleUIDTable.emplace(parser->m_FilePath, m_ModCount);
        m_ModCount += 1;
    }

    Parser* getMainModParser() const {
        return m_MainModule;
    }

    std::string getModuleUID(const fs::path& path) const {
        return path.filename().replace_extension().string() +
               '_' + std::to_string(m_ModuleUIDTable.at(path));
    }

    auto begin()  {
        return m_OrderedMods.begin();
    }

    auto end() {
        return m_OrderedMods.end();
    }

    auto begin() const  {
        return m_OrderedMods.begin();
    }

    auto end() const  {
        return m_OrderedMods.end();
    }
};