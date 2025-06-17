/**
 * @file FileHandle.hpp
 * @brief Provides a file abstraction layer for assets, memory, or disk-based I/O.
 */
#pragma once
#include <string>
#include <memory>
#include <iostream>

namespace dz
{
    /**
     * @brief Represents a generalized file handle that can reference disk files, embedded assets, or memory streams.
     */
    struct FileHandle
    {
        /**
         * @brief Specifies the origin of the file handle.
         */
        enum LOCATION
        {
            PATH,   /**< Represents a file path on disk. */
            ASSET,  /**< Represents an embedded asset in a package. */
            MEMORY  /**< Represents a stream from memory. */
        };

        LOCATION location;                           /**< The source type of the file. */
        std::string path;                            /**< File path or logical name. */
        std::shared_ptr<std::iostream> cached_mem_stream; /**< Cached stream for MEMORY mode. */

        /**
         * @brief Opens the file stream using the specified I/O mode.
         * 
         * @param ios I/O mode flags (e.g., std::ios::in | std::ios::binary).
         * @return A shared pointer to a valid std::iostream instance.
         */
        std::shared_ptr<std::iostream> open(std::ios_base::openmode ios);
    };
}