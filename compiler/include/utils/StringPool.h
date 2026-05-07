#pragma once
#include <cstdlib>
#include <cstring>
#include <unordered_set>

#include "BumpAllocator.h"


namespace sw {

/// A trivial string pool. Not thread-safe.
class StringPool {
public:
    explicit StringPool(const std::size_t chunk_size): m_BumpAllocator(chunk_size) {}

    std::string_view intern(const std::string_view str) {
        if (const auto a = m_Strings.find(str); a != m_Strings.end())
            return *a;

        const auto memory = reinterpret_cast<char*>(m_BumpAllocator.allocate(str.size()));
        memcpy(memory, str.data(), str.size());

        const auto ret = std::string_view{memory, str.size()};
        m_Strings.insert(ret);
        return ret;
    }


private:
    BumpAllocator m_BumpAllocator;
    std::unordered_set<std::string_view> m_Strings;
};
}