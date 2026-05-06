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
        for (Node* node : ast) {
            dispatch(node, std::forward<Args>(args)...);
        }
    }


    template <typename... Args> requires (!std::is_void_v<Ret>)
    Ret dispatch(AST_t& ast, Args&&... args) {
        std::optional<Ret> ret;
        for (auto& node : ast) {
            auto result = dispatch(node, std::forward<Args>(args)...);
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
