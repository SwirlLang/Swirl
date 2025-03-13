#include <parser/Parser.h>
#include "parser/Nodes.h"

using SwAST_t = std::vector<std::unique_ptr<Node>>;
class SymbolManager;

struct AnalysisContext {
    AnalysisContext(Parser& parser): m_AST(parser.AST), SymMan(parser.SymbolTable) {}

    void startAnalysis() {
        for (auto& child : m_AST) {
            child->analyzeSemantics(*this);
        }
    }
    

    SymbolManager& SymMan;

private:
    SwAST_t& m_AST;
};