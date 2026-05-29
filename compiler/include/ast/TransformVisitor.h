#pragma once
#include "modules/Module.h"
#include "utils/logging.h"


struct Module;

namespace detail {
template <typename D, typename N, typename... Args>
concept HasTransform = requires (D& derived, N* node, Args&&... args) {
    { derived.transform(node, std::forward<Args>(args)...) }
        -> std::convertible_to<const Node*>;
};
}


template <typename Derived>
class TransformVisitor {
public:
    explicit TransformVisitor(Module* module)
        : SymMan(module->symbol_table)
        , m_Module(module) {}


    template <typename... Args>
    Node* run(const Node* node, Args&&... args) {
    #define SW_NODE(x, y) case x: \
    if constexpr (detail::HasTransform<Derived, y, Args...>) \
        return const_cast<Node*> \
            (derived()->transform(static_cast<y*>(const_cast<Node*>(node)), std::forward<Args>(args)...)); \
        return const_cast<Node*> \
            (transformDefault(static_cast<const y*>(node), std::forward<Args>(args)...));

        if (node == nullptr) {
            SW_LOG_WARN("TransformVisitor::run: node == nullptr");
            return nullptr;
        }

        switch (node->kind) {
            SW_NODE_LIST
        } throw std::runtime_error("TransformVisitor::run: unexpected node kind");
    #undef SW_NODE
    }


    template <typename... Args>
    AST_t run(const AST_t& ast, Args&&... args) {
        std::vector<Node*> ret;
        for (Node* node : ast) {
            ret.push_back(run(node, std::forward<Args>(args)...));
        } return ret;
    }


protected:
    SymbolManager& SymMan;

    template <typename T, typename... Args>
    T* makeNode(Args&&... args) {
        return m_Module->makeNode<T>(std::forward<Args>(args)...);
    }


    /// For leaf nodes
    template <typename... Args>
    const Node* transformDefault(const Node* node, Args&&... args) {
        return node;
    }


    // --- default transform for nodes which hold pointers to other nodes --- //

