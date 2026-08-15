#pragma once
#include <vector>
#include <memory>
#include <expected>
#include <unordered_map>

#include "Target.h"
#include "parser/Parser.h"
#include "utils/BumpAllocator.h"
#include "utils/FileSystem.h"


class ModuleManager;
class CompilerInst;

namespace sw {
    class StringPool;
}


struct ModuleContext {
    sw::FileHandle* file_handle{};
    ModuleManager&  module_manager;
    sw::StringPool& string_pool;
    sw::Target&     target;
};


struct Module {
    AST_t ast{};
    SymbolManager symbol_table;
    sw::FileHandle* file_handle = nullptr;

    struct ProtocolImplInfo {
        bool is_exported = false; Namespace* scope = nullptr;
        Module* parent_module = nullptr;
    };

    using ProtocolImplTable = std::unordered_map<
        Type*, std::unordered_map<ProtocolConstraint*, ProtocolImplInfo>>;

    /// A registered protocol impl: its info (member-scope, export status,
    /// declaring module) and the protocol it satisfies.
    struct ImplScopeRef {
        const ProtocolImplInfo* info = nullptr;
        const ProtocolConstraint* protocol = nullptr;
    };


    explicit Module(const ModuleContext& context);

    /// the modules which directly depend on this module
    std::unordered_set<Module*> dependents{};

    /// the modules which this module directly depends on
    std::unordered_set<Module*> dependencies{};

    /// all modules which directly or indirectly depend on this one
    std::unordered_set<Module*> cumulative_dependents{};

    /// counter for the no. of unresolved dependencies
    std::size_t unresolved_deps{};

    /// Decrements the unresolved-deps counter of dependents
    void decrementUnresolvedDeps();

    /// Returns whether this module is the main one
    bool isMainModule() const { return m_IsMainModule; }

    /// Returns whether the module has been marked erroneous
    bool isErroneous() const { return m_IsErroneous; }

    sw::Target& getTarget() const { return m_Target; }

    /// Marks the module as erroneous
    void markErroneous() { m_IsErroneous = true; }

    /// Creates a Parser instance and begins parsing
    void parse(const ErrorCallback_t& error_callback) {
        auto context = ParserContext{this, error_callback, m_ModuleManager, m_StringPool};
        const auto parser = std::make_unique<Parser>(context);
        parser->parse();
    }

    void performSema(const ErrorCallback_t& error_callback);

    void performComptimeEval(const ErrorCallback_t& error_callback);

    /// Calls `inserter` with the symbol name for each exported-symbol in the AST
    template <typename Inserter_t> requires std::invocable<Inserter_t, std::string_view>
    void insertExportedSymbolsInto(Inserter_t inserter) {
        for (const auto& node : ast) {
            if (node->isGlobal()) {
                const auto glob_node = node->to<GlobalNode>();
                if (glob_node->is_exported && !glob_node->name.empty()) {
                    inserter(glob_node->name);
                }
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


    template <typename T>
    std::span<T> internArray(std::span<T> arr) {
        if (arr.empty()) return {};

        auto* memory = m_Allocator.allocate(sizeof(T) * arr.size(), alignof(T));

        auto* offset = memory;
        for (const T& element : arr) {
            std::construct_at(reinterpret_cast<T*>(offset), element);
            offset += sizeof(T);
        }

        return std::span<T>{reinterpret_cast<T*>(memory), arr.size()};
    }


    std::optional<ProtocolImplInfo> lookupProtocolImpl(Type* type, ProtocolConstraint* protocol) {
        if (m_ProtocolImplTable.contains(type)) {
            if (auto& map = m_ProtocolImplTable[type]; map.contains(protocol)) {
                return map[protocol];
            }
        }

        for (Module* dep : dependencies) {
            auto& table = dep->m_ProtocolImplTable;
            if (table.contains(type)) {
                if (auto& map = table[type]; map.contains(protocol)) {
                    return map[protocol];
                }
            }
        } return std::nullopt;
    }


    /// Returns every protocol impl registered for `type`, across this module
    /// and its direct dependencies.
    std::vector<ImplScopeRef> getImplScopesFor(Type* type) const {
        std::vector<ImplScopeRef> refs;

        if (m_ProtocolImplTable.contains(type)) {
            for (const auto& [protocol, info] : m_ProtocolImplTable.at(type)) {
                if (info.scope) refs.push_back({.info = &info, .protocol = protocol});
            }
        }

        for (const Module* dep : dependencies) {
            if (const auto& table = dep->m_ProtocolImplTable; table.contains(type)) {
                for (const auto& [protocol, info] : table.at(type)) {
                    if (info.scope) refs.push_back({.info = &info, .protocol = protocol});
                }
            }
        }

        return refs;
    }

    /// Returns false if the implementation already exists.
    bool insertProtocolImpl(Type* type, ProtocolConstraint* protocol, const ProtocolImplInfo& impl_info) {
        if (lookupProtocolImpl(type, protocol) != std::nullopt)
            return false;

        if (!m_ProtocolImplTable.contains(type)) {
            m_ProtocolImplTable.insert({type, {}});
        } m_ProtocolImplTable[type].insert({protocol, impl_info});
        return true;
    }


    sw::StringPool& getStringPool() const {
        return m_StringPool;
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
    bool m_IsMainModule   = false;
    bool m_IsSemaComplete = false;
    bool m_IsErroneous    = false;

    ModuleManager&    m_ModuleManager;
    sw::BumpAllocator m_Allocator{64 * 1024};
    sw::StringPool&   m_StringPool;
    sw::Target&       m_Target;
    ModuleContext     m_CtxCopy;

    std::vector<std::array<std::size_t, 2>> m_LineOffsets{};
    std::vector<std::unique_ptr<Node*>> m_Nodes{};
    std::vector<std::function<void()>>  m_Destructors{};

    ProtocolImplTable m_ProtocolImplTable;

    friend class CompilerInst;
    friend class SourceManager;
};
