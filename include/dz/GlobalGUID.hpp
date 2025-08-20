/**
 * @file GlobalGUID.hpp
 * @brief Provides a static method for getting a GUID
 */
#pragma once
#include <mutex>
#include <unordered_map>
#include <string>
#include "_GUID_.hpp"

namespace dz
{
    /**
     * @brief Thread-safe UID generator that increments globally across the application.
     */
    struct GlobalGUID : StaticRestorable
    {
    private:

    public:
        /**
         * @brief Returns a new GUID.
         * 
         * @return A std::string representing a new GUID.
         */
        static _GUID_ GetNew();

    };
} // namespace dz