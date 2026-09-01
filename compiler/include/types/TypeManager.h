#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "SwTypes.h"
#include "utils/utils.h"
#include "comptime/Value.h"
#include "types/definitions.h"
#include "modules/ModuleManager.h"
#include "symbols/IdentManager.h"


struct Type;
class  IdentInfo;

namespace detail {
struct Pointer {
    Type* of_type    = nullptr;
    bool  is_mutable = false;  // does it point to a mutable object?

    bool operator==(const Pointer& other) const {
        return is_mutable == other.is_mutable && of_type == other.of_type;
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
        for (const auto& val : BuiltinTypes | std::views::values) {
            if (val == ptr)
                return;
        } delete ptr;
    }
};


/// Template class for creating a Sharded Interner, where N is the no. of shards.
template <std::size_t N, typename Key, typename Value>
class TypeInterner {
public:
    Value& intern(const std::size_t index, Key key, Value&& value) {
        assert(index < N);
        auto& entry = m_Entries.at(index);
        typename Map_t::Guard guard(entry);

        auto [it, _] = entry.map.try_emplace(
            std::forward<Key>(key),
            std::forward<Value>(value));
        return it->second;
    }

    bool contains(const std::size_t index, Key key) {
        assert(index < N);
        auto& entry = m_Entries.at(index);
        typename Map_t::Guard guard(entry);
        return entry.map.contains(key);
    }

    Value& get(const std::size_t index, Key key) {
        assert(index < N);
        auto& entry = m_Entries.at(index);
        typename Map_t::Guard guard(entry);
        return entry.map.at(key);
    }

private:
    struct Map_t {
        std::unordered_map<Key, Value> map;
        std::mutex mutex;

        void lock()   { mutex.lock(); }
        void unlock() { mutex.unlock(); }

        struct Guard {
            Map_t& map;
            Guard(Map_t& map)
            : map(map) { map.lock();   }
            ~Guard()   { map.unlock(); }
        };
    };

    std::array<Map_t, N> m_Entries;
};


struct InstKey {
    using Args_t = std::vector<std::variant<std::monostate, Type*, sw::Value>>;

    IdentInfo* id = nullptr;
    Args_t args{};

    bool operator==(const InstKey& other) const {
        return id == other.id && args == other.args;
    }
};
}


