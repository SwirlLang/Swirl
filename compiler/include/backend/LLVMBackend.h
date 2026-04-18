#pragma once
#include "ast/Nodes.h"
#include "definitions.h"
#include "types/SwTypes.h"
#include "types/definitions.h"
#include "symbols/SymbolManager.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Target/TargetMachine.h>


class ModuleManager;
class SymbolManager;

#define SW_LLVM_SET_CURRENT_PARENT(x) LLVMBackend::ParentSetter GET_UNIQUE_NAME(Par){this, x};


struct SwContext {
    Type* bound_type = nullptr;

    llvm::Value* bound_memory = nullptr;   // used by arrays to handle nesting
    llvm::BasicBlock* break_to = nullptr;
    llvm::BasicBlock* continue_to = nullptr;
};


class CGValue {
public:
    CGValue(): m_LValue(nullptr), m_RValue(nullptr) {}
    CGValue(llvm::Value* lvalue, llvm::Value* rvalue): m_LValue(lvalue), m_RValue(rvalue) {}

    [[nodiscard]]
    bool isLValue() const;

    [[nodiscard]]
    llvm::Value*   getLValue() const;
    llvm::Value*   getRValue(LLVMBackend&, const SwContext& context) const;

    static CGValue lValue(llvm::Value* lvalue);
    static CGValue rValue(llvm::Value* rvalue);

private:
    llvm::Value* m_LValue;
    llvm::Value* m_RValue;
};


class LLVMBackend {
public:
    llvm::LLVMContext LLVMContext;
    llvm::IRBuilder<> Builder{LLVMContext};

    ::ModuleManager& ModuleManager;
    SymbolManager& SymMan;

    std::unique_ptr<llvm::Module> LModule;
    std::unordered_map<Type*, llvm::Type*> LLVMTypeCache;

    inline static llvm::TargetMachine* TargetMachine = nullptr;

    llvm::Function* CurParent       = nullptr;
    llvm::Value*    RefMemory       = nullptr;  // set by the addr-taking op after computation
    llvm::Value*    ComputedPtr     = nullptr;  // set GEP operations like `[]` or `.`
    Type*           StructFieldType = nullptr;  // used to "bubble-up" struct-field types
    llvm::Value*    StructFieldPtr  = nullptr;  // used to "bubble-up" StructGEPd pointers


    LLVMBackend() = delete;
    explicit  LLVMBackend(Parser& parser);

    struct ParentSetter {
        ParentSetter(LLVMBackend* instance, llvm::Function* parent)
            : instance(instance) { instance->CurParent = parent; }

        ~ParentSetter() { instance->CurParent = nullptr; }

    private:
        LLVMBackend* instance;
    };

    /// calls `codegen` on all top AST nodes
    void dispatch(const AST_t& ast) {
        for (auto& node : ast) {
            codegen(node.get(), {});
        }
    }

    /// Calls the corresponding `llvmCodegen` on `node` and returns it
    CGValue codegen(Node* node, SwContext ctx) {
    #define SW_NODE(x, y) case x: return llvmCodegen(static_cast<y*>(node), std::move(ctx));
        switch (node->kind) {
            SW_NODE_LIST // NOLINT(*-pro-type-static-cast-downcast)
        } throw std::runtime_error("LLVMBackend::dispatch: unexpected node kind");
    #undef SW_NODE
    }


    CGValue codegen(const SwNode& node, const SwContext& ctx) {
        return codegen(node.get(), ctx);
    }


    llvm::Type* codegen(Type* type, SwContext ctx) {
    #define SW_TYPE(x, y) case Type::x: return llvmCodegen(static_cast<y*>(type), std::move(ctx));
        switch (type->kind) {
            SW_TYPE_LIST // NOLINT(*-pro-type-static-cast-downcast)
            default: throw std::runtime_error("LLVMBackend::dispatch: unexpected type tag");
        }
    #undef SW_TYPE
    }


    std::string mangleString(IdentInfo* id) const;

    /// Performs any necessary type casts (controlled by `BoundTypeState`), then returns the llvm::Value*
    /// note: `subject` is supposed to be a "loaded" value
    llvm::Value* castIfNecessary(Type* source_type, llvm::Value* subject, const SwContext& context);


    /// Codegens the vector of nodes while respecting statements like `return`. If `condition` is not
    /// nullptr, checks whether it is a `ConstantInt`, and if so, whether it is 0 (in which case no
    /// code generation takes place).
    void codegenChildrenUntilRet(const NodesVec& children, const SwContext& context, llvm::Value* condition = nullptr);

    llvm::Module* getLLVMModule() const {
        return LModule.get();
    }

    const llvm::DataLayout& getDataLayout() const {
        return LModule->getDataLayout();
    }

