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
    struct ArenaChunk
    {
        size_t size;
        size_t used;
        uint8_t *data;

        ArenaChunk(size_t sz)
        {
            size = sz;
            used = 0;
            data = (uint8_t *)malloc(sz);
            memset(data, 0, size);
            assert(data != nullptr);
        }

        ~ArenaChunk()
        {
            free(data);
        }

        size_t remaining() const
        {
            return size - used;
        }
    };

    struct Arena
    {
        std::vector<ArenaChunk *> chunks;
        std::unordered_map<size_t, std::vector<void*>> free_spots;
        size_t chunk_size;

        Arena(size_t initial_chunk_size = 4096)
        {
            chunk_size = initial_chunk_size;
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
            auto free_it = free_spots.find(sz);
            if (free_it != free_spots.end())
            {
                auto& free_vec = free_it->second;
                auto free_ptr = free_vec.front();
                free_vec.erase(free_vec.begin());
                if (free_vec.empty())
                {
                    free_spots.erase(free_it);
                }
                return free_ptr;
            }
            if (chunks.empty() || chunks.back()->remaining() < sz)
            {
                size_t new_chunk_size = sz > chunk_size ? sz : chunk_size;
                chunks.push_back(new ArenaChunk(new_chunk_size));
            }
            ArenaChunk *c = chunks.back();
            void *ptr = c->data + c->used;
            c->used += sz;
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
            free_spots[sz].push_back(ptr);
            (void)ptr;
        }

        void reset()
        {
            for (auto chunk : chunks)
            {
                chunk->used = 0;
                memset(chunk->data, 0, chunk->size);
            };
        }
    };
}