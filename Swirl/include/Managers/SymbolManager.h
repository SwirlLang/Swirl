#pragma once
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <ranges>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>


struct TableEntry {
    llvm::Value* ptr{};
    llvm::Type* type{};

    bool is_const    = false;
    bool is_param    = false;
    bool is_volatile = false;

    // maps field-name to {index, type}, set to nullopt when ident is not a struct
    std::optional<
        std::unordered_map<std::string, std::pair<std::size_t, llvm::Type*>
            >> fields = std::nullopt;
};


class SymbolManager {
    llvm::LLVMContext& m_Context;

    std::unordered_map<std::string, llvm::Type*> type_registry = {
        {"", nullptr},
        {"i8", llvm::Type::getInt8Ty(m_Context)},
        {"i32",   llvm::Type::getInt32Ty(m_Context)},
        {"i64",   llvm::Type::getInt64Ty(m_Context)},
        {"i128",  llvm::Type::getInt128Ty(m_Context)},
        {"f32",   llvm::Type::getFloatTy(m_Context)},
        {"f64",   llvm::Type::getDoubleTy(m_Context)},
        {"bool",  llvm::Type::getInt1Ty(m_Context)},
        {"void",  llvm::Type::getVoidTy(m_Context)},

        {"i8*", llvm::PointerType::getInt8Ty(m_Context)},
        {"i32*", llvm::PointerType::getInt32Ty(m_Context)},
        {"i64*", llvm::PointerType::getInt64Ty(m_Context)},
        {"i128*", llvm::PointerType::getInt128Ty(m_Context)},
        {"f32*", llvm::PointerType::getFloatTy(m_Context)},
        {"f64*", llvm::PointerType::getDoubleTy(m_Context)},
        {"bool*", llvm::PointerType::getInt1Ty(m_Context)}
    };

     std::vector<std::unordered_map<std::string, TableEntry>> SymbolTable = {{}};


public:
    explicit SymbolManager(llvm::LLVMContext& ctx): m_Context(ctx) {}

    llvm::Type* lookupType(const std::string& str) {
        if (type_registry.contains(str))
            return type_registry[str];

        if (str.ends_with("*")) {
            if (
                const auto base_type = str.substr(0, str.find_first_of('*') + 1);
                type_registry.contains(base_type)
                )
            {
                llvm::Type* llvm_base_type = type_registry[base_type];
                llvm::Type* ptr_type       = llvm_base_type;

                std::size_t ptr_levels = std::ranges::count(str, '*');
                while (ptr_levels--) {
                    ptr_type = llvm::PointerType::get(ptr_type, 1);
                } return ptr_type;
            }
        } else {
            // TODO struct types non-global scopes
        }

        throw std::runtime_error("TypeRegistry: cannot resolve type: " + str);
    }


    void registerGlobalType(const std::string& ident, llvm::Type* type) {
        type_registry[ident] = type;
    }

    TableEntry& lookupSymbol(const std::string& ident) {
        for (auto& e : SymbolTable | std::views::reverse) {
            if (e.contains(ident))
                return e[ident];
        } throw std::runtime_error("SymbolManager: cannot resolve symbol: " + ident);
    }

    void emplace_back() {
        SymbolTable.emplace_back();
    }

    void pop_back() {
        SymbolTable.pop_back();
    }

    std::unordered_map<std::string, TableEntry>& back() {
        return SymbolTable.back();
    }
};