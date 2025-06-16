#pragma once
namespace dz
{
    struct AssetPack;
    AssetPack* load_asset_pack();
    Asset* get_asset(AssetPack* asset_pack, const std::string& path);
}