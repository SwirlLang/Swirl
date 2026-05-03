#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

#include "parser/Parser.h"
#include "utils/FileSystem.h"


class ModuleManager;
struct Module {
    AST_t ast{};
    SymbolManager symbol_table;
    sw::FileHandle* file_handle = nullptr;

    Module(sw::FileHandle* file_handle, ModuleManager& module_manager)
        : symbol_table(file_handle, module_manager)
        , file_handle(file_handle)
        , m_ModuleManager(module_manager) {}

    /// the modules which directly depend on this module
    std::unordered_set<Module*> dependents{};

    /// the modules which this module directly depends on
    std::unordered_set<Module*> dependencies{};

    /// all modules which directly or indirectly depend on this one
    std::unordered_set<Module*> cumulative_dependents{};

    /// maps global symbols to their nodes
    std::unordered_map<IdentInfo*, Node*> node_jmp_table{};

    /// counter for the no. of unresolved dependencies
    std::size_t unresolved_deps{};

    /// Decrements the unresolved-deps counter of dependents
    void decrementUnresolvedDeps();

    /// Returns whether this module is the main one
    bool isMainModule() const { return m_IsMainModule; }

    /// Creates a Parser instance and begins parsing
    void parse(const ErrorCallback_t& error_callback) {
        const auto parser = std::make_unique<Parser>(this, error_callback, m_ModuleManager);
        parser->parse();
    }

    void performSema(const ErrorCallback_t& error_callback);

    // TODO: to be removed when Sema gets decoupled from the Parser
    void parseAndRunSema(const ErrorCallback_t& error_callback) {
        const auto parser = std::make_unique<Parser>(this, error_callback, m_ModuleManager);
        parser->parse();
        performSema(error_callback);
    }

    /// Calls `inserter` with the symbol name for each exported-symbol in the AST
    template <typename Inserter_t> requires std::invocable<Inserter_t, std::string>
    void insertExportedSymbolsInto(Inserter_t inserter) {
        for (const auto& node : ast) {
            if (node->is_exported) {
                inserter(node->getIdentInfo()->toString());
            }
        }
    }

    std::string_view getLineAt(const std::size_t line) const {
        auto [from, line_size] = m_LineOffsets[line - 1];
        return file_handle->readAll().substr(from, line_size);
    }

    ModuleManager& getModuleManager() const { return m_ModuleManager; }

private:
    bool m_IsMainModule = false;
    bool m_IsSemaComplete = false;

    ModuleManager& m_ModuleManager;

    std::vector<std::array<std::size_t, 2>> m_LineOffsets{};

    friend class CompilerInst;
    friend class SourceManager;
};


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

    /// pops and returns a Parser instance which has no dependency
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

    Module* insert(sw::FileHandle* path) {
        const auto ret = new Module{path, *this};
        insert(path, ret);
        return ret;
    }

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


inline void Module::decrementUnresolvedDeps() {
    unresolved_deps--;
    if (unresolved_deps == 0) {
        m_ModuleManager.m_BackBuffer.push_back(this);
    }
}
