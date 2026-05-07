#pragma once
#include "Visitor.h"

struct Module;

namespace detail {
template <typename D, typename N, typename... Args>
concept HasTransform = requires (D& derived, const N* node, Args&&... args) {
    { derived->transform(node, std::forward<Args>(args)...) } -> std::convertible_to<N*>;
};
}


template <typename Derived>
class TransformVisitor {
public:
    explicit TransformVisitor(Module* module)
        : m_Module(module) {}


    template <typename... Args>
    Node* run(const Node* node, Args&&... args) {
    #define SW_NODE(x, y) case x: \
        if constexpr (detail::HasTransform<Derived, y, Args...>) \
            return transform(static_cast<y*>(node), std::forward<Args>(args)...); \
        return transformDefault(static_cast<y*>(node), std::forward<Args>(args)...);

        switch (node->kind) {
            SW_NODE_LIST
        } throw std::runtime_error("TransformVisitor::run: unexpected node kind");
    #undef SW_NODE
    }


private:
    Module* m_Module;

    template <typename... Args>
    Node* transformDefault(const Expression* node, Args&&... args) {
        auto expr = run(node->expr);

    }
};