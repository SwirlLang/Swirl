#include <llvm/IR/DerivedTypes.h>
#include <types/SwTypes.h>
#include <backend/LLVMBackend.h>
#include <symbols/IdentManager.h>


// TODO: assign address spaces to types

llvm::Type* FunctionType::llvmCodegen(LLVMBackend& instance) {
    std::vector<llvm::Type*> llvm_param_types;
    for (const auto& ptr : this->param_types)
        llvm_param_types.push_back(ptr->llvmCodegen(instance));

    llvm::FunctionType* function = llvm::FunctionType::get(
        ret_type == nullptr ? llvm::Type::getVoidTy(instance.Context) : this->ret_type->llvmCodegen(instance),
        llvm_param_types,
        false
    );

    return function;
}

llvm::Type* TypeStr::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }

    auto lit_arr = llvm::ArrayType::get(llvm::Type::getInt8Ty(instance.Context), size);

    const auto ret = llvm::StructType::create(instance.Context, "__Str");
    ret->setBody({lit_arr});

    instance.LLVMTypeCache[this] = ret;
    return ret;
}


llvm::Type* StructType::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }

    std::vector<llvm::Type*> llvm_fields;
    llvm_fields.reserve(field_types.size());

    for (const auto& field : field_types) {
        llvm_fields.push_back(field->llvmCodegen(instance));
    }

    const auto struct_t = llvm::StructType::create(instance.Context, this->ident->toString());
    struct_t->setBody(llvm_fields);

    instance.LLVMTypeCache[this] = struct_t;
    return struct_t;
}

llvm::Type* PointerType::llvmCodegen(LLVMBackend& instance) {
    return llvm::PointerType::get(of_type->llvmCodegen(instance), 1);
}

llvm::Type* ReferenceType::llvmCodegen(LLVMBackend& instance) {
    return llvm::PointerType::get(of_type->llvmCodegen(instance), 1);
}

llvm::Type* ArrayType::llvmCodegen(LLVMBackend& instance) {
    if (instance.LLVMTypeCache.contains(this)) {
        return instance.LLVMTypeCache[this];
    }
    const auto arr_struct = llvm::StructType::create(instance.Context, "__Arr");
    arr_struct->setBody(llvm::ArrayType::get(of_type->llvmCodegen(instance), size));
    instance.LLVMTypeCache[this] = arr_struct;
    return arr_struct;
}


llvm::Type* TypeI8::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt8Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI32::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt32Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI16::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt16Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI64::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt64Ty(instance.Context);
    return ret;
}

llvm::Type* TypeI128::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt128Ty(instance.Context);
    return ret;
}

llvm::Type* TypeF32::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getFloatTy(instance.Context);
    return ret;
}

llvm::Type* TypeF64::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getDoubleTy(instance.Context);
    return ret;
}

llvm::Type* TypeBool::llvmCodegen(LLVMBackend& instance) {
    auto ret = llvm::Type::getInt1Ty(instance.Context);
    return ret;
}

