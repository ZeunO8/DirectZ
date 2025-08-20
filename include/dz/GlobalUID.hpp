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

    public:
        /**
         * @brief Returns a new unique identifier.
         * 
         * @return A size_t representing a new unique ID.
         */
        static size_t GetNew();

        /**
         * @brief Returns a new unique identifier incrementing the given Keys count
         * 
         * @return A size_t representing a new unique ID.
         */
        static size_t GetNew(const std::string& key);

        inline static int SID = 1;
        static bool RestoreFunction(Serial&);
        static bool BackupFunction(Serial&);

    };
} // namespace dz