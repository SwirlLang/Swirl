#pragma once
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ast/TransformVisitor.h"


/// Substitutes the associated types mandated by a protocol with the concrete
/// types an implementation binds them to, e.g. `type associate_type = <type>`.
class ProtocolSubstitutor : public TransformVisitor<ProtocolSubstitutor> {
public:
    /// Maps the protocol's associated-type name to the impl's bound concrete type.
    using AliasMap_t = std::unordered_map<std::string_view, Type*>;

    explicit ProtocolSubstitutor(Module* module)
        : TransformVisitor(module) {}


    /// Applies the substitution to a single type wrapper. The returned node is
    /// different from `node` if an associated type was substituted.
    Node* substitute(const TypeWrapper* node, const AliasMap_t& map) {
        return run(node, map);
    }

    /// Applies the substitution to a list of type wrappers.
    std::span<TypeWrapper*> substitute(std::span<TypeWrapper*> nodes, const AliasMap_t& map) {
        std::vector<TypeWrapper*> ret;
        ret.reserve(nodes.size());

        for (TypeWrapper* node : nodes) {
            ret.push_back(static_cast<TypeWrapper*>(run(node, map)));
        }

        return m_Module->internArray<TypeWrapper*>(ret);
    }

    /// Swaps the associated-type leaf with the impl's bound type.
    Node* transform(const TypeWrapper* node, const AliasMap_t& map) {
        if (node->type_id && !node->type_id->full_qualification.empty()) {
            const auto name = node->type_id->full_qualification.front().name;
            if (const auto it = map.find(name); it != map.end() && it->second) {
                auto* ret = makeNode<TypeWrapper>(*node);
                ret->type = it->second;
                return ret;
            }
        }

        return const_cast<Node*>(transformDefault(node, map));
    }
};
