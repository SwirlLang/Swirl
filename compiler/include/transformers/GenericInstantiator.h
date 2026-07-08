#pragma once
#include <utility>

#include "GenericSubstitutor.h"
#include "sema/SymbolRegistrationPass.h"
#include "sema/SymbolResolver.h"
#include "ast/RecursiveVisitor.h"


namespace sw {
class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
public:
    explicit GenericInstantiator(Module* module, ErrorCallback_t error_callback)
        : m_SymMan(module->symbol_table)
        , m_Module(module)
        , m_ComptimeEvaluator(module, error_callback)
        , m_Substitutor(module, m_ComptimeEvaluator)
        , m_ErrorCallback(std::move(error_callback)) {}


    struct InstKey {
        using Args_t = std::vector<std::variant<std::monostate, Type*, Value>>;

        IdentInfo* id = nullptr;
        Args_t args{};

        struct hasher {
            std::size_t operator()(const InstKey& key) const noexcept {
                std::size_t vec_hash = 0;

                for (auto& arg : key.args) {
                    vec_hash ^= std::hash<Args_t::value_type>{}(arg);
                } return combineHashes(std::hash<IdentInfo*>{}(key.id), vec_hash);
            }
        };

        bool operator==(const InstKey& other) const {
            return id == other.id && args == other.args;
        }
    };


    std::unordered_map<InstKey, IdentInfo*, InstKey::hasher> Cache;


    /// Runs symbol registration and symbol resolution passes on the node, returns `nullptr` if errors
    /// were reported.
    Node* runPasses(Node* node) {
        sema::SymbolRegistrationPass reg_pass{{
                .module = m_Module,
                .error_callback = m_ErrorCallback,
                .is_monomorphization = true
            }};

        reg_pass.dispatch(node);

        if (reg_pass.errorsOccurred()) {
            return nullptr;
        }

        sema::SymbolResolver resolver{{
                .module = m_Module,
                .error_callback = m_ErrorCallback,
                .is_monomorphization = true
            }};

        resolver.dispatch(node, sema::SymbolResolver::Data{});

        if (resolver.errorsOccurred()) {
            return nullptr;
        }

        return node;
    }


    void handle(Ident* ident) {
        std::vector<Ident::Qualifier> tmp;

        for (auto& [name, generic_args] : ident->full_qualification) {
            tmp.push_back({.name = name});

            if (!generic_args.empty()) {
                IdentInfo* id = m_SymMan.getIDInfoFor(Ident{tmp});
                assert(id != nullptr);

                const Node* node = m_SymMan.lookupDecl(id).node_ptr;
                assert(node != nullptr);

                InstKey inst_key;
                inst_key.id = id;

                SubstitutionMap_t subst_map;
                std::string       subst_name = inst_key.id->toString();

                const GlobalNode* target = m_SymMan.lookupDecl(inst_key.id).node_ptr->to<GlobalNode>();
                assert(target != nullptr);

                // early-return if args and param-size do not match
                if (target->generic_params.size() != generic_args.size()) {
                    reportError(
                        ErrCode::TOO_MANY_GENERIC_ARGS,
                        {.location = node->location});
                    return;
                }

                // --- build the key and map for the instantiation --- //
                for (auto [arg, param] : std::views::zip(generic_args, target->generic_params)) {
                    if (arg->isExpression()) {
                        Value expr_value = m_ComptimeEvaluator.evaluate(arg->getExpr(), {});
                        inst_key.args.emplace_back(expr_value);

                        subst_map.insert({param->name, {expr_value}});
                        subst_name += expr_value.toString();
                    }

                    else {
                        Type* type = arg->getType()->type;
                        assert(type != nullptr);

                        inst_key.args.emplace_back(type);

                        subst_map.insert({param->name, {type}});
                        subst_name += type->toString();
                    } subst_name += '_';
                }

                // monomorphize the generic and push it to the ast if not already done
                if (!Cache.contains(inst_key)) {
                    SubstitutionContext ctx;
                    ctx.map = subst_map;
                    ctx.substitution_name = subst_name;

                    name = m_Module->getStringPool().internLocked(subst_name);

                    Node* new_node = m_Substitutor.run(node, ctx);
                    new_node = runPasses(new_node);

                    if (!new_node) return;

                    assert(new_node->isGlobal());

                    auto* glob_node = new_node->to<GlobalNode>();
                    glob_node->is_monomorphization = true;

                    m_Module->ast.push_back(new_node);
                    Cache.insert({inst_key, new_node->getIdentInfo()});

                    ident->value = new_node->getIdentInfo();
                    ident->has_generic_args = false;

                } else {
                    ident->value = Cache.at(inst_key);
                    ident->has_generic_args = false;
                }
            }
        }
    }


    void postVisit(TypeWrapper* node) const {
        if (node->type && node->type->containsGeneric() && node->type_id && node->type_id->value) {
            Type* new_type = m_SymMan.lookupType(node->type_id->value);
            if (new_type && !new_type->containsGeneric()) {
                node->type = new_type;
            }
        }
    }


    void postVisit(Var* node) {
        if (node->var_type && node->var_type->type && node->var_ident) {
            auto& decl = m_SymMan.lookupDecl(node->var_ident);
            if (decl.swirl_type != node->var_type->type) {
                decl.swirl_type = node->var_type->type;
            }
        }
    }


    void reportError(const ErrCode code, ErrorContext ctx) const {
        ctx.module = m_Module;
        m_ErrorCallback(code, ctx);
    }


private:
    SymbolManager&     m_SymMan;
    Module*            m_Module;
    ComptimeEvaluator  m_ComptimeEvaluator;
    GenericSubstitutor m_Substitutor;
    ErrorCallback_t    m_ErrorCallback;
};
}
