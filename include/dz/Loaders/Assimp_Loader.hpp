#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <functional>
#include <vector>
#include <dz/math.hpp>
#include <dz/Image.hpp>

namespace dz::loaders {
    using MeshPair = std::pair<size_t, int>;
    using MaterialPair = std::pair<size_t, int>;
    using SceneID = size_t;
    using EntityID = size_t;
    using ParentID = int;
    using MaterialIndex = int;
    using TPosition = vec<float, 4>;
    using TRotation = vec<float, 4>;
    using TScale = vec<float, 4>;
    using TUV2 = vec<float, 2>;
    using TNormal = vec<float, 4>;
    using AddSceneFunction = std::function<SceneID(
        ParentID,
        const std::string&
    )>;
    using AddEntityFunction = std::function<EntityID(
        ParentID,
        const std::string&,
        const std::vector<int>&,
        TPosition,
        TRotation,
        TScale
    )>;
    using AddMaterialFunction = std::function<MaterialPair(
        const std::string&,
        Image*
    )>; 
    using AddMeshFunction = std::function<MeshPair(
        const std::string&,
        MaterialIndex,
        const std::vector<TPosition>&,
        const std::vector<TUV2>&,
        const std::vector<TNormal>&
    )>;
    struct Assimp_Info {
        ParentID parent_id = -1;
        AddSceneFunction add_scene_function;
        AddEntityFunction add_entity_function;
        AddMeshFunction add_mesh_function;
        AddMaterialFunction add_material_function;
        std::filesystem::path path;
        std::shared_ptr<char> bytes;
        size_t bytes_length = 0;
    };
    struct Assimp_Loader {
        using value_type = SceneID;
        using info_type = Assimp_Info;
        static value_type Load(const info_type& info);
    };
}