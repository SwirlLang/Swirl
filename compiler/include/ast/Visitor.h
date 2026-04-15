#pragma once
#include "Nodes.h"
#include "utils/utils.h"


#define CREATE_STATE_RAII_WRAPPER_IMPL(attr_name, name) struct name { \
    decltype(attr_name) cache; \
    decltype(attr_name)& ref; \
    name(decltype(attr_name)& attr, decltype(attr_name) to): ref(attr) { \
        cache = attr; attr = to;\
    } \
    ~name() { \
        ref = cache;\
    }}

/// Generates a struct which sets `attr_name` to a value in its constructor, and resets it in its destructor.
#define CREATE_STATE_RAII_WRAPPER(attr_name) CREATE_STATE_RAII_WRAPPER_IMPL(attr_name, SW_PASTE(attr_name, ___))
#define SET_STATE_TO(attr_name, attr_value) SW_PASTE(attr_name, ___) GET_UNIQUE_NAME(raii_inst){attr_name, attr_value}


namespace detail {
template <typename T>
struct ExactType {
    operator T() const;
    template <typename U> operator U() const = delete;
};

template <typename D, typename T, typename... Args>
concept HasExactHandle = requires(D& d, T* node, Args&&... args) {
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


template <typename Callable, typename Ret>
concept UnifierCallable =
    std::same_as<Ret, void> || requires (Callable& callable, Ret& ret)
{
    { callable(ret, ret) } -> std::convertible_to<Ret>;
};


template <typename Ret>
struct DefaultUnifier {
    Ret operator()(Ret&, Ret&) {
        static_assert(std::is_default_constructible_v<Ret>);
        return Ret{};
    }
};

template <>
struct DefaultUnifier<void> {
    void operator()() const {}
};
}


#define SW_NODE(x, y) case x: \
return dispatchAs<y>(node, std::forward<Args>(args)...);


template <typename Derived,
          typename Ret,
          typename Unifier_t = detail::DefaultUnifier<Ret>>
requires detail::UnifierCallable<Unifier_t, Ret>
class Visitor {
public:
    template <typename... Args>
    Ret dispatch(Node* node, Args&&... args) {
        if (node == nullptr) {
            if constexpr (std::is_pointer_v<Ret>)
                return nullptr;
            else return Ret{};
        }

        switch (node->kind) {
            SW_NODE_LIST
            default: throw std::runtime_error("Visitor::dispatch: invalid node kind");
        }
    }


