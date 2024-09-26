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


    // ---------------[states]-------------------
    bool IsLocalScope;
    bool ChildHasReturned;

    llvm::Type*        FloatTypeState;
    llvm::IntegerType* IntegralTypeState;
    // ------------------------------------------


    explicit LLVMBackend(std::string mod_name)
        : Builder{Context}
        , LModule{std::make_unique<llvm::Module>(std::move(mod_name), Context)}
        , SymManager{Context}
        , IsLocalScope{false}
        , ChildHasReturned{false}
        , FloatTypeState{llvm::Type::getFloatTy(Context)}
        , IntegralTypeState{llvm::Type::getInt32Ty(Context)}
    {}
};
