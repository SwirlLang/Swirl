#include "parser/Nodes.h"

using SwAST_t = std::vector<std::unique_ptr<Node>>;

struct AnalysisContext {
    AnalysisContext(SwAST_t& nodes): m_AST(nodes) {}

    void startAnalysis() {
        for (auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }
    
private:
    SwAST_t& m_AST;
};