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
llvm::IRBuilder<> Builder(Context);

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
    std::stack<llvm::Value*> eval{};
    static std::unordered_map<std::string, std::function<llvm::Value*(llvm::Value*, llvm::Value*)>> op_table = {
            {"+", [](llvm::Value* a, llvm::Value* b) { return Builder.CreateAdd(a, b); }},
            {"-", [](llvm::Value* a, llvm::Value* b) { return Builder.CreateSub(a, b); }},
            {"*", [](llvm::Value* a, llvm::Value* b) { return Builder.CreateMul(a, b); }},
            {"/",
             [](llvm::Value* a, llvm::Value* b) {
                if (a->getType()->isIntegerTy())
                    a = Builder.CreateSIToFP(a, llvm::Type::getFloatTy(Context));
                if (b->getType()->isIntegerTy()) {
                    b = Builder.CreateSIToFP(b, llvm::Type::getFloatTy(Context));
                return Builder.CreateFDiv(a, b);
                }
            }
            }
    };

    for (const std::unique_ptr<Node>& nd : expr) {
        if (nd->getType() != ND_OP)
            eval.push(nd->codegen());
        else if (nd->getType() == ND_OP) {
            if (nd->getArity() == 2) {
                if (eval.size() < 2) { throw std::runtime_error("Not enough operands on eval stack"); }
                llvm::Value* op1 = eval.top();
                eval.pop();
                llvm::Value* op2 = eval.top();
                eval.pop();

                eval.push(op_table[nd->getValue()](op1, op2));
            }
        }
    }
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

