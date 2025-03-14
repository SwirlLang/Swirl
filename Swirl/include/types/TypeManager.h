#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <utils/utils.h>


struct Type;
class  IdentInfo;

struct ParsedType {
    IdentInfo* ident = nullptr;
    bool       is_reference = false;
    uint16_t   pointer_level = false;

    bool operator==(const ParsedType& other) const {
        return other.ident == ident && other.pointer_level == pointer_level && other.is_reference == is_reference;
    }
};


template <>
struct std::hash<ParsedType> {
    std::size_t operator()(const ParsedType& pt) const noexcept {
        return combineHashes(
            std::hash<bool>{}(pt.is_reference),
            std::hash<uint16_t>{}(pt.pointer_level),
            std::hash<IdentInfo*>{}(pt.ident)
            );
    }
};


class TypeManager {
    std::unordered_map<ParsedType, std::unique_ptr<Type>> m_TypeTable;

public:
    Type* getFor(const ParsedType& type) {
        if (const auto it = m_TypeTable.find(type); it != m_TypeTable.end())
            return it->second.get();
        throw std::invalid_argument("TypeManager::getFor: No such type");
    }

    void remove(const ParsedType& type) {
       m_TypeTable.erase(type);
    }

    /// registers a new association between a `ParsedType` and swirl-Type-derived instance
    void registerType(const ParsedType& pt, Type* t) {
        if (m_TypeTable.contains(pt))
            throw std::runtime_error("TypeManager::registerType: Duplicate type registration request!");
        m_TypeTable[pt] = std::unique_ptr<Type>(t);
    }

    bool contains(IdentInfo* type) {
        return m_TypeTable.contains({type});
    }

    bool contains(const ParsedType& t) {
        return m_TypeTable.contains(t);
    }
};