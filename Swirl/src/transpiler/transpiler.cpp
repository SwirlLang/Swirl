#include <iostream>
#include <parser/parser.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>


llvm::LLVMContext Context;
llvm::IRBuilder   Builder(Context);

auto LModule = std::make_unique<llvm::Module>("test", Context);


llvm::Value* IntLit::codegen() {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), value, 10);
}

llvm::Value* FloatLit::codegen() {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), value);
}

llvm::Value* StrLit::codegen() {
    return llvm::ConstantDataArray::getString(Context, value, true);
}

llvm::Value* Expression::codegen() {
    // TODO: support for unary operators

}

llvm::Value* FuncCall::codegen() {
    llvm::Function* func = LModule->getFunction(ident);
    if (!func)
        throw std::runtime_error("No such function defined");

    std::vector<llvm::Value*> arguments{};
    for ( auto& item : args)
        arguments.push_back(item.codegen());

    llvm::Value* ret = Builder.CreateCall(func, arguments, ident);
    return ret;
}

