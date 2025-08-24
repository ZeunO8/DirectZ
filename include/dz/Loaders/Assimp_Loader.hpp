#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <dz/function.hpp>
#include <vector>
#include <dz/math.hpp>
#include <dz/Image.hpp>
#include <dz/ImagePack.hpp>

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
    using TTangent = vec<float, 4>;
    using TBitangent = vec<float, 4>;
    using TColor = vec<float, 4>;
    using TMetalness = float;
    using TRoughness = float;
    using AddSceneFunction = dz::function<SceneID(
        ParentID,
        const std::string&,
        TPosition,
        TRotation,
        TScale
    )>;
    using AddEntityFunction = dz::function<EntityID(
        ParentID,
        const std::string&,
        const std::vector<int>&,
        TPosition,
        TRotation,
        TScale
    )>;
    using AddMaterialFunction = dz::function<MaterialPair(
        const std::string&,
        const std::vector<Image*>&,
        TColor,
        TMetalness,
        TRoughness
    )>; 
    using AddMeshFunction = dz::function<MeshPair(
        const std::string&,
        MaterialIndex,
        const std::vector<TPosition>&,
        const std::vector<TUV2>&,
        const std::vector<TNormal>&,
        const std::vector<TTangent>&,
        const std::vector<TBitangent>&
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
        TPosition root_position = TPosition(0.0, 0.0, 0.0, 1.0);
        TRotation root_rotation = TRotation(0.0, 0.0, 0.0, 1.0);
        TScale root_scale = TScale(1.0, 1.0, 1.0, 1.0);
    };
    struct Assimp_Loader {
        using value_type = SceneID;
        using info_type = Assimp_Info;
        static value_type Load(const info_type& info);
    };
}