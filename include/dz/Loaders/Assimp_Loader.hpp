#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>

namespace dz::loaders {
    using MeshPair = std::pair<size_t, int>;
    using AddMeshFunction = std::function<MeshPair(
        size_t,
        const std::vector<vec<float, 4>>&,
        const std::vector<vec<float, 2>>&,
        const std::vector<vec<float, 4>>&
    )>;
    struct Assimp_Info {
        AddMeshFunction add_mesh_function;
        std::filesystem::path path;
        std::shared_ptr<char> bytes;
        size_t bytes_length = 0;
    };
    struct Assimp_Loader {
        using value_type = MeshPair;
        using info_type = Assimp_Info;
        static value_type Load(const info_type& info);
    };
}