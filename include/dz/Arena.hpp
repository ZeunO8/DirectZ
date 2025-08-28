#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <vector>
#include <unordered_map>

namespace dz
{
    template<size_t size>
    struct ArenaChunk
    {
        size_t used;
        uint8_t *data;

        ArenaChunk()
        {
            used = 0;
            data = (uint8_t *)malloc(size);
            memset(data, 0, size);
            assert(data != nullptr);
        }

        ~ArenaChunk()
        {
            free(data);
        }

        size_t remaining() const
        {
            return ;
        }
    };

    template<size_t chunk_size>
    struct Arena
    {
        std::vector<ArenaChunk<chunk_size> *> chunks;
        std::unordered_map<size_t, std::vector<void*>> free_spots;
        bool track_free_blocks;
        ArenaChunk<chunk_size>* back_chunk = nullptr;

        Arena(bool track_free_blocks = false):
            track_free_blocks(track_free_blocks)
        {
            chunks.reserve(128);
        }

        inline size_t chunks_size()
        {
            return chunks.size();
        }

        inline size_t memory_used()
        {
            return chunk_size * chunks.size();
        }

        ~Arena()
        {
            for (auto chunk : chunks)
            {
                delete chunk;
            }
        }

        void *arena_malloc(size_t sz)
        {
            if (track_free_blocks)
            {
                auto free_it = free_spots.find(sz);
                if (free_it != free_spots.end())
                {
                    auto& free_vec = free_it->second;
                    auto free_ptr = free_vec.front();
                    free_vec.erase(free_vec.end() - 1);
                    if (free_vec.empty())
                    {
                        free_spots.erase(free_it);
                    }
                    return free_ptr;
                }
            }
            if (!back_chunk || (chunk_size - back_chunk->used) < sz)
            {
                size_t new_chunk_size = sz > chunk_size ? sz : chunk_size;
                chunks.push_back(back_chunk = new ArenaChunk<chunk_size>());
            }
            void *ptr = back_chunk->data + back_chunk->used;
            back_chunk->used += sz;
            return ptr;
        }

        void *arena_realloc(void *ptr, size_t old_sz, size_t new_sz)
        {
            if (ptr == nullptr)
                return arena_malloc(new_sz);
            if (new_sz <= old_sz)
                return ptr;
            void *new_ptr = arena_malloc(new_sz);
            memcpy(new_ptr, ptr, (std::min)(old_sz, new_sz));
            arena_free(ptr, old_sz);
            return new_ptr;
        }

        void arena_free(void *ptr, size_t sz)
        {
            memset(ptr, 0, sz);
            if (track_free_blocks)
                free_spots[sz].push_back(ptr);
            (void)ptr;
        }

        void reset()
        {
            for (auto chunk : chunks)
            {
                chunk->used = 0;
                memset(chunk->data, 0, chunk_size);
            };
        }
    };
}