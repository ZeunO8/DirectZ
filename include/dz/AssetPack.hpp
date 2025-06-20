/**
 * @file AssetPack.hpp
 * @brief Functions for managing asset packs for efficient binary data storage.
 */
#pragma once
#include "FileHandle.hpp"

namespace dz
{
    struct AssetPack;

    /**
     * @brief Asset alias representing a pointer to int8_t data.
     */
    using Asset = dz::size_ptr<char>;

    /**
     * @brief Creates an AssetPack using the provided file handle.
     * 
     * @param file_handle Reference to the file handle.
     * @return Pointer to the created AssetPack.
     */
    AssetPack* create_asset_pack(FileHandle& file_handle);

    /**
     * @brief Frees memory associated with the provided AssetPack.
     * 
     * @param asset_pack Pointer to the AssetPack to free.
     */
    void free_asset_pack(AssetPack* asset_pack);

    /**
     * @brief Retrieves an asset from the pack.
     * 
     * @param asset_pack Pointer to the AssetPack.
     * @param path Path identifying the asset.
     * @return Asset data as size_ptr<int8_t>.
     */
    bool get_asset(AssetPack* asset_pack, const std::string& path, Asset& out);

    /**
     * @brief Adds a new asset from a size_ptr buffer to the pack.
     * 
     * @param asset_pack Pointer to the AssetPack.
     * @param path Path to identify the asset.
     * @param asset Asset buffer to be added.
     */
    void add_asset(AssetPack* asset_pack, const std::string& path, const Asset& asset);

    /**
     * @brief Adds a new asset from a file to the pack.
     * 
     * @param asset_pack Pointer to the AssetPack.
     * @param file_handle File handle to the asset source.
     */
    void add_asset(AssetPack* asset_pack, FileHandle& file_handle);
}