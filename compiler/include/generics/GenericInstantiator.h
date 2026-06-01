#pragma once
#include <utility>

#include "GenericSubstitutor.h"
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


    std::unordered_set<InstKey, InstKey::hasher> Cache;


    void instantiateAllGenerics() {
        for (Node* node : m_Module->ast) {
            visit(node);
        }
    }


    Node* runAllPassesOn(Node* node) {
        sema::Sema sema_inst{m_Module, m_ErrorCallback};
        sema_inst.start(node);

        if (!sema_inst.errorsOccurred()) {
            Node* ret = m_ComptimeEvaluator.run(node);
            return ret;
        } return nullptr;
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
                    }
                }

                // monomorphize the generic and push it to the ast if not already done
                if (!Cache.contains(inst_key)) {
                    SubstitutionContext ctx;
                    ctx.map = subst_map;
                    ctx.substitution_name = subst_name;

                    name = m_Module->getStringPool().internLocked(subst_name);

                    Node* new_node = m_Substitutor.run(node, ctx);
                    new_node = runAllPassesOn(new_node);

                    assert(new_node->isGlobal());
                    auto* glob_node = new_node->to<GlobalNode>();
                    glob_node->is_monomorphization = true;

                    m_Module->ast.push_back(new_node);
                    Cache.insert(inst_key);

                    ident->value = new_node->getIdentInfo();
                }
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
