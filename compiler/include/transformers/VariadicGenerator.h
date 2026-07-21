#pragma once
#include <ranges>
#include "ast/TransformVisitor.h"
#include "sema/SymbolRegistrationPass.h"
#include "sema/SymbolResolver.h"


namespace sw {
class VariadicGenerator : public TransformVisitor<VariadicGenerator> {
public:
    struct Context {
        std::span<Type*> types{};
        std::string_view variadic_name{};

        int active_parameter = 0;
        std::vector<std::string_view> var_param_names;
    };

    struct Key {
        IdentInfo* ident{};
        std::vector<Type*> types;

        bool operator==(const Key& other) const {
            return ident == other.ident && std::ranges::equal(types, other.types);
        }

        struct hasher {
            std::size_t operator()(const Key& k) const {
                return combineHashes(
                    std::hash<IdentInfo*>()(k.ident),
                    hashSequence(k.types)
                );
            }
        };
    };

    std::unordered_map<Key, Node*, Key::hasher> Cache;

    explicit VariadicGenerator(Module* module, ErrorCallback_t error_callback)
        : TransformVisitor(module)
        , m_ErrorCallback(std::move(error_callback))
        {}


    Node* transform(const Function* node, Context& context) {
        assert(!node->params.empty());
        assert(node->params.back()->is_variadic);

        // lookup the cache and early return if this has been resolved
        if (const Key key{node->ident, {context.types.begin(), context.types.end()}};
            Cache.contains(key)) {
            return Cache[key];
            }

        IdentInfo* old_id = node->ident;

        std::string new_name = "__Vdic_" + node->ident->toString();
        std::ranges::for_each(context.types, [&new_name](const Type* ty) {
            new_name += ty->toString();
        });

        std::vector<Parameter*> new_parameters
            { node->params.begin(), node->params.end() };

        // populate it with concrete parameters
        new_parameters.pop_back();
        for (auto [i, type] : std::views::enumerate(context.types)) {
            Parameter parameter;
            parameter.type = makeNode<TypeWrapper>(type);
            parameter.name = internString(std::format("____Variadic_{}", i));

            context.var_param_names.push_back(parameter.name);
            new_parameters.push_back(makeNode<Parameter>(parameter));
        }

        auto* new_function   = makeNode<Function>(*cast<Function>(transformDefault(node, context)));
        new_function->params = internArray<Parameter*>(new_parameters);
        new_function->ident  = nullptr;
        new_function->name   = internString(new_name);

        // run symbol resolution on the function and put it in the cache
        resolve(new_function, old_id);
        Cache.insert({
            Key{
                .ident = node->ident,
                .types = {context.types.begin(), context.types.end()}
            },  new_function});

        return new_function;
    }


    Node* transform(const ForLoop* node, Context& context) {
        if (node->is_comptime) {
            if (node->iterable->expr->getNodeType() == ND_IDENT) {
                // check if the iterable is the variadic parameter
                const auto id = node->iterable->expr->to<Ident>();
                if (id->full_qualification.front().name == context.variadic_name) {
                    // begin unrolling the loop
                    Scope unrolled_loop;
                    std::vector<Node*> children;

                    context.variadic_name = node->loop_var_name;

                    for (auto _ : context.var_param_names) {
                        for (const Node* child : node->children->children) {
                            children.push_back(run(child, context));
                        } context.active_parameter++;
                    }

                    unrolled_loop.children = internArray<Node*>(children);
                    return makeNode<Scope>(unrolled_loop);
                }
            }
        } return cast<Node>(node);
    }


    Node* transform(const Ident* node, Context& context) {
        if (node->full_qualification.front().name == context.variadic_name) {
            const auto active_param = context.var_param_names[context.active_parameter];

            Ident new_node(*node);
            new_node.value = nullptr;

            std::vector<Ident::Qualifier> new_full_qual = {
                node->full_qualification.begin(),
                node->full_qualification.end()
            };

            new_full_qual.front().name  = internString(active_param);
            new_node.full_qualification = internArray<Ident::Qualifier>(new_full_qual);

            return makeNode<Ident>(new_node);
        } return cast<Node>(node);
    }


    void resolve(Function* node, IdentInfo* old_id) {
        const sema::SemaContext ctx{
            .module = m_Module,
            .error_callback = m_ErrorCallback,
            .target = m_Module->getTarget()
        };

        sema::SymbolRegistrationPass pass_1{ctx};
        sema::SymbolResolver pass_2{ctx};

        pass_1.dispatch(node);
        assert(!pass_1.errorsOccurred());

        if (!pass_1.errorsOccurred()) {
            pass_2.dispatch(node);
        }

        // since the entire struct isn't re-handled by the first two passes, parent context
        // fields of TableEntry like `method_of` or `is_static` will not be set, we therefore
        // copy the old id's fields into the new one
        assert(node->ident != nullptr);
        assert(old_id != nullptr);

        SymMan.lookupDecl(node->ident) = SymMan.lookupDecl(old_id);
        SymMan.lookupDecl(node->ident).node_ptr = node;
        m_Module->ast.push_back(node);
    }


private:
    ErrorCallback_t m_ErrorCallback;
};
}