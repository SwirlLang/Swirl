#include <iostream>
#include <llvm/Support/ErrorHandling.h>
#include <parser/parser.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/SourceMgr.h>
#include <string>
#include <string_view>

std::size_t ScopeDepth = 0;

extern std::string SW_OUTPUT;
extern std::vector<std::unique_ptr<Node>> ParsedModule;

llvm::LLVMContext Context;
llvm::IRBuilder<> Builder(Context);

auto LModule = std::make_unique<llvm::Module>(SW_OUTPUT, Context);
bool IsLocalScope = false;

std::unordered_map<std::string, llvm::Type*> type_registry = {
        {"i64",   llvm::Type::getInt64Ty(Context)},
        {"i32",   llvm::Type::getInt32Ty(Context)},
        {"i128",  llvm::Type::getInt128Ty(Context)},
        {"f32", llvm::Type::getFloatTy(Context)},
        {"bool",  llvm::Type::getInt1Ty(Context)},
        {"void",  llvm::Type::getVoidTy(Context)}
};

llvm::IntegerType* IntegralTypeState = llvm::Type::getInt32Ty(Context);

struct BeginScope {
    BeginScope()  { IsLocalScope = true; }
    ~BeginScope() { IsLocalScope = false; }
};


llvm::Value* IntLit::codegen() {
    return llvm::ConstantInt::get(IntegralTypeState, getValue(), 10);
}

llvm::Value* FloatLit::codegen() {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), value);
}

llvm::Value* StrLit::codegen() {
    return llvm::ConstantDataArray::getString(Context, value, true);
}

llvm::Value* Ident::codegen() {

}

void printIR() {
    LModule->print(llvm::outs(), nullptr);
    llvm::outs().flush();
}


llvm::Value* Function::codegen() {
    std::vector<llvm::Type*> param_types;

    param_types.reserve(params.size());
    for (const auto& param : params)
        param_types.push_back(type_registry[param.var_type]);

    llvm::FunctionType* f_type = llvm::FunctionType::get(type_registry[this->ret_type], param_types, false);
    llvm::Function*     func   = llvm::Function::Create(f_type, llvm::GlobalValue::InternalLinkage, ident, LModule.get());

    llvm::BasicBlock*   BB     = llvm::BasicBlock::Create(Context, "", func);
    Builder.SetInsertPoint(BB);
    BeginScope _;

    for (const auto& child : this->children)
        child->codegen();

    if (!return_val.expr.empty()) {
        Builder.CreateRet(return_val.codegen());
    }
    else Builder.CreateRetVoid();

    return func;
}


llvm::Value* Expression::codegen() {
    std::stack<llvm::Value*> eval{};
    static std::unordered_map<std::string, std::function<llvm::Value*(llvm::Value*, llvm::Value*)>> op_table
    = {
            // Standard Arithmetic Operators
        {"+", [](llvm::Value* a, llvm::Value* b) { return Builder.CreateAdd(a, b); }},
        {"-", [](llvm::Value* a, llvm::Value* b) { return Builder.CreateSub(a, b); }},
        {"*", [](llvm::Value* a, llvm::Value* b) { return Builder.CreateMul(a, b); }},
        {"/",
            [](llvm::Value *a, llvm::Value *b) {
                if (a->getType()->isIntegerTy())
                    a = Builder.CreateSIToFP(a, llvm::Type::getFloatTy(Context));
                if (b->getType()->isIntegerTy()) {
                    b = Builder.CreateSIToFP(b, llvm::Type::getFloatTy(Context));
                }
                return Builder.CreateFDiv(a, b);
            },

        },

        // Boolean operators
        {"==", [](llvm::Value* a, llvm::Value* b) {
            if (a->getType()->isIntegerTy() || b->getType()->isIntegerTy())
                return Builder.CreateICmp(llvm::CmpInst::ICMP_EQ, a, b);

            if (a->getType()->isFloatingPointTy() && b->getType()->isFloatingPointTy())
                return Builder.CreateFCmp(llvm::CmpInst::FCMP_OEQ, a, b);

            throw std::runtime_error("undefined operation '==' on the type");
        }}
    };

    for (const std::unique_ptr<Node>& nd : expr) {
        if (nd->getType() != ND_OP) {
            eval.push(nd->codegen());
        }
        else {
            if (nd->getArity() == 2) {
                if (eval.size() < 2) { throw std::runtime_error("Not enough operands on eval stack"); }
                llvm::Value* op2 = eval.top();
                eval.pop();
                llvm::Value* op1 = eval.top();
                eval.pop();
                eval.push(op_table[nd->getValue()](op1, op2));
            }
        }
    }
    if (eval.empty()) throw std::runtime_error("expr-codegen: eval stack is empty");
    return eval.top();
}

llvm::Value* Condition::codegen() {
    auto if_block = llvm::BasicBlock::Create(Context);
    auto else_block = llvm::BasicBlock::Create(Context);

    // Builder.CreateCondBr(this->bool_expr.codegen(), if_block, else_block);
    // Builder.SetInsertPoint(if_block);
}


llvm::Value* FuncCall::codegen() {
    llvm::Type* returnType = type_registry[type];
    std::vector<llvm::Type*> paramTypes;
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, paramTypes, false);
    llvm::Function* myFunction = llvm::Function::Create(functionType, llvm::GlobalValue::InternalLinkage, "c", LModule.get());

    llvm::Function* func = LModule->getFunction(ident);
    if (!func)
        throw std::runtime_error("No such function defined");

    std::vector<llvm::Value*> arguments{};
    arguments.reserve(args.size());

    for (auto& item : args) {
        arguments.push_back(item.codegen());
    }

    llvm::Value* ret = Builder.CreateCall(func, arguments, ident);
    return ret;
}

llvm::Value* Var::codegen() {
    auto type_iter = type_registry.find(var_type);
    if (type_iter == type_registry.end()) throw std::runtime_error("undefined type");

    if (type_iter->second->isIntegerTy())
        IntegralTypeState = llvm::dyn_cast<llvm::IntegerType>(type_iter->second);

    llvm::Value* ret;
    llvm::Value* init = nullptr;

    if (initialized)
        init = value.codegen();

    if (!IsLocalScope) {
        auto* var = new llvm::GlobalVariable(
                *LModule, type_iter->second, is_const, llvm::GlobalVariable::InternalLinkage,
                nullptr, var_ident
                );

        if (init) {
            auto* val = llvm::dyn_cast<llvm::Constant>(init);
            var->setInitializer(val);
        }

        ret = var;
    } else {
        llvm::AllocaInst* var_alloca = Builder.CreateAlloca(type_iter->second, nullptr, var_ident);
        // * is_volatile is false
        if (init) Builder.CreateStore(init, var_alloca);
        ret = var_alloca;
    }

    return ret;
}
