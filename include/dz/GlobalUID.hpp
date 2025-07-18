/**
 * @file GlobalUID.hpp
 * @brief Provides a globally thread-safe monotonically incrementing UID generator.
 */
#pragma once
#include <mutex>
#include <unordered_map>
#include <string>
#include "State.hpp"

namespace dz
{
    /**
     * @brief Thread-safe UID generator that increments globally across the application.
     */
    struct GlobalUID : StaticRestorable
    {
    private:
        static inline size_t Count = 0;     /**< Global counter shared across threads. */
        static inline std::unordered_map<std::string, size_t> KeyedCounts = {};
        static inline std::mutex Mutex = {};/**< Mutex used for thread-safety. */

    public:
        /**
         * @brief Returns a new unique identifier.
         * 
         * @return A size_t representing a new unique ID.
         */
        inline static size_t GetNew()
        {
            std::lock_guard lock(Mutex);
            return ++Count;
        }

        /**
         * @brief Returns a new unique identifier incrementing the given Keys count
         * 
         * @return A size_t representing a new unique ID.
         */
        inline static size_t GetNew(const std::string& key)
        {
            std::lock_guard lock(Mutex);
            return ++KeyedCounts[key];
        }

        inline static int SID = 1;
        inline static std::function<bool(Serial&)> RestoreFunction = [](auto& serial) {
            serial >> Count;
            auto keyed_counts_size = KeyedCounts.size();
            serial >> keyed_counts_size;
            for (size_t count = 1; count <= keyed_counts_size; ++count) {
                std::string key;
                size_t key_count = 0;
                serial >> key >> key_count;
                KeyedCounts[key] = key_count;
            }
            return true;
        };
        inline static std::function<bool(Serial&)> BackupFunction = [](auto& serial) {
            serial << Count;
            serial << KeyedCounts.size();
            for (auto& [key, key_count] : KeyedCounts)
                serial << key << key_count;
            return true;
        };

    };
} // namespace dz