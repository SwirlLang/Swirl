#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

#include "parser/Parser.h"
#include "utils/BumpAllocator.h"
#include "utils/FileSystem.h"
#include "modules/Module.h"


/// This is a helper class which works with the `Parser`, it manages the entire collection of modules
/// and keeps them in a topologically sorted order (dependencies-first).
class ModuleManager {

    std::unordered_map<sw::FileHandle*, std::unique_ptr<Module>> m_ModuleMap;
    std::vector<Module*> m_ZeroDepVec;  // holds modules with zero dependencies
    std::vector<Module*> m_BackBuffer;  // this will be swapped with the above vector after flushing
    std::vector<Module*> m_OrderedMods; // keeps parsers in dependencies-dependent order, left-to-right

    std::size_t m_ModCount = 0;
    std::unordered_map<fs::path, std::size_t> m_ModuleUIDTable;

    Module* m_MainModule = nullptr;

    friend struct Module;
    friend class  Parser;

public:
    Module& get(sw::FileHandle* m) const {
        return *m_ModuleMap.at(m);
    }

    /// pops and returns a Module which has no dependency
    Module* popZeroDepVec() {
        if (m_ZeroDepVec.empty())
            return nullptr;

        const auto ret = m_ZeroDepVec.back();
        m_ZeroDepVec.pop_back();
        m_OrderedMods.push_back(ret);

        // decrement the dependency counters of all parser instances which
        // depend on this one
        for (Module* dependent : ret->dependents) {
            dependent->decrementUnresolvedDeps();
        } return ret;
    }

    void swapBuffers() {
        m_ZeroDepVec = std::move(m_BackBuffer);
    }


    /// Create a Module with the given context
    Module* insert(const ModuleContext& context) {
        const auto ret = new Module{context};
        insert(context.file_handle, ret);
        return ret;
    }


    /// Create a Module entry for `handle` and replace the context's file handle with the given one
    Module* insert(sw::FileHandle* handle, const ModuleContext& context) {
        auto ctx = context;
        ctx.file_handle = handle;
        return insert(ctx);
    }


    /// Create a Module entry for `path` with the given module object
    void insert(sw::FileHandle* path, Module* module) {
        if (module->isMainModule()) {
            m_MainModule = module;
        }

        m_ModuleMap.emplace(path, module);
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

    Module* getMainModule() const {
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
