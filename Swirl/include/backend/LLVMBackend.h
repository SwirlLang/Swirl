#pragma once
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <memory>
#include <print>
#include <queue>
#include <stdexcept>
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
    
    inline static const std::string TargetTriple = llvm::sys::getDefaultTargetTriple();
    inline static const llvm::TargetMachine* TargetMachine = nullptr;

    std::queue<std::function<void()>> PendingCodegenQ;
    
    // ----------------[contextual-states]-------------------
    bool IsLocalScope = false;
    bool ChildHasReturned = false;
    bool LatestBoundIsIntegral = false;
    bool LatestBoundIsFloating = false;
    bool IsAssignmentLHS = false;

    Type*              LatestBoundType = nullptr;
    llvm::Type*        BoundLLVMTypeState = nullptr;
    // -------------------------------------------------------
    
    explicit LLVMBackend(SwAST_t ast, const std::string& mod_name, SymbolManager sym_man, ErrorManager em)
            : LModule{std::make_unique<llvm::Module>(mod_name, Context)}
            , AST(std::move(ast))
            , SymMan(std::move(sym_man))
            , ErrMan(std::move(em)) 
    {
        if (m_AlreadyInstantiated) {
            LModule->setDataLayout(TargetMachine->createDataLayout());
            LModule->setTargetTriple(TargetTriple);
        }

        if (m_AlreadyInstantiated) return;

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        std::string error;
        const auto target = llvm::TargetRegistry::lookupTarget(TargetTriple, error);

        if (!target) {
            throw std::runtime_error("Failed to lookup target! " + error);
        }

        llvm::TargetOptions options;
        auto reloc_model = std::optional<llvm::Reloc::Model>();

        TargetMachine = target->createTargetMachine(
            TargetTriple, "generic", "", options, reloc_model
        );

        LModule->setDataLayout(TargetMachine->createDataLayout());
        LModule->setTargetTriple(TargetTriple);

        m_AlreadyInstantiated = true;
    }



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

    void setBoundTypeState(Type* to) {
        m_Cache = LatestBoundType;
        std::println("caching expr_type: {}", (void*)m_Cache);

        LatestBoundType = to;
        BoundLLVMTypeState = to->llvmCodegen(*this);
        
        if (BoundLLVMTypeState->isFloatingPointTy())
            LatestBoundIsFloating = true;
        else if (BoundLLVMTypeState->isIntegerTy())
            LatestBoundIsIntegral = true;
    }

    void restoreBoundTypeState() {
        LatestBoundType = m_Cache;
        if (m_Cache != nullptr) {
            BoundLLVMTypeState = m_Cache->llvmCodegen(*this);
            LatestBoundIsFloating = BoundLLVMTypeState->isFloatingPointTy();
            LatestBoundIsIntegral = BoundLLVMTypeState->isIntegerTy();
        }
    }

private:
    Type* m_Cache = nullptr;
    bool m_AlreadyInstantiated = false;

    std::unordered_set<std::size_t> m_ResolvedList;
    std::size_t m_CurParentIndex = 0;
};