    template <typename... Args> requires std::is_void_v<Ret>
    void dispatch(AST_t& ast, Args&&... args) {
        for (auto& node : ast) {
            dispatch(node.get(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args> requires (!std::is_void_v<Ret>)
    Ret dispatch(AST_t& ast, Args&&... args) {
        std::optional<Ret> ret;
        for (auto& node : ast) {
            auto result = dispatch(node.get(), std::forward<Args>(args)...);
            ret = ret.has_value() ? m_UnifierCallable(ret.value(), result) : result;
        } return ret.has_value() ? *ret : Ret{};
    }


private:
    Unifier_t m_UnifierCallable;

    template <typename T, typename... Args>
    Ret dispatchAs(Node* node, Args&&... args) {
        auto* derived = static_cast<Derived*>(this);
        return derived->visit(static_cast<T*>(node), std::forward<Args>(args)...);
    }
};

#undef SW_NODE

#define SW_NODE(x, y) case x: \
    visitImpl(static_cast<y*>(node), std::forward<Args>(args)...); \
    break;


#if !defined(SW_PRE_VISIT_IMPL_HOOK) && !defined(SW_POST_VISIT_IMPL_HOOK)
#define SW_PRE_VISIT_IMPL_HOOK(x)
#define SW_POST_VISIT_IMPL_HOOK(x)
#endif


/*
 * This visitor class attempts to traverse the entire AST and provides the following hooks:
 * - `bool preVisit`: called first upon the visit, if it returns true, then the other hooks (beside `postVisit`)
 *                    are called.
 * - `void handle`:   can contain the main node mutation or reading logic.
 *
 * - `bool shouldTraverse`: if this returns `true`, then further traversal of the node continues.
 * - `void postVisit`     : always called for symmetry with `preVisit`. */
template <typename Derived>
class RecursiveVisitor : public Visitor<Derived, void> {
public:
    template <typename... Args>
    void visit(Node* node, Args&&... args) {
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

        SW_PRE_VISIT_IMPL_HOOK(node);
        bool handle_node = true;

        if constexpr (HasPreVisit<Derived, T, Args...>) {
            handle_node = self->preVisit(node, std::forward<Args>(args)...);
        }

        if (handle_node) {
            if constexpr (HasExactHandle<Derived, T, Args...>) {
                self->handle(node, std::forward<Args>(args)...);
            }

            bool should_traverse = true;
            if constexpr (HasShouldTraverse<Derived, T, Args...>) {
                should_traverse = self->shouldTraverse(node, std::forward<Args>(args)...);
            }

            if (should_traverse) {
                this->traverse(node, std::forward<Args>(args)...);
            }
        }

        if constexpr (HasPostVisit<Derived, T, Args...>) {
            self->postVisit(node, std::forward<Args>(args)...);
        }

        SW_POST_VISIT_IMPL_HOOK(node);
    }


    template <typename... Args>
    void traverse(Scope* node, Args&&... args) {
        for (auto& child : node->children) {
            this->dispatch(child.get(), std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void traverse(FuncCall* node, Args&&... args) {
        this->dispatch(&node->ident, std::forward<Args>(args)...);

        for (auto& expr : node->args) {
            this->dispatch(&expr, std::forward<Args>(args)...);
        }

        for (auto& arg : node->generic_args) {
            this->dispatch(&arg, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Function* node, Args&&... args) {
        for (auto& child : node->generic_params) {
            this->dispatch(&child, std::forward<Args>(args)...);
        }

        for (auto& child : node->params) {
            this->dispatch(&child, std::forward<Args>(args)...);
        }

        this->dispatch(&node->return_type, std::forward<Args>(args)...);
        for (auto& child : node->children) {
            this->dispatch(child.get(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Condition* node, Args&&... args) {
        this->dispatch(&node->bool_expr, std::forward<Args>(args)...);
        for (auto& child : node->if_children) {
            this->dispatch(child.get(), std::forward<Args>(args)...);
        }

        for (auto& item : node->elif_children) {
            auto& condition = std::get<Expression>(item);
            auto& children  = std::get<std::vector<SwNode>>(item);

            this->dispatch(&condition, std::forward<Args>(args)...);
            for (auto& child : children) {
                this->dispatch(child.get(), std::forward<Args>(args)...);
            }
        }

        for (auto& child : node->else_children) {
            this->dispatch(child.get(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Struct* node, Args&&... args) {
        for (auto& param : node->generic_params) {
            this->dispatch(&param, std::forward<Args>(args)...);
        }

        for (auto& proto : node->protocols) {
            this->dispatch(&proto, std::forward<Args>(args)...);
        }

        for (auto& member : node->members) {
            this->dispatch(member.get(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(ArrayLit* node, Args&&... args) {
        for (auto& element : node->elements) {
            this->dispatch(&element, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(TypeWrapper* node, Args&&... args) {
        this->dispatch(&node->type_id, std::forward<Args>(args)...);
        this->dispatch(node->of_type.get(), std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Var* node, Args&&... args) {
        if (node->var_type.has_value()) {
            this->dispatch(&node->var_type.value(), std::forward<Args>(args)...);
        }

        if (node->initialized) {
            this->dispatch(&node->value, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Op* node, Args&&... args) {
        for (auto& operand : node->operands) {
            this->dispatch(operand.get(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Intrinsic* node, Args&&... args) {
        this->dispatch(&node->ident, std::forward<Args>(args)...);
        for (auto& expr : node->args) {
            this->dispatch(&expr, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Protocol* node, Args&&... args) {
        for (auto& dep : node->depended_protocols) {
            this->dispatch(&dep, std::forward<Args>(args)...);
        }

        for (auto& member : node->members) {
            this->dispatch(&member.type, std::forward<Args>(args)...);
        }

        for (auto& member : node->methods) {
            for (auto& param : member.params) {
                this->dispatch(&param, std::forward<Args>(args)...);
            } this->dispatch(&member.return_type, std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(WhileLoop* node, Args&&... args) {
        this->dispatch(&node->condition, std::forward<Args>(args)...);
        for (auto& child : node->children) {
            this->dispatch(child.get(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(Enum* node, Args&&... args) {
        if (node->enum_type.has_value()) {
            this->dispatch(&node->enum_type.value(), std::forward<Args>(args)...);
        }
    }


    template <typename... Args>
    void traverse(ReturnStatement* node, Args&&... args) {
        this->dispatch(&node->value, std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Expression* node, Args&&... args) {
        this->dispatch(node->expr.get(), std::forward<Args>(args)...);
    }


    template <typename... Args>
    void traverse(Ident* node, Args&&... args) {
        for (auto& arg : node->full_qualification) {
            for (TypeWrapper* ty : arg.generic_args) {
                this->dispatch(ty, std::forward<Args>(args)...);
            }
        }
    }


    template <typename... Args>
    void traverse(Node* node, Args&&... args) {}
};

#undef SW_NODE