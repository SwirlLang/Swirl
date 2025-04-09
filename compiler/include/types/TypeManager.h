#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <utils/utils.h>


struct Type;
class  IdentInfo;


namespace detail {
struct Pointer {
    Type* of_type = nullptr;
    uint16_t level = 0;

    bool operator==(const Pointer& other) const {
        return level == other.level && of_type == other.of_type;
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

class TypeManager {
    std::unordered_map<IdentInfo*, std::unique_ptr<Type>>              m_TypeTable;  // for named types
    std::unordered_map<Type*, std::unique_ptr<ReferenceType>>          m_ReferenceTable;
    std::unordered_map<detail::Pointer, std::unique_ptr<PointerType>>  m_PointerTable;

    friend std::hash<detail::Pointer>;

public:
    /// returns the type with the id `name`
    Type* getFor(IdentInfo* name) {
        if (const auto it = m_TypeTable.find(name); it != m_TypeTable.end())
            return it->second.get();
        throw std::invalid_argument("TypeManager::getFor: No such type");
    }

    void registerType(IdentInfo* name, Type* type) {
        if (m_TypeTable.contains(name))
            throw std::runtime_error("TypeManager::registerType: Duplicate type registration request!");
        m_TypeTable[name] = std::unique_ptr<Type>(type);
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

    /// returns a reference for the type `to`
    Type* getReferenceType(Type* to) {
        if (m_ReferenceTable.contains(to)) {
            return m_ReferenceTable[to].get();
        } m_ReferenceTable[to] = std::make_unique<ReferenceType>(to);
        return m_ReferenceTable[to].get();
    }

    bool contains(IdentInfo* name) const {
        return m_TypeTable.contains(name);
    }
};
