#pragma once
#include <memory>
#include <format>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cassert>
#include <stdexcept>


namespace sw {

/// A BumpAllocator with stable-addressing. Not thread-safe.
class BumpAllocator {
public:
    /// `chunk_size`: the size of each individual memory chunk in bytes
    explicit BumpAllocator(const std::size_t chunk_size)
        : m_ChunkSize(chunk_size)
        , m_CurrentChunk(static_cast<Chunk*>(malloc(sizeof(Chunk) + chunk_size)))
    {
        if (m_CurrentChunk == nullptr)
            throw std::bad_alloc();
        std::construct_at(m_CurrentChunk, m_ChunkSize, nullptr);
    }


    /// Allocates memory for `T` and constructs it in-place, returns the pointer to the memory.
    template <typename T, typename... Args>
    T* construct(Args&&... args) {
        // static_assert(std::is_trivially_destructible_v<T>);
        assert(sizeof(T) <= m_ChunkSize);

        std::byte* memory = allocate(sizeof(T), alignof(T));
        return std::construct_at(reinterpret_cast<T*>(memory), std::forward<Args>(args)...);
    }


    /// Returns a pointer to an aligned storage of `n` bytes
    std::byte* allocate(const std::size_t n, const std::size_t alignment = alignof(std::max_align_t)) {
        if (n > m_ChunkSize) {
            throw std::runtime_error(std::format
                ("BumpAllocator::allocate: n >= m_ChunkSize ({} >= {}).",
                    n, m_ChunkSize));
        }

        std::byte* addr = getAlignedAddress(n, alignment);

        if (!addr) {
            // create a new chunk
            const auto memory = malloc(sizeof(Chunk) + m_ChunkSize);
            if (!memory) {
                throw std::runtime_error("BumpAllocator::allocate: malloc returned null!");
            }

            auto* new_chunk = static_cast<Chunk*>(memory);
            std::construct_at(new_chunk, m_ChunkSize, m_CurrentChunk);

            m_CurrentChunk = new_chunk;
            addr = getAlignedAddress(n, alignment);

            if (!addr) {
                throw std::runtime_error(
                    "BumpAllocator::allocate: aligned address calculation failed.");
            }
        }

        // increment offset by n + padding size
        m_CurrentChunk->offset += n + (addr - (m_CurrentChunk->data() + m_CurrentChunk->offset));
        return addr;
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

        Chunk(const std::size_t size, Chunk* prev_chunk)
            : offset(0)
            , capacity(size)
            , prev_chunk(prev_chunk) {}

        [[nodiscard]]
        std::byte* data() {
            return reinterpret_cast<std::byte*>(this) + sizeof(Chunk);
        }
    };

    const std::size_t m_ChunkSize;
    Chunk* m_CurrentChunk;


    /// Returns `nullptr` if a new chunk is required to fit the data
    [[nodiscard]]
    std::byte* getAlignedAddress(const std::size_t n, const std::size_t alignment) const {
        std::size_t size_left = m_CurrentChunk->capacity - m_CurrentChunk->offset;
        void* pointer = m_CurrentChunk->data() + m_CurrentChunk->offset;

        return static_cast<std::byte*>(std::align(alignment, n, pointer, size_left));
    }
};
}