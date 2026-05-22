#pragma once
#include "Nodes.h"
#include "Visitor.h"
#include "utils/logging.h"


namespace detail {
template <typename D, typename T, typename... Args>
concept HasHandle = requires(D& d, T* node, Args&&... args) {
    { d.handle(node, std::forward<Args>(args)...) } -> std::same_as<void>;
};


template <typename D, typename T, typename... Args>
concept HasPreVisit = requires (D& d, T* node, Args&&... args) {
    { d.preVisit(node, std::forward<Args>(args)...) } -> std::same_as<bool>;
};


template <typename D, typename T, typename... Args>
concept HasPostVisit = requires (D& d, T* node, Args&&... args) {
    { d.postVisit(node, std::forward<Args>(args)...) } -> std::same_as<void>;
};


template <typename D, typename T, typename... Args>
concept HasShouldTraverse = requires (D& d, T* node, Args&&... args) {
    { d.shouldTraverse(node, std::forward<Args>(args)...) } -> std::same_as<bool>;
};


template <typename D, typename T, typename... Args>
concept HasPreVisitImplHook = requires (D& d, T* node, Args&&... args) {
    { d.preVisitImplHook(node) } -> std::same_as<void>;
};


template <typename D, typename T, typename... Args>
concept HasPostVisitImplHook = requires (D& d, T* node, Args&&... args) {
    { d.postVisitImplHook(node) } -> std::same_as<void>;
};
}


#define SW_NODE(x, y) case x: \
    visitImpl(static_cast<y*>(node), std::forward<Args>(args)...); \
    break;


/*
 * This visitor class attempts to traverse the entire AST and provides the following hooks:
 * - `bool preVisit`: called first upon the visit, if it returns true, then the other hooks (beside `postVisit`)
 *                    are called.
 * - `void handle`:   if overridden, is responsible for traversal too.
 * - `bool shouldTraverse`: if this returns `true`, then further traversal of the node continues.
 * - `void postVisit`     : always called for symmetry with `preVisit`. */
template <typename Derived>
class RecursiveVisitor : public Visitor<Derived, void> {
public:
    template <typename... Args>
    void visit(Node* node, Args&&... args) {
        if (!node) {
            SW_LOG_WARN("RecursiveVisitor::visit: node = nullptr.");
            return;
        }

        // all unhandled nodes will be passed here by the base visitor
        switch (node->kind) {
            SW_NODE_LIST; // NOLINT(*-pro-type-static-cast-downcast)
            default: throw std::runtime_error("RecursiveVisitor::visit: invalid node kind");
        }
    }


private:
    template <typename T, typename... Args>
    void visitImpl(T* node, Args&&... args) {
        using namespace detail;
        auto self = static_cast<Derived*>(this);

        if constexpr (HasPreVisitImplHook<Derived, T, Args...>) {
            self->preVisitImplHook(node);
        }

        bool handle_node = true;

        if constexpr (HasPreVisit<Derived, T, Args...>) {
            handle_node = self->preVisit(node, std::forward<Args>(args)...);
        }

        if (handle_node) {
            if constexpr (HasHandle<Derived, T, Args...>) {
                self->handle(node, std::forward<Args>(args)...);
            } else {
                bool should_traverse = true;
                if constexpr (HasShouldTraverse<Derived, T, Args...>) {
                    should_traverse = self->shouldTraverse(node, std::forward<Args>(args)...);
                }

                if (should_traverse) {
                    this->traverse(node, std::forward<Args>(args)...);
                }
            }
        }

        if constexpr (HasPostVisit<Derived, T, Args...>) {
            self->postVisit(node, std::forward<Args>(args)...);
        }

        if constexpr (HasPostVisitImplHook<Derived, T, Args...>) {
            self->postVisitImplHook(node);
        }
    }


protected:
    template <typename... Args>
    void traverse(Scope* node, Args&&... args) {
        for (auto& child : node->children) {
            this->dispatch(child, std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void traverse(FuncCall* node, Args&&... args) {
        this->dispatch(node->ident, std::forward<Args>(args)...);

        for (Expression* expr : node->args) {
            this->dispatch(expr, std::forward<Args>(args)...);
        }

        for (auto& arg : node->generic_args) {
            this->dispatch(arg, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Function* node, Args&&... args) {
        for (GenericParam* child : node->generic_params) {
            this->dispatch(child, std::forward<Args>(args)...);
        }

        for (Var* child : node->params) {
            this->dispatch(child, std::forward<Args>(args)...);
        }

        this->dispatch(node->return_type, std::forward<Args>(args)...);
        visit(node->children, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Condition* node, Args&&... args) {
        this->dispatch(node->bool_expr, std::forward<Args>(args)...);
        visit(node->if_children, std::forward<Args>(args)...);

        for (auto& [expr, body] : node->elif_children) {
            visit(expr, std::forward<Args>(args)...);
            visit(body, std::forward<Args>(args)...);
        }

        visit(node->else_children, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Struct* node, Args&&... args) {
        for (GenericParam* param : node->generic_params) {
            this->dispatch(param, std::forward<Args>(args)...);
        }

        for (Ident* proto : node->protocols) {
            this->dispatch(proto, std::forward<Args>(args)...);
        }

        visit(node->members, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(ArrayLit* node, Args&&... args) {
        for (Expression* element : node->elements) {
            this->dispatch(element, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(TypeWrapper* node, Args&&... args) {
        this->dispatch(node->type_id, std::forward<Args>(args)...);
        this->dispatch(node->of_type, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Var* node, Args&&... args) {
        if (node->var_type) {
            this->dispatch(node->var_type, std::forward<Args>(args)...);
        }

        if (node->initialized) {
            this->dispatch(node->value, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Op* node, Args&&... args) {
        for (auto& operand : node->operands) {
            this->dispatch(operand, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Intrinsic* node, Args&&... args) {
        this->dispatch(node->ident, std::forward<Args>(args)...);
        for (Expression* expr : node->args) {
            this->dispatch(expr, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Protocol* node, Args&&... args) {
        for (Ident* dep : node->depended_protocols) {
            this->dispatch(dep, std::forward<Args>(args)...);
        }

        for (auto& member : node->members) {
            this->dispatch(member.type, std::forward<Args>(args)...);
        }

        for (auto& member : node->methods) {
            for (auto& param : member.params) {
                this->dispatch(param, std::forward<Args>(args)...);
            } this->dispatch(member.return_type, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(WhileLoop* node, Args&&... args) {
        this->dispatch(node->condition, std::forward<Args>(args)...);
        visit(node->children, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Enum* node, Args&&... args) {
        if (node->enum_type.has_value()) {
            this->dispatch(node->enum_type.value(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(ReturnStatement* node, Args&&... args) {
        this->dispatch(node->value, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Expression* node, Args&&... args) {
        this->dispatch(node->expr, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Ident* node, Args&&... args) {
        for (auto& arg : node->full_qualification) {
            this->dispatch(&arg.generic_args, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(GenericArgList* node, Args&&... args) {
        for (GenericArg* arg : node->generic_args) {
            this->dispatch(arg, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(GenericArg* node, Args&&... args) {
        if (node->isEmpty()) return;
        if (node->isExpression()) {
            this->dispatch(node->getExpr(), std::forward<Args>(args)...);
        } else this->dispatch(node->getType(), std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Node* node, Args&&... args) {}
};

#undef SW_NODE