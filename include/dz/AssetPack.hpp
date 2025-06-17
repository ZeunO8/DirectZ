#pragma once
#include "FileHandle.hpp"
namespace dz
{
    struct AssetPack;
    using Asset = dz::size_ptr<int8_t>;
    AssetPack* create_asset_pack(FileHandle& file_handle);
    void free_asset_pack(AssetPack* asset_pack);
    Asset get_asset(AssetPack* asset_pack, const std::string& path);
    void add_asset(AssetPack* asset_pack, const std::string& path, const Asset& asset);
    void add_asset(AssetPack* asset_pack, FileHandle& file_handle);
}