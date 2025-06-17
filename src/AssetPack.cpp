struct AssetPack
{
    KeyValueStream<std::string, Asset> asset_stream;
};
AssetPack* create_asset_pack(FileHandle& file_handle)
{
    return new AssetPack{
        .asset_stream = {file_handle}
    };
}
void free_asset_pack(AssetPack* asset_pack)
{
    delete asset_pack;
}
Asset get_asset(AssetPack* asset_pack, const std::string& path)
{
    Asset asset;
    if (asset_pack->asset_stream.read(path, asset))
        return asset;
    throw "";
}
void add_asset(AssetPack* asset_pack, const std::string& path, const Asset& asset)
{

}
void add_asset(AssetPack* asset_pack, FileHandle& file_handle)
{
    auto stream_ptr = file_handle.open(std::ios::in | std::ios::binary);
    auto& stream = *stream_ptr;
    stream.seekg(0, std::ios::end);
    auto size = stream.tellg();
    stream.seekg(0);
    Asset asset((int8_t*)malloc(size), size, &default_free_deleter::call);
    stream.read((char*)asset.ptr, size);
    asset_pack->asset_stream.write(file_handle.path, asset);
}