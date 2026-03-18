#pragma once
#include <vector>
#include <unordered_map>

#include "parser/Parser.h"
#include "utils/FileSystem.h"


// Thread-safe wrapper over a Parser pointer
class Module {
    Parser*    m_ParserPtr;
    std::mutex m_Mutex;

public:
    explicit Module(Parser* parser) : m_ParserPtr(parser) {}
};


/// This is a helper class which works with the `Parser`, it manages the entire collection of modules
/// and keeps them in a topologically sorted order (based on who depends on whom).
class ModuleManager {

    std::unordered_map<sw::FileHandle*, std::unique_ptr<Parser>> m_ModuleMap;
    std::vector<Parser*> m_ZeroDepVec;  // holds modules with zero dependencies
    std::vector<Parser*> m_BackBuffer;  // this will be swapped with the above vector after flushing
    std::vector<Parser*> m_OrderedMods; // keeps parsers in dependencies-dependent order, left-to-right

    std::size_t m_ModCount = 0;
    std::unordered_map<fs::path, std::size_t> m_ModuleUIDTable;

    Parser* m_MainModule = nullptr;
    friend class Parser;

public:
    Parser& get(sw::FileHandle* m) const {
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

    void insert(sw::FileHandle* path, Parser* parser) {
        m_ModuleMap.emplace(path, parser);
        m_ModuleUIDTable.emplace(path->getPath(), m_ModCount);
        m_ModCount += 1;
    }

    bool zeroVecIsEmpty() const {
        return m_ZeroDepVec.empty();
    }

    bool contains(sw::FileHandle* mod) const {
        return m_ModuleMap.contains(mod);
    }

    std::size_t size() const {
        return m_ModuleMap.size();
    }

    void setMainModParser(Parser* parser) {
        assert(parser != nullptr);
        m_MainModule = parser;
        m_ModuleMap[parser->m_FileHandle] = std::unique_ptr<Parser>(m_MainModule);
        m_ModuleUIDTable.emplace(parser->m_FileHandle->getPath(), m_ModCount);
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