#pragma once
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

#include <parser/Nodes.h>
#include <managers/SymbolManager.h>
#include <managers/ErrorManager.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>


using SwAST_t = std::vector<std::unique_ptr<Node>>;

class LLVMBackend {
public:
    llvm::LLVMContext Context;
    llvm::IRBuilder<> Builder{Context};

    std::unique_ptr<llvm::Module> LModule;

    std::vector<std::unique_ptr<Node>> AST;
    SymbolManager SymMan;
    ErrorManager  ErrMan;
    
    std::queue<std::function<void()>> PendingCodegenQ;
    
    // ----------------[contextual-states]-------------------
    bool IsLocalScope = false;
    bool ChildHasReturned = false;
    bool LatestBoundIsIntegral = false;
    bool LatestBoundIsFloating = false;
    bool IsAssignmentLHS = false;

    Type*              LatestBoundType = nullptr;
    llvm::Type*        FloatTypeState = nullptr;
    llvm::IntegerType* IntegralTypeState = nullptr;
    // -------------------------------------------------------
    
    explicit LLVMBackend(SwAST_t ast, const std::string& mod_name, SymbolManager sym_man, ErrorManager em)
            : LModule{std::make_unique<llvm::Module>(mod_name, Context)}
            , AST(std::move(ast))
            , SymMan(std::move(sym_man))
            , ErrMan(std::move(em))
            , FloatTypeState{llvm::Type::getFloatTy(Context)}
            , IntegralTypeState{llvm::Type::getInt32Ty(Context)} {}
    

    void startGeneration() {
        for (auto& node : AST) {
            if (!m_ResolvedList.contains(m_CurParentIndex))
                node->llvmCodegen(*this);
            m_CurParentIndex++;
        }
    }
    
    void runPendingTasks() {
        while (!PendingCodegenQ.empty()) {
            PendingCodegenQ.front()();
            PendingCodegenQ.pop();
        }
    }

    /// codegen's the function with the id `id`
    void codegenTheFunction(IdentInfo* id) {
        std::size_t counter = m_CurParentIndex;
        auto ip_cache = this->Builder.saveIP();

        while (counter < AST.size()) {
            if (AST[counter]->getIdentInfo() == id) {
                AST[counter]->llvmCodegen(*this);
                m_ResolvedList.insert(counter);
            } counter++;
        }
        this->Builder.restoreIP(ip_cache);
    }

    void setIntegralTypeState(llvm::Type* state) {
        m_Cache = IntegralTypeState;
        IntegralTypeState = llvm::dyn_cast<llvm::IntegerType>(state);
        LatestBoundIsIntegral = true;
    }

    void restoreIntegralTypeState() {
        IntegralTypeState = llvm::dyn_cast<llvm::IntegerType>(m_Cache);
        LatestBoundIsIntegral = false;
    }

    void setFloatTypeState(llvm::Type* state) {
        m_Cache = FloatTypeState;
        FloatTypeState = state;
        LatestBoundIsFloating = true;
    }

    void restoreFloatTypeState() {
        FloatTypeState = m_Cache;
        LatestBoundIsFloating = false;
    }

    llvm::IntegerType* getIntegralTypeState() const {
        return IntegralTypeState;
    }

    llvm::Type* getFloatTypeState() const {
        return FloatTypeState;
    }

private:
    llvm::Type* m_Cache = nullptr;
    std::unordered_set<std::size_t> m_ResolvedList;
    std::size_t m_CurParentIndex = 0;
};
