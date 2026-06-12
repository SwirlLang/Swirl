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
    explicit GenericSubstitutor(Module* module, sw::ComptimeEvaluator& evaluator)
        : TransformVisitor(module)
        , m_Evaluator(evaluator) {}


    Node* transform(const Function* node, SubstitutionContext& ctx) {
        const auto transformed_fn = const_cast<Node*>(transformDefault(node, ctx));

        const auto new_node = makeNode<Function>(*(transformed_fn->to<Function>()));

        if (!m_IsWithinStruct) {
            new_node->name = ctx.substitution_name;
        }

        return new_node;
    }


    Node* transform(const Struct* node, SubstitutionContext& ctx) {
        m_IsWithinStruct = true;
        const auto transformed_struct = const_cast<Node*>(transformDefault(node, ctx));
        m_IsWithinStruct = false;

        const auto new_node = makeNode<Struct>(*(transformed_struct->to<Struct>()));
        new_node->name = ctx.substitution_name;
        return new_node;
    }


    Node* transform(const Ident* node, SubstitutionContext& ctx) {
        assert(!node->full_qualification.empty());
        if (ctx.map.contains(node->full_qualification.front().name)) {
            const auto element = ctx.map[node->full_qualification.front().name];
            assert(std::holds_alternative<sw::Value>(element));

            return m_Evaluator.makeNode(std::get<sw::Value>(element));
        } return const_cast<Node*>(transformDefault(node, ctx));
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


private:
    sw::ComptimeEvaluator& m_Evaluator;
    bool m_IsWithinStruct = false;
};