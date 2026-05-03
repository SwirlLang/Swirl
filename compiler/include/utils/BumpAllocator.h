#pragma once
#include <memory>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cassert>
#include <type_traits>


namespace sw {

/// A BumpAllocator with stable-addressing.
class BumpAllocator {
public:
    /// `chunk_size`: the size of each individual memory chunk in bytes
    explicit BumpAllocator(const std::size_t chunk_size)
        : m_ChunkSize(chunk_size)
        , m_CurrentChunk(static_cast<Chunk*>(malloc(sizeof(Chunk) + chunk_size)))
    {
        m_CurrentChunk->offset = 0;
        m_CurrentChunk->capacity = m_ChunkSize;
        m_CurrentChunk->prev_chunk = nullptr;
    }


    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        static_assert(std::is_trivially_destructible_v<T>);
        assert(sizeof(T) <= m_ChunkSize);

        std::byte* cur_address  = m_CurrentChunk->data + m_CurrentChunk->offset;

        // calculate the next aligned address
        auto ptr_to_int   = reinterpret_cast<std::uintptr_t>(cur_address + alignof(T) - 1);
        auto aligned_addr = reinterpret_cast<std::byte*>(ptr_to_int & ~(alignof(T) - 1));

        const auto padding_size = aligned_addr - cur_address;

        // when the current chunk cannot satisfy the memory requirement
        if (cur_address + padding_size + sizeof(T) >= m_CurrentChunk->data + m_CurrentChunk->capacity) {
            // create a new chunk
            auto* new_chunk = static_cast<Chunk*>(malloc(sizeof(Chunk) + m_ChunkSize));
            new_chunk->offset = 0;
            new_chunk->capacity = m_ChunkSize;

            new_chunk->prev_chunk = m_CurrentChunk;
            m_CurrentChunk = new_chunk;

            return allocate<T>(std::forward<Args>(args)...);
        }

        m_CurrentChunk->offset += padding_size + sizeof(T);
        return std::construct_at(reinterpret_cast<T*>(aligned_addr), std::forward<Args>(args)...);
    }


    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;


    ~BumpAllocator() {
        Chunk* it = m_CurrentChunk;
        while (it != nullptr) {
            Chunk* prev = it->prev_chunk;
            free(it);
            it = prev;
        }
    }


private:
    struct Chunk {
        std::size_t offset;
        std::size_t capacity;

        Chunk* prev_chunk;
        std::byte data[];
    };

    const std::size_t m_ChunkSize;
    Chunk* m_CurrentChunk;
};
}