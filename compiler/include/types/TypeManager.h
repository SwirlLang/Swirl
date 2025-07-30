#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <definitions.h>
#include <utils/utils.h>

#include "SwTypes.h"


struct Type;
class  IdentInfo;


namespace detail {
struct Pointer {
    Type* of_type = nullptr;
    uint16_t level = 1;
    bool is_mutable = false;  // does it point to a mutable object?

    bool operator==(const Pointer& other) const {
        return level == other.level && of_type == other.of_type;
    }
};

struct Array {
    Type* of_type = nullptr;
    std::size_t size = 0;

    bool operator==(const Array& other) const {
        return size == other.size && of_type == other.of_type;
    }
};

struct Deleter {
    void operator()(const Type* ptr) const {
        for (const auto& val: BuiltinTypes | std::views::values) {
            if (val == ptr)
                return;
        } delete ptr;
    }
};
}


template <>
struct std::hash<detail::Pointer> {
    std::size_t operator()(const detail::Pointer ptr) const noexcept {
        return combineHashes(
            std::hash<uint16_t>{}(ptr.level),
            std::hash<Type*>{}(ptr.of_type)
        );
    }
};


template <>
struct std::hash<detail::Array> {
    std::size_t operator()(const detail::Array ptr) const noexcept {
        return combineHashes(
            std::hash<Type*>{}(ptr.of_type),
            std::hash<std::size_t>{}(ptr.size)
        );
    }
};


/// Factory class for all children of `Type`
class TypeManager {
    using Str_t = std::size_t;
    std::unordered_map<IdentInfo*, std::unique_ptr
        <Type, detail::Deleter>>                          m_TypeTable;  // for named types
    std::unordered_map<Str_t, std::unique_ptr<TypeStr>>   m_StringTable;

    std::unordered_map<detail::Array, std::unique_ptr<ArrayType>>         m_ArrayTable;
    std::unordered_map<detail::Pointer, std::unique_ptr<PointerType>>     m_PointerTable;
    std::unordered_map<detail::Pointer, std::unique_ptr<ReferenceType>>   m_ReferenceTable;
    std::unordered_map<detail::Array, std::unique_ptr<SliceType>>         m_SliceTable;


public:
    /// returns the type with the id `name`, nullptr otherwise
    Type* getFor(IdentInfo* name) {
        if (const auto it = m_TypeTable.find(name); it != m_TypeTable.end())
            return it->second.get();
        return nullptr;
    }

    void registerType(IdentInfo* name, Type* type) {
        if (m_TypeTable.contains(name))
            throw std::runtime_error("TypeManager::registerType: Duplicate type registration request!");
        m_TypeTable[name] = std::unique_ptr<Type, detail::Deleter>(type);
    }

    /// returns a pointer for the type `to` of the level `level`
    Type* getPointerType(Type* to, uint16_t level) {
        using namespace detail;
        const Pointer ptr{to, level};
        if (m_PointerTable.contains(ptr)) {
            return m_PointerTable[ptr].get();
        } m_PointerTable[ptr] = std::make_unique<PointerType>(to, level);
        return m_PointerTable[ptr].get();
    }

    /// returns the corresponding array-type for the type and size
    Type* getArrayType(Type* of_type, std::size_t size) {
        using namespace detail;
        const Array arr{of_type, size};
        if (m_ArrayTable.contains(arr)) {
            return m_ArrayTable[arr].get();
        } m_ArrayTable[arr] = std::make_unique<ArrayType>(of_type, size);
        return m_ArrayTable[arr].get();
    }

    /// returns a reference for the type `to`
    Type* getReferenceType(Type* to, const bool is_mutable) {
        if (to->getTypeTag() == Type::REFERENCE && to->is_mutable == is_mutable)
            return to;  // reference collapsing, & + & = &

        const detail::Pointer obj{.of_type = to, .is_mutable = is_mutable};
        if (m_ReferenceTable.contains(obj))
            return m_ReferenceTable[obj].get();

        m_ReferenceTable[obj] = std::make_unique<ReferenceType>(to);
        m_ReferenceTable[obj]->is_mutable = is_mutable;
        return m_ReferenceTable[obj].get();
    }

    Type* getStringType(const std::size_t size) {
        if (m_StringTable.contains(size)) {
            return m_StringTable[size].get();
        } m_StringTable[size] = std::make_unique<TypeStr>(size);
        return m_StringTable[size].get();
    }

    /// returns a slice type instance pointer for the given array type
    Type* getSliceType(Type* array_type) {
        const auto arr_type = dynamic_cast<ArrayType*>(array_type);
        assert(arr_type != nullptr);

        auto slice_entry = detail::Array{arr_type->of_type, arr_type->size};
        if (m_SliceTable.contains(slice_entry)) {
            return m_SliceTable[slice_entry].get();
        } m_SliceTable[slice_entry] =
            std::make_unique<SliceType>(slice_entry.of_type, slice_entry.size);
        return m_SliceTable[slice_entry].get();
    }


    bool contains(IdentInfo* name) const {
        return m_TypeTable.contains(name);
    }
};
