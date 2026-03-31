#pragma once
#include "Nodes.h"

#define SW_NODE(x, y) case x: \
    return dispatchAs<y>(node, std::forward<Args>(args)...);

template <typename Derived, typename Ret>
class Visitor {
public:
    template <typename... Args>
    Ret dispatch(Node* node, Args&&... args) {
        if (node == nullptr) {
            if constexpr (std::is_pointer_v<Ret>)
                return nullptr;
            else throw std::runtime_error("Visitor::dispatch: node == nullptr");
        }

        switch (node->kind) {
            SW_NODE_LIST
            default: throw std::runtime_error("Visitor::dispatch: invalid node kind");
        }
    }

private:
    template <typename T, typename... Args>
    Ret dispatchAs(Node* node, Args&&... args) {
        return derived().visit(static_cast<T*>(node), std::forward<Args>(args)...);
    }

    Derived& derived() {
        return *static_cast<Derived*>(this);
    }
};

#undef SW_NODE
