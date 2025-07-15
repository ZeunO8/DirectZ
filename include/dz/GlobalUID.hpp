/**
 * @file GlobalUID.hpp
 * @brief Provides a globally thread-safe monotonically incrementing UID generator.
 */
#pragma once
#include <mutex>
#include <unordered_map>
#include <string>

namespace dz
{
    /**
     * @brief Thread-safe UID generator that increments globally across the application.
     */
    struct GlobalUID
    {
    private:
        static inline std::unordered_map<std::string, size_t> KeyedCounts = {};
        static inline size_t Count = 0;     /**< Global counter shared across threads. */
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
    };
} // namespace dz