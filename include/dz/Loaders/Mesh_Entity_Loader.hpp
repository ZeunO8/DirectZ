#pragma once

namespace dz::loaders {
    template <typename TECS>
    struct Mesh_Entity_Info {
        TECS& ecs;
        std::filesystem::path path;
        std::shared_ptr<char> bytes;
    };
    template <typename TECS>
    struct Mesh_Entity_Loader {
        using value_type = std::pair<size_t, int>;
        using info_type = Mesh_Entity_Info<TECS>;
        static value_type Load(const info_type& info) {
            // if (!info.path.empty())
            //     return STB_Image_load_path(info.path);
            // if (info.bytes && info.bytes_length)
            //     return STB_Image_load_bytes(info.bytes, info.bytes_length);
            throw std::runtime_error("Neither bytes nor path were provided to info!");
        }
    };
}