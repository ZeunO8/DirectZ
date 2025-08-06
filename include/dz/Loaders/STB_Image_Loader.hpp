#pragma once

#include "../Image.hpp"
#include <filesystem>
#include <memory>

namespace dz::loaders {
    struct STB_Image_Info {
        std::filesystem::path path;
        std::shared_ptr<char> bytes;
        size_t bytes_length = 0;
        bool load_float = 0; // false loads UNORM, true loads SFLOAT
    };
    struct STB_Image_Loader {
        using value_type = Image*;
        using info_type = STB_Image_Info;
        static value_type Load(const info_type& info);
    };
}