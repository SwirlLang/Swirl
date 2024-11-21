#pragma once
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <ranges>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>


struct TableEntry {
    llvm::Value* ptr{};
    llvm::Type*  type{};

    bool is_type     = false;
    bool is_const    = false;
    bool is_param    = false;
    bool is_volatile = false;

    // contains the type that a declared LValue is bound to, needed for complex (non-primitive) types
    std::string bound_type;

    /// maps field-name to {index, type}, set to nullopt when ident is not an aggregate, where index is related to the
    /// position of the element in the aggregate
    std::optional<
        std::unordered_map<std::string, std::pair<std::size_t, std::string>
            >> fields = std::nullopt;
};


class SymbolManager {
    llvm::LLVMContext& m_Context;
    const int m_AddrSpace = 1;

     std::vector<std::unordered_map<std::string, TableEntry>> SymbolTable = {
         {
             {"i8", TableEntry{.is_type = true, .type = llvm::Type::getInt8Ty(m_Context)}},
             {"i32", TableEntry{.is_type = true, .type = llvm::Type::getInt32Ty(m_Context)}},
             {"i64", TableEntry{.is_type = true, .type = llvm::Type::getInt64Ty(m_Context)}},
             {"i128", TableEntry{.is_type = true, .type = llvm::Type::getInt128Ty(m_Context)}},
             {"f32", TableEntry{.is_type = true, .type = llvm::Type::getFloatTy(m_Context)}},
             {"f64", TableEntry{.is_type = true, .type = llvm::Type::getDoubleTy(m_Context)}},
             {"bool", TableEntry{.is_type = true, .type = llvm::Type::getInt1Ty(m_Context)}},
             {"void", TableEntry{.is_type = true, .type = llvm::Type::getVoidTy(m_Context)}},

             {"i8*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getInt8Ty(m_Context), m_AddrSpace)}},
             {"i32*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getInt32Ty(m_Context), m_AddrSpace)}},
             {"i64*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getInt64Ty(m_Context), m_AddrSpace)}},
             {"i128*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getInt128Ty(m_Context), m_AddrSpace)}},
             {"f32*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getFloatTy(m_Context), m_AddrSpace)}},
             {"f64*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getDoubleTy(m_Context), m_AddrSpace)}},
             {"bool*", TableEntry{.is_type = true, .type = llvm::PointerType::get(llvm::Type::getInt1Ty(m_Context), m_AddrSpace)}},
         }
     };


public:
    explicit SymbolManager(llvm::LLVMContext& ctx): m_Context(ctx) {}

    TableEntry lookupType(const std::string& str) {
        if (SymbolTable.front().contains(str))
            return SymbolTable.front().at(str);

        for (std::unordered_map entry : SymbolTable | std::views::drop(1) | std::views::reverse) {
            if (entry.contains(str))
                return entry.at(str);
        } throw std::runtime_error("TypeRegistry: cannot resolve type: " + str);
    }


    void registerGlobalType(const std::string& ident, TableEntry&& entry) {
        SymbolTable.front()[ident] = entry;
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
