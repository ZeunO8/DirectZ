#include <dz/FileHandle.hpp>

namespace dz {
    std::shared_ptr<std::iostream> FileHandle::open(std::ios_base::openmode ios)
    {
        auto final_path = path;
        switch (location)
        {
            case ASSET:
            {
            #if defined(ANDROID)
                if (cached_mem_stream)
                {
                    std::dynamic_pointer_cast<memory_stream>(cached_mem_stream)->mod(ios);
                    return cached_mem_stream;
                }
                cached_mem_stream = std::make_shared<memory_stream>(ios);
                auto android_asset_manager = dr_ptr->android_asset_manager;
                AAsset* asset = AAssetManager_open(android_asset_manager, final_path.c_str(), AASSET_MODE_BUFFER);
                if (!asset)
                    throw std::runtime_error("Failed to open asset: " + final_path);
                off_t size = AAsset_getLength(asset);
                auto data = (char*)malloc(size);
                memset(data, 0, size);
                AAsset_read(asset, data, size);
                AAsset_close(asset);
                cached_mem_stream->write(data, size);
                free(data);
                return cached_mem_stream;
            #else
                final_path = (getProgramDirectoryPath() / "assets" / path).string();
            #endif
            }
            case PATH:
                return std::make_shared<std::fstream>(final_path.c_str(), ios);
            case MEMORY:
            {
                if (cached_mem_stream)
                {
                    std::dynamic_pointer_cast<memory_stream>(cached_mem_stream)->mod(ios);
                    return cached_mem_stream;
                }
                return (cached_mem_stream = std::make_shared<memory_stream>(ios));
            }
        }
        throw std::runtime_error("Invalid file location");
    }
}