    /// Convenient function to construct a llvm int from a `size_t`
    llvm::Value* toLLVMInt(const std::size_t i, const int size_in_bits = 64) {
        switch (size_in_bits) {
            case 8   : return llvm::ConstantInt::get(llvm::Type::getInt8Ty(LLVMContext), i);
            case 16  : return llvm::ConstantInt::get(llvm::Type::getInt16Ty(LLVMContext), i);
            case 32  : return llvm::ConstantInt::get(llvm::Type::getInt32Ty(LLVMContext), i);
            case 64  : return llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLVMContext), i);
            case 128 : return llvm::ConstantInt::get(llvm::Type::getInt128Ty(LLVMContext), i);
            default  : throw std::runtime_error("LLVMBackend::toLLVMInt: invalid size_in_bits");
        }
    }


    /// Calls `print` on the llvm module
    void printIR() const {
        // bonus: run the llvm verifier on the module
        verifyModule(*LModule, &llvm::errs());
        LModule->print(llvm::outs(), nullptr);
    }


    Type* fetchSwType(Node* node) const {
        switch (node->getNodeType()) {
            case ND_STR:
                return &GlobalTypeStr;
            case ND_INT:
                return &GlobalTypeI32;
            case ND_FLOAT:
                return &GlobalTypeF64;
            case ND_BOOL:
                return &GlobalTypeBool;
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


    Type* fetchSwType(const SwNode& node) const {
        return fetchSwType(node.get());
    }


    bool isLocalScope() const {
        return CurParent != nullptr;
    }

    llvm::Function* getCurrentParent() const {
        return CurParent;
    }


    // --- --- llvmCodegen for Nodes --- --- //
    CGValue llvmCodegen(Node*, const SwContext&)           { return {}; }
    CGValue llvmCodegen(Enum*, const SwContext&)           { return {}; }
    CGValue llvmCodegen(ImportNode*, const SwContext&)     { return {}; }
    CGValue llvmCodegen(Protocol*, const SwContext&)       { return {}; }
    CGValue llvmCodegen(UndefinedValue*, const SwContext&) { return {}; }

    CGValue llvmCodegen(Expression* node, SwContext context);
    CGValue llvmCodegen(const IntLit* node, const SwContext &context);
    CGValue llvmCodegen(const FloatLit* node, const SwContext& context);
    CGValue llvmCodegen(Op* node, SwContext context);
    CGValue llvmCodegen(Var* node, const SwContext& context);
    CGValue llvmCodegen(const StrLit* node, const SwContext& context);
    CGValue llvmCodegen(FuncCall* node, const SwContext& context);
    CGValue llvmCodegen(Ident* node, const SwContext& context);
    CGValue llvmCodegen(Function* node, const SwContext& context);
    CGValue llvmCodegen(Condition* node, const SwContext& context);
    CGValue llvmCodegen(WhileLoop* node, SwContext context);
    CGValue llvmCodegen(Struct* node, const SwContext& context);
    CGValue llvmCodegen(ArrayLit* node, const SwContext& context);
    CGValue llvmCodegen(const TypeWrapper* node, const SwContext& context);
    CGValue llvmCodegen(BoolLit* node, SwContext context);
    CGValue llvmCodegen(Scope* node, const SwContext& context);
    CGValue llvmCodegen(BreakStmt* node, const SwContext& context);
    CGValue llvmCodegen(ContinueStmt* node, const SwContext& context);
    CGValue llvmCodegen(Intrinsic* node, const SwContext& context);
    CGValue llvmCodegen(ReturnStatement* node, const SwContext& context);


    // --- llvmCodegen for Types (implementation in SwTypes.cpp) --- //
    llvm::Type* llvmCodegen(FunctionType* type, const SwContext &context);
    llvm::Type* llvmCodegen(StructType* type, const SwContext &context);
    llvm::Type* llvmCodegen(EnumType* type, const SwContext &context);
    llvm::Type* llvmCodegen(TypeI8* type, SwContext context);
    llvm::Type* llvmCodegen(TypeI16* type, SwContext context);
    llvm::Type* llvmCodegen(TypeI32* type, SwContext context);
    llvm::Type* llvmCodegen(TypeI64* type, SwContext context);
    llvm::Type* llvmCodegen(TypeI128* type, SwContext context);
    llvm::Type* llvmCodegen(TypeU8* type, SwContext context);
    llvm::Type* llvmCodegen(TypeU16* type, SwContext context);
    llvm::Type* llvmCodegen(TypeU32* type, SwContext context);
    llvm::Type* llvmCodegen(TypeU64* type, SwContext context);
    llvm::Type* llvmCodegen(TypeU128* type, SwContext context);
    llvm::Type* llvmCodegen(TypeF32* type, SwContext context);
    llvm::Type* llvmCodegen(TypeF64* type, SwContext context);
    llvm::Type* llvmCodegen(TypeBool* type, SwContext context);
    llvm::Type* llvmCodegen(TypeStr* type, SwContext context);
    llvm::Type* llvmCodegen(ReferenceType* type, const SwContext& context);
    llvm::Type* llvmCodegen(PointerType* type, SwContext context);
    llvm::Type* llvmCodegen(ArrayType* type, const SwContext& context);
    llvm::Type* llvmCodegen(SliceType* type, const SwContext &context);
    llvm::Type* llvmCodegen(VoidType* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCInt* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCUInt* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCLL* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCULL* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCL* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCUL* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCSizeT* type, const SwContext& context);
    llvm::Type* llvmCodegen(TypeCSSizeT* type, SwContext context);
    llvm::Type* llvmCodegen(TypeChar* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCSChar* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCUChar* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCShort* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCUShort* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCBool* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCFloat* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCDouble* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCLDouble* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCIntPtr* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCUIntPtr* type, const SwContext& context);
    llvm::Type* llvmCodegen(TypeCPtrDiffT* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCIntMax* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCUIntMax* type, SwContext context);
    llvm::Type* llvmCodegen(TypeCWChar* type, SwContext context);
};
