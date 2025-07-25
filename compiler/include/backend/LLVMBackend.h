#pragma once
#include <memory>
#include <stack>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <parser/Nodes.h>
#include <symbols/SymbolManager.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

#include <parser/Parser.h>


using SwAST_t = std::vector<std::unique_ptr<Node>>;

class LLVMBackend {
public:
    llvm::LLVMContext Context;
    llvm::IRBuilder<> Builder{Context};

    std::unique_ptr<llvm::Module> LModule;

    std::vector<std::unique_ptr<Node>> AST;
    SymbolManager& SymMan;

    inline static llvm::TargetMachine* TargetMachine = nullptr;

    std::unordered_map<Type*, llvm::Type*> LLVMTypeCache;

    // ----------------[contextual-states]-------------------
    bool IsLocalScope = false;
    bool ChildHasReturned = false;

    llvm::Value* BoundMemory = nullptr;
    llvm::Value* StructFieldPtr = nullptr;  // used to "bubble-up" StructGEPd pointers
    Type*        StructFieldType = nullptr; // used to "bubble-up" struct-field types
    // -------------------------------------------------------

    explicit LLVMBackend(Parser& parser);

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

    Type* fetchSwType(const std::unique_ptr<Node>& node) const {
        switch (node->getNodeType()) {
            case ND_STR:
                return SymMan.getStrType(dynamic_cast<StrLit*>(node.get())->value.size());
            case ND_INT:
                return &GlobalTypeI32;
            case ND_FLOAT:
                return &GlobalTypeF64;
            case ND_IDENT:
                return SymMan.lookupDecl(node->getIdentInfo()).swirl_type;
            case ND_CALL:
                return dynamic_cast<FunctionType*>(SymMan.lookupType(node->getIdentInfo()))->ret_type;
            case ND_EXPR:
            case ND_ARRAY:
            case ND_OP:
                return node->getSwType();
            default:
                throw std::runtime_error("LLVMBackend::fetchSwType: failed to fetch type");
        }
    }

    llvm::Value* toLLVMInt(const std::size_t i) {
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), i);
    }

    void printIR() const {
        verifyModule(*LModule, &llvm::errs());
        LModule->print(llvm::outs(), nullptr);
    }

    Type* getBoundTypeState() const {
        return m_LatestBoundType.top();
    }

    bool getAssignmentLhsState() const {
        assert(!m_AssignmentLhsStack.empty());
        return m_AssignmentLhsStack.top();
    }

    llvm::Type* getBoundLLVMType() {
        return m_LatestBoundType.top()->llvmCodegen(*this);
    }

    void setBoundTypeState(Type* to) {
        m_LatestBoundType.emplace(to);
    }

    void setAssignmentLhsState(bool value) {
        m_AssignmentLhsStack.emplace(value);
    }

    void restoreAssignmentLhsState() {
        m_AssignmentLhsStack.pop();
    }

    void restoreBoundTypeState() {
        m_LatestBoundType.pop();
    }

    const llvm::DataLayout& getDataLayout() const {
        return LModule->getDataLayout();
    }

    llvm::Module* getLLVMModule() const {
        return LModule.get();
    }

private:
    bool m_IsAssignmentLHS = false;

    std::unordered_set<std::size_t> m_ResolvedList;
    std::size_t m_CurParentIndex = 0;

    std::stack<Type*>   m_LatestBoundType;
    std::stack<bool>    m_AssignmentLhsStack;
};
