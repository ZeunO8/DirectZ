#pragma once

#include "../Image.hpp"
#include <filesystem>
#include <memory>

namespace dz::loaders {
    struct STB_Image_Info {
        std::filesystem::path path;
        std::shared_ptr<char> bytes;
        size_t bytes_length = 0;
    };
    struct STB_Image_Loader {
        using ptr_type = Image;
        using info_type = STB_Image_Info;
        static ptr_type* Load(const info_type& info);
    };
}