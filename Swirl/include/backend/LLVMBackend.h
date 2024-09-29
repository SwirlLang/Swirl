#pragma once
#include <memory>
#include <Managers/SymbolManager.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>


class LLVMBackend {
public:
    llvm::LLVMContext Context;
    llvm::IRBuilder<> Builder;

    std::unique_ptr<llvm::Module> LModule;
    SymbolManager SymManager;

    // ----------------[states]-------------------
    bool IsLocalScope;
    bool ChildHasReturned;

    llvm::Type*        FloatTypeState;
    llvm::IntegerType* IntegralTypeState;
    // ------------------------------------------


    explicit LLVMBackend(const std::string& mod_name)
        : Builder{Context}
        , LModule{std::make_unique<llvm::Module>(mod_name, Context)}
        , SymManager{Context}
        , IsLocalScope{false}
        , ChildHasReturned{false}
        , FloatTypeState{llvm::Type::getFloatTy(Context)}
        , IntegralTypeState{llvm::Type::getInt32Ty(Context)}
    {}

    void setIntegralTypeState(llvm::Type* state) {
        m_Cache = IntegralTypeState;
        IntegralTypeState = llvm::dyn_cast<llvm::IntegerType>(state);
    }

    void restoreIntegralTypeState() {
        IntegralTypeState = llvm::dyn_cast<llvm::IntegerType>(m_Cache);
    }

    void setFloatTypeState(llvm::Type* state) {
        m_Cache = FloatTypeState;
        FloatTypeState = state;
    }

    void restoreFloatTypeState() {
        FloatTypeState = m_Cache;
    }

    llvm::IntegerType* getIntegralTypeState() const {
        return IntegralTypeState;
    }

    llvm::Type* getFloatTypeState() const {
        return FloatTypeState;
    }

private:
    llvm::Type* m_Cache;
};
