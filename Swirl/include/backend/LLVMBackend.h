#pragma once
#include <memory>

#include <managers/SymbolManager.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>


class LLVMBackend {
public:
    llvm::LLVMContext Context;
    llvm::IRBuilder<> Builder;

    std::unique_ptr<llvm::Module> LModule;
    SymbolManager SymManager;

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


    explicit LLVMBackend(const std::string& mod_name, SymbolManager sym_man)
            : Builder{Context}
            , LModule{std::make_unique<llvm::Module>(mod_name, Context)}
            , SymManager(std::move(sym_man))
            , FloatTypeState{llvm::Type::getFloatTy(Context)}
            , IntegralTypeState{llvm::Type::getInt32Ty(Context)} {}


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
};