    template <typename... Args>
    const Node* transformDefault(const Expression* node, Args&&... args) {
        auto expr = run(node->expr, std::forward<Args>(args)...);
        if (expr != node->expr) {
            auto new_node = makeNode<Expression>(*node);
            new_node->expr = expr;
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Scope* node, Args&&... args) {
        bool changed = false;
        std::vector<Node*> new_children;
        for (auto child : node->children) {
            auto new_child = run(child, std::forward<Args>(args)...);
            if (new_child != child) changed = true;
            new_children.push_back(new_child);
        }
        if (changed) {
            auto new_node = makeNode<Scope>(*node);
            new_node->children = m_Module->internArray<Node*>(new_children);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const FuncCall* node, Args&&... args) {
        bool changed = false;

        auto ident = run(node->ident, std::forward<Args>(args)...);
        if (ident != node->ident) changed = true;

        std::vector<Expression*> new_args;
        for (auto arg : node->args) {
            auto new_arg = run(arg, std::forward<Args>(args)...);
            if (new_arg != arg) changed = true;
            new_args.push_back(static_cast<Expression*>(new_arg));
        }

        auto gen_args = run(&node->generic_args, std::forward<Args>(args)...);
        if (gen_args != &node->generic_args) changed = true;

        if (changed) {
            auto new_node = makeNode<FuncCall>(*node);
            new_node->ident = static_cast<Ident*>(ident);
            new_node->args = m_Module->internArray<Expression*>(new_args);
            if (gen_args != &node->generic_args)
                new_node->generic_args = *static_cast<GenericArgList*>(gen_args);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Function* node, Args&&... args) {
        bool changed = false;

        std::vector<GenericParam*> new_gen_params;
        for (auto param : node->generic_params) {
            auto new_param = run(param, std::forward<Args>(args)...);
            if (new_param != param) changed = true;
            new_gen_params.push_back(static_cast<GenericParam*>(new_param));
        }

        std::vector<Var*> new_params;
        for (auto param : node->params) {
            auto new_param = run(param, std::forward<Args>(args)...);
            if (new_param != param) changed = true;
            new_params.push_back(static_cast<Var*>(new_param));
        }

        Node* ret = nullptr;
        if (node->return_type) {
            ret = run(node->return_type, std::forward<Args>(args)...);
            if (ret != node->return_type) changed = true;
        }

        Node* children = node->children;
        if (children) {
            children = run(children, std::forward<Args>(args)...);
            if (children != node->children) changed = true;
        }

        if (changed) {
            const auto new_node = makeNode<Function>(*node);
            new_node->ident = nullptr;
            new_node->generic_params = m_Module->internArray<GenericParam*>(new_gen_params);
            new_node->params = m_Module->internArray<Var*>(new_params);
            new_node->return_type = static_cast<TypeWrapper*>(ret);
            new_node->children = static_cast<Scope*>(children);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Condition* node, Args&&... args) {
        bool changed = false;

        auto bool_expr = run(node->bool_expr, std::forward<Args>(args)...);
        if (bool_expr != node->bool_expr) changed = true;

        Node* if_scope = node->if_children;
        if (if_scope) {
            if_scope = run(if_scope, std::forward<Args>(args)...);
            if (if_scope != node->if_children) changed = true;
        }

        std::vector<Condition::elif_t> new_elif;
        for (auto& [expr, body] : node->elif_children) {
            auto new_expr = run(expr, std::forward<Args>(args)...);
            bool expr_changed = (new_expr != expr);

            bool body_changed = false;

            Node* elif_body = body;
            if (elif_body) {
                elif_body = run(elif_body, std::forward<Args>(args)...);
                if (elif_body != body) body_changed = true;
            }

            if (expr_changed || body_changed) {
                changed = true;
                new_elif.emplace_back(static_cast<Expression*>(new_expr), static_cast<Scope*>(body));
            } else {
                new_elif.emplace_back(static_cast<Expression*>(new_expr), static_cast<Scope*>(body));
            }
        }

        Scope* else_children = node->else_children;
        if (else_children) {
            else_children = static_cast<Scope*>(run(else_children, std::forward<Args>(args)...));
            if (else_children != node->else_children) changed = true;
        }

        if (changed) {
            const auto new_node = makeNode<Condition>(*node);
            new_node->bool_expr = static_cast<Expression*>(bool_expr);
            new_node->if_children   = static_cast<Scope*>(if_scope);
            new_node->elif_children = internArray<Condition::elif_t>(new_elif);
            new_node->else_children = else_children;
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Struct* node, Args&&... args) {
        bool changed = false;

        std::vector<GenericParam*> new_gen_params;
        for (auto param : node->generic_params) {
            auto new_param = run(param, std::forward<Args>(args)...);
            if (new_param != param) changed = true;
            new_gen_params.push_back(static_cast<GenericParam*>(new_param));
        }

        std::vector<Ident*> new_protocols;
        for (auto proto : node->protocols) {
            auto new_proto = run(proto, std::forward<Args>(args)...);
            if (new_proto != proto) changed = true;
            new_protocols.push_back(static_cast<Ident*>(new_proto));
        }

        Scope* members = node->members;
        if (members) {
            members = static_cast<Scope*>(run(members, std::forward<Args>(args)...));
            if (members != node->members) changed = true;
        }

        if (changed) {
            const auto new_node = makeNode<Struct>(*node);
            new_node->ident = nullptr;
            new_node->generic_params = m_Module->internArray<GenericParam*>(new_gen_params);
            new_node->protocols = m_Module->internArray<Ident*>(new_protocols);
            new_node->members = members;
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const ArrayLit* node, Args&&... args) {
        bool changed = false;
        std::vector<Expression*> new_elements;
        for (auto elem : node->elements) {
            auto new_elem = run(elem, std::forward<Args>(args)...);
            if (new_elem != elem) changed = true;
            new_elements.push_back(static_cast<Expression*>(new_elem));
        }
        if (changed) {
            auto new_node = makeNode<ArrayLit>(*node);
            new_node->elements = m_Module->internArray<Expression*>(new_elements);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const TypeWrapper* node, Args&&... args) {
        bool changed = false;

        auto type_id = run(node->type_id, std::forward<Args>(args)...);
        if (type_id != node->type_id) changed = true;

        Node* of_type = nullptr;
        if (node->of_type) {
            of_type = run(node->of_type, std::forward<Args>(args)...);
            if (of_type != node->of_type) changed = true;
        }

        if (changed) {
            const auto new_node = makeNode<TypeWrapper>(*node);
            new_node->type = nullptr;
            if (type_id != node->type_id)
                new_node->type_id = static_cast<Ident*>(type_id);
            if (of_type)
                new_node->of_type = static_cast<TypeWrapper*>(of_type);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Var* node, Args&&... args) {
        bool changed = false;

        auto var_type = run(node->var_type, std::forward<Args>(args)...);
        if (var_type != node->var_type) changed = true;

        auto value = node->initialized ? run(node->value, std::forward<Args>(args)...) : node->value;
        if (value != node->value) changed = true;

        if (changed) {
            auto new_node = makeNode<Var>(*node);
            new_node->var_ident = nullptr;
            new_node->var_type = static_cast<TypeWrapper*>(var_type);
            new_node->value = static_cast<Expression*>(value);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Op* node, Args&&... args) {
        bool changed = false;
        std::vector<Node*> new_operands;
        for (auto operand : node->operands) {
            auto new_operand = run(operand, std::forward<Args>(args)...);
            if (new_operand != operand) changed = true;
            new_operands.push_back(new_operand);
        }
        if (changed) {
            auto new_node = makeNode<Op>(*node);
            new_node->operands = m_Module->internArray<Node*>(new_operands);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Intrinsic* node, Args&&... args) {
        bool changed = false;

        auto ident = run(node->ident, std::forward<Args>(args)...);
        if (ident != node->ident) changed = true;

        std::vector<Expression*> new_args;
        for (auto arg : node->args) {
            auto new_arg = run(arg, std::forward<Args>(args)...);
            if (new_arg != arg) changed = true;
            new_args.push_back(static_cast<Expression*>(new_arg));
        }

        if (changed) {
            auto new_node = makeNode<Intrinsic>(*node);
            new_node->ident = static_cast<Ident*>(ident);
            new_node->args = m_Module->internArray<Expression*>(new_args);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Protocol* node, Args&&... args) {
        bool changed = false;

        std::vector<Ident*> new_depended;
        for (auto dep : node->depended_protocols) {
            auto new_dep = run(dep, std::forward<Args>(args)...);
            if (new_dep != dep) changed = true;
            new_depended.push_back(static_cast<Ident*>(new_dep));
        }

        std::vector<Protocol::MemberSignature> new_members;
        for (auto& member : node->members) {
            auto new_type = run(member.type, std::forward<Args>(args)...);
            if (new_type != member.type) {
                changed = true;
                auto m = member;
                m.type = static_cast<TypeWrapper*>(new_type);
                new_members.push_back(m);
            } else {
                new_members.push_back(member);
            }
        }

        std::vector<Protocol::MethodSignature> new_methods;
        for (auto& method : node->methods) {
            bool method_changed = false;

            auto new_ret = run(method.return_type, std::forward<Args>(args)...);
            if (new_ret != method.return_type) method_changed = true;

            std::vector<TypeWrapper*> new_params;
            for (auto param : method.params) {
                auto new_param = run(param, std::forward<Args>(args)...);
                if (new_param != param) method_changed = true;
                new_params.push_back(static_cast<TypeWrapper*>(new_param));
            }

            if (method_changed) {
                changed = true;
                auto m = method;
                m.return_type = static_cast<TypeWrapper*>(new_ret);
                m.params = m_Module->internArray<TypeWrapper*>(new_params);
                new_methods.push_back(m);
            } else {
                new_methods.push_back(method);
            }
        }

        if (changed) {
            auto new_node = makeNode<Protocol>(*node);
            new_node->protocol_id = nullptr;
            new_node->depended_protocols = m_Module->internArray<Ident*>(new_depended);
            new_node->members = m_Module->internArray<Protocol::MemberSignature>(new_members);
            new_node->methods = m_Module->internArray<Protocol::MethodSignature>(new_methods);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const WhileLoop* node, Args&&... args) {
        bool changed = false;

        auto condition = run(node->condition, std::forward<Args>(args)...);
        if (condition != node->condition) changed = true;

        Scope* children = node->children;
        if (children) {
            children = static_cast<Scope*>(run(children, std::forward<Args>(args)...));
            if (children != node->children) changed = true;
        }

        if (changed) {
            const auto new_node = makeNode<WhileLoop>(*node);
            new_node->condition = static_cast<Expression*>(condition);
            new_node->children = children;
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Enum* node, Args&&... args) {
        if (node->enum_type.has_value()) {
            auto new_type = run(node->enum_type.value(), std::forward<Args>(args)...);
            if (new_type != node->enum_type.value()) {
                auto new_node = makeNode<Enum>(*node);
                new_node->ident = nullptr;
                new_node->enum_type = static_cast<TypeWrapper*>(new_type);
                return new_node;
            }
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const ReturnStatement* node, Args&&... args) {
        if (node->value) {
            auto value = run(node->value, std::forward<Args>(args)...);
            if (value != node->value) {
                auto new_node = makeNode<ReturnStatement>(*node);
                new_node->value = static_cast<Expression*>(value);
                return new_node;
            }
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const Ident* node, Args&&... args) {
        bool changed = false;
        std::vector<Ident::Qualifier> new_qual;
        for (auto& q : node->full_qualification) {
            auto gen_args = run(&q.generic_args, std::forward<Args>(args)...);
            if (gen_args != &q.generic_args) {
                changed = true;
                auto q2 = q;
                q2.generic_args = *static_cast<GenericArgList*>(gen_args);
                new_qual.push_back(q2);
            } else {
                new_qual.push_back(q);
            }
        }
        if (changed) {
            auto new_node = makeNode<Ident>(*node);
            new_node->full_qualification = m_Module->internArray<Ident::Qualifier>(new_qual);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const GenericArgList* node, Args&&... args) {
        bool changed = false;
        std::vector<GenericArg*> new_args;
        for (auto arg : node->generic_args) {
            auto new_arg = run(arg, std::forward<Args>(args)...);
            if (new_arg != arg) changed = true;
            new_args.push_back(static_cast<GenericArg*>(new_arg));
        }
        if (changed) {
            auto new_node = makeNode<GenericArgList>(*node);
            new_node->generic_args = m_Module->internArray<GenericArg*>(new_args);
            return new_node;
        }
        return node;
    }


    template <typename... Args>
    const Node* transformDefault(const GenericArg* node, Args&&... args) {
        if (node->isExpression()) {
            auto expr = run(node->getExpr(), std::forward<Args>(args)...);
            if (expr != node->getExpr()) {
                const auto new_node = makeNode<GenericArg>(
                    static_cast<Expression*>(expr));
                return new_node;
            }
        } else if (node->isType()) {
            auto type = run(node->getType(), std::forward<Args>(args)...);
            if (type != node->getType()) {
                const auto new_node = makeNode<GenericArg>(
                    static_cast<TypeWrapper*>(type));
                return new_node;
            }
        }
        return node;
    }


    template <typename T>
    std::span<T> internArray(std::span<T> arr) {
        return m_Module->internArray<T>(arr);
    }


private:
    Module* m_Module;

    constexpr Derived* derived() {
        return static_cast<Derived*>(this);
    }
};