template <>
struct std::hash<detail::Pointer> {
    std::size_t operator()(const detail::Pointer ptr) const noexcept {
        return combineHashes(
            std::hash<uint16_t>{}(ptr.is_mutable),
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


template <>
struct std::hash<detail::InstKey> {
    std::size_t operator()(const detail::InstKey& key) const noexcept {
        std::size_t vec_hash = 0;

        for (auto& arg : key.args) {
            vec_hash ^= std::hash<detail::InstKey::Args_t::value_type>{}(arg);
        } return combineHashes(std::hash<IdentInfo*>{}(key.id), vec_hash);
    }
};


namespace sw {
class TypeManager {
public:
    static constexpr std::size_t TypeShardsSize         = 32;
    static constexpr std::size_t ArrayShardsSize        = 32;
    static constexpr std::size_t SliceShardsSize        = 32;
    static constexpr std::size_t PointerShardsSize      = 32;
    static constexpr std::size_t ReferenceShardsSize    = 32;
    static constexpr std::size_t GenericCacheShardsSize = 32;

    explicit
    TypeManager(ModuleManager& mod_man)
        : m_ModMan(mod_man) {}


    void registerType(IdentInfo* ident, Type* type) {
        const auto index = m_ModMan.getModuleIndex(ident->getModuleFileHandle()) % TypeShardsSize;
        m_TypeInterner.intern(index, ident, std::unique_ptr<Type, detail::Deleter>{type});
        type->location.source = ident->getModuleFileHandle();
    }

    Type* getPointerType(Type* to, bool is_mutable = true) {
        is_mutable = true;  // (temporarily disabled until mutability analysis is stable)
        const detail::Pointer key{.of_type = to, .is_mutable = is_mutable};
        const auto index = m_ModMan.getModuleIndex(to->location.source) % PointerShardsSize;

        auto new_ty = std::make_unique<PointerType>(to, is_mutable);
        return m_PointerInterner.intern(index, key, std::move(new_ty)).get();
    }

    Type* getArrayType(Type* of, const std::size_t size) {
        const detail::Array key{.of_type = of, .size = size};
        const auto index = m_ModMan.getModuleIndex(of->location.source) % ArrayShardsSize;

        auto new_ty = std::make_unique<ArrayType>(of, size);
        return m_ArrayInterner.intern(index, key, std::move(new_ty)).get();
    }

    /// returns the slice type instance pointer for the given type, note that the type
    /// is supposed to be what's within the target array.
    Type* getSliceType(Type* of, bool is_mutable = true) {
        is_mutable = true;  // (temporarily disabled until mutability analysis is stable)
        const detail::Pointer key{.of_type = of, .is_mutable = is_mutable};
        const auto index = m_ModMan.getModuleIndex(of->location.source) % SliceShardsSize;

        auto new_ty = std::make_unique<SliceType>(of);
        return m_SliceInterner.intern(index, key, std::move(new_ty)).get();
    }

    Type* getReferenceType(Type* to, bool is_mutable = true) {
        is_mutable = true;  // (temporarily disabled until mutability analysis is stable)
        const detail::Pointer key{.of_type = to, .is_mutable = is_mutable};
        if (to->getTypeTag() == Type::ARRAY) {
            return getSliceType(to, is_mutable);
        }

        // collapse the reference of a reference
        if (to->isReferenceType() && to->is_mutable == is_mutable) {
            return to;
        }

        const auto index = m_ModMan.getModuleIndex(to->location.source) % ReferenceShardsSize;

        auto new_ty = std::make_unique<ReferenceType>(to);
        return m_ReferenceInterner.intern(index, key, std::move(new_ty)).get();
    }

    Type* lookupType(IdentInfo* ident) {
        const auto index = m_ModMan.getModuleIndex(ident->getModuleFileHandle()) % TypeShardsSize;
        return m_TypeInterner.get(index, ident).get();
    }

    /// Returns the cached `IdentInfo*` for the generic-inst key, returns `nullptr` if it
    /// doesn't exist.
    IdentInfo* lookupGenericCache(const detail::InstKey& key) {
        const auto index = m_ModMan.getModuleIndex(key.id->getModuleFileHandle()) % GenericCacheShardsSize;
        if (m_GenericCache.contains(index, key)) {
            return m_GenericCache.get(index, key);
        } return nullptr;
    }

    /// Returns true if the cache submission was successful, false otherwise. The caller
    /// must discard its result if this method returns false and recheck the cache.
    bool submitToGenericCache(const detail::InstKey& key, IdentInfo* ident) {
        const auto index = m_ModMan.getModuleIndex(key.id->getModuleFileHandle()) % GenericCacheShardsSize;
        return m_GenericCache.intern(index, key, std::move(ident)) == ident;
    }

    bool contains(IdentInfo* ident) {
        const auto index = m_ModMan.getModuleIndex(ident->getModuleFileHandle()) % TypeShardsSize;
        return m_TypeInterner.contains(index, ident);
    }


private:
    template <std::size_t N, typename Key, typename Value>
    using TypeInterner = detail::TypeInterner<N, Key, Value>;

    TypeInterner<ArrayShardsSize,        detail::Array,   std::unique_ptr<Type>> m_ArrayInterner;
    TypeInterner<SliceShardsSize,        detail::Pointer, std::unique_ptr<Type>> m_SliceInterner;
    TypeInterner<PointerShardsSize,      detail::Pointer, std::unique_ptr<Type>> m_PointerInterner;
    TypeInterner<ReferenceShardsSize,    detail::Pointer, std::unique_ptr<Type>> m_ReferenceInterner;
    TypeInterner<GenericCacheShardsSize, detail::InstKey, IdentInfo*> m_GenericCache;

    TypeInterner<TypeShardsSize, IdentInfo*, std::unique_ptr<Type, detail::Deleter>> m_TypeInterner;

    ModuleManager& m_ModMan;
};
}  // namespace sw