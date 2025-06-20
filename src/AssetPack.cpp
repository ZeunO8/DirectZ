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
bool get_asset(AssetPack* asset_pack, const std::string& path, Asset& out)
{
    return asset_pack->asset_stream.read(path, out);
}
void add_asset(AssetPack* asset_pack, const std::string& path, const Asset& asset)
{

}
void add_asset(AssetPack* asset_pack, FileHandle& file_handle)
{
    auto stream_ptr = file_handle.open(std::ios::in | std::ios::binary);
    auto& stream = *stream_ptr;
    stream.seekg(0, std::ios::end);
    auto size = (size_t)stream.tellg();
    stream.seekg(0);
    auto mem = (char*)malloc(size + 1);
    memset(mem, 0, size + 1);
    Asset asset(mem, size + 1, &default_free_deleter::call);
    stream.read(asset.ptr, size);
    asset_pack->asset_stream.write(file_handle.path, asset);
}