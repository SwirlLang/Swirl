#pragma once
#include "ast/TransformVisitor.h"


using SubstitutionMap_t = std::unordered_map<
        std::string_view,
        std::variant<std::monostate, sw::Value, Type*>>;


struct SubstitutionContext {
    SubstitutionMap_t map;
    std::string_view  substitution_name;
};


class GenericSubstitutor : public TransformVisitor<GenericSubstitutor> {
public:
    explicit GenericSubstitutor(Module* module)
        : TransformVisitor(module) {}


    Node* transform(const Function* node, SubstitutionContext& ctx) {
        const auto transformed_fn = const_cast<Node*>(transformDefault(node, ctx));

        const auto new_node = makeNode<Function>(*(transformed_fn->to<Function>()));
        new_node->name = ctx.substitution_name;
        return new_node;
    }


    Node* transform(const TypeWrapper* node, SubstitutionContext& ctx) {
        // perform a type substitution if id matches
        if (const auto type_id = node->type_id) {
            if (const auto name = type_id->full_qualification.front().name; ctx.map.contains(name)) {
                const auto type = std::get_if<Type*>(&ctx.map[name]);
                assert(type != nullptr);
                auto* ret = makeNode<TypeWrapper>(*node);
                ret->type = *type;
                return ret;
            }
        }
        return const_cast<Node*>(transformDefault(node, ctx));
    }
};