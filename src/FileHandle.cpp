std::shared_ptr<std::iostream> FileHandle::open(std::ios_base::openmode ios)
{
    auto final_path = path;
    switch (location)
    {
        case ASSET:
        #if defined(ANDROID)
            // get android asset
            break;
        #else
            final_path = /*assets_location + */ path;
        #endif
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
    throw "";
}