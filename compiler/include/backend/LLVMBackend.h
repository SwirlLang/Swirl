#pragma once
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <parser/Nodes.h>
#include <managers/SymbolManager.h>
#include <managers/ErrorManager.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>


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

    std::unordered_map<IdentInfo*, Node*> GlobalNodeJmpTable;


    // ----------------[contextual-states]-------------------
    bool IsLocalScope = false;
    bool ChildHasReturned = false;
    bool LatestBoundIsIntegral = false;
    bool LatestBoundIsFloating = false;
    bool IsAssignmentLHS = false;

    Type*              LatestBoundType = nullptr;
    llvm::Type*        BoundLLVMTypeState = nullptr;
    // -------------------------------------------------------
    
    explicit LLVMBackend(
        SwAST_t ast,
        const std::string& mod_path,
        SymbolManager sym_man,
        ErrorManager em,
        std::unordered_map<IdentInfo*, Node*>& jmp_table)
            : LModule{std::make_unique<llvm::Module>(mod_path, Context)}
            , AST(std::move(ast))
            , SymMan(std::move(sym_man))
            , ErrMan(std::move(em))
            , GlobalNodeJmpTable(std::move(jmp_table))
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

    /// perform any necessary type casts, then return the llvm::Value*
    /// note: `subject` is supposed to be a "loaded" value
    llvm::Value* castIfNecessary(Type* source_type, llvm::Value* subject);

    void startGeneration() {
        for (auto& node : AST) {
            if (!m_ResolvedList.contains(m_CurParentIndex))
                node->llvmCodegen(*this);
            m_CurParentIndex++;
        }
    }

    /// codegen's the function with the id `id`
    void codegenTheFunction(IdentInfo* id);

    Type* fetchSwType(const std::unique_ptr<Node>& node) {
        switch (node->getNodeType()) {
            case ND_STR:
                return &GlobalTypeStr;
            case ND_INT:
                return &GlobalTypeI32;
            case ND_FLOAT:
                return &GlobalTypeF64;
            case ND_IDENT:
                return SymMan.lookupDecl(node->getIdentInfo()).swirl_type;
            case ND_EXPR:
                return node->getSwType();
            case ND_CALL:
                return dynamic_cast<FunctionType*>(node->getSwType())->ret_type;
            default:
                throw std::runtime_error("LLVMBackend::fetchSwType: failed to fetch type");
        }
    }

    void setBoundTypeState(Type* to) {
        m_Cache = LatestBoundType;

        LatestBoundType = to;
        BoundLLVMTypeState = to->llvmCodegen(*this);
        
        LatestBoundIsFloating = BoundLLVMTypeState->isFloatingPointTy();
        LatestBoundIsIntegral = BoundLLVMTypeState->isIntegerTy();
    }

    void restoreBoundTypeState() {
        LatestBoundType = m_Cache;
        if (m_Cache != nullptr) {
            BoundLLVMTypeState = m_Cache->llvmCodegen(*this);
            LatestBoundIsFloating = BoundLLVMTypeState->isFloatingPointTy();
            LatestBoundIsIntegral = BoundLLVMTypeState->isIntegerTy();
            return;
        }

        BoundLLVMTypeState = nullptr;
        LatestBoundIsIntegral = false;
        LatestBoundIsFloating = false;
    }

private:
    Type* m_Cache = nullptr;
    bool m_AlreadyInstantiated = false;

    std::unordered_set<std::size_t> m_ResolvedList;
    std::size_t m_CurParentIndex = 0;
};
