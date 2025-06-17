#pragma once

namespace dz
{
    struct FileHandle
    {
        enum LOCATION
        {
            PATH,
            ASSET,
            MEMORY
        };
        LOCATION location;
        std::string path;
        std::shared_ptr<std::iostream> cached_mem_stream;
        std::shared_ptr<std::iostream> open(std::ios_base::openmode ios);
    };
}