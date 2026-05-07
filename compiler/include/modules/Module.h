#pragma once
#include <vector>
#include <memory>
#include <unordered_map>

#include "parser/Parser.h"
#include "utils/BumpAllocator.h"
#include "utils/FileSystem.h"


class ModuleManager;
class CompilerInst;

namespace sw {
    class StringPool;
}


struct ModuleContext {
    sw::FileHandle* file_handle;
    ModuleManager&  module_manager;
    sw::StringPool& string_pool;
};


struct Module {
    AST_t ast{};
    SymbolManager symbol_table;
    sw::FileHandle* file_handle = nullptr;

    explicit Module(const ModuleContext& context);

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
        auto context = ParserContext{this, error_callback, m_ModuleManager, m_StringPool};
        const auto parser = std::make_unique<Parser>(context);
        parser->parse();
    }

    void performSema(const ErrorCallback_t& error_callback);

    /// Calls `inserter` with the symbol name for each exported-symbol in the AST
    template <typename Inserter_t> requires std::invocable<Inserter_t, std::string>
    void insertExportedSymbolsInto(Inserter_t inserter) {
        for (const auto& node : ast) {
            if (node->is_exported) {
                inserter(node->getIdentInfo()->toString());
            }
        }
    }

    template <typename T, typename... Args>
    requires std::derived_from<T, Node>
    T* makeNode(Args&&... args) {
        T* ret = m_Allocator.construct<T>(std::forward<Args>(args)...);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            m_Destructors.push_back([ret] {
               ret->~T();
           });
        } return ret;
    }

    sw::BumpAllocator& getAllocator() {
        return m_Allocator;
    }

    std::string_view getLineAt(const std::size_t line) const {
        auto [from, line_size] = m_LineOffsets[line - 1];
        return file_handle->readAll().substr(from, line_size);
    }

    ModuleContext getModuleContext() const {
        return m_CtxCopy;
    }

    ModuleManager& getModuleManager() const { return m_ModuleManager; }

    // NOTE: the manual destructor calls is a temporary workaround until all Nodes become
    //       trivially destructible
    ~Module() {
        for (auto& destructor : m_Destructors) {
            destructor();
        }
    }


private:
    bool m_IsMainModule = false;
    bool m_IsSemaComplete = false;

    ModuleManager&    m_ModuleManager;
    sw::BumpAllocator m_Allocator{64 * 1024};
    sw::StringPool&   m_StringPool;
    ModuleContext     m_CtxCopy;

    std::vector<std::array<std::size_t, 2>> m_LineOffsets{};
    std::vector<std::unique_ptr<Node*>> m_Nodes{};
    std::vector<std::function<void()>>  m_Destructors{};

    friend class CompilerInst;
    friend class SourceManager;
};
