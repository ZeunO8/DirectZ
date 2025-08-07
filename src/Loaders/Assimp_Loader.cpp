#include <dz/Loaders/Assimp_Loader.hpp>
#include "../Assimp/Assimp.hpp"
#include <dz/Loaders/STB_Image_Loader.hpp>
#include "../Image.cpp.hpp"
#include <iostream>
#include <unordered_map>

namespace dz::loaders::assimp_loader {
    Assimp::Importer importer;
    struct AssimpContext {
        const aiScene* scene_ptr = 0;
        size_t totalNodes = 0;
        std::unordered_map<uint32_t, MeshPair> mesh_index_pair_map;
    };

    #define ASSIMP_FLAGS aiProcess_Triangulate | aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_ConvertToLeftHanded

    void InitContext(AssimpContext& context, const Assimp_Info& info) {
        if (!info.path.empty()) {
            auto info_path_string = info.path.string();
            const aiScene *scene_ptr = importer.ReadFile(info_path_string.c_str(), ASSIMP_FLAGS);
            if (!scene_ptr || scene_ptr->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_ptr->mRootNode)
            {
                std::cerr << importer.GetErrorString() << std::endl;
                return;
            }
            context.scene_ptr = scene_ptr;
        }
        else if (info.bytes && info.bytes_length) {
            const aiScene *scene_ptr = importer.ReadFileFromMemory(info.bytes.get(), info.bytes_length, ASSIMP_FLAGS, nullptr);
            if (!scene_ptr || scene_ptr->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_ptr->mRootNode)
            {
                std::cerr << importer.GetErrorString() << std::endl;
                return;
            }
            context.scene_ptr = scene_ptr;
        }
        else
            throw std::runtime_error("Neither bytes nor path were provided to info!");
    }

    void CountNodes(AssimpContext& context, aiNode* node) {
        context.totalNodes++;
        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            CountNodes(context, node->mChildren[i]);
        }
    }

    bool IsCombinedImage(
        const aiScene* aiscene,
        aiMaterial* material,
        aiTextureType type_x,
        aiTextureType type_y
    ) {
        static auto HasType = [](auto material, auto type, auto& imageIndex) -> bool {
            auto count = material->GetTextureCount(type);
            for (uint32_t i = 0; i < count; ++i) {
                aiString str;
                material->GetTexture(type, i, &str);
                if (str.data[0] == '*') {
                    imageIndex = atoi(&str.data[1]);
                    return true;
                }
            }
            return false;
        };
        int32_t x_index = -1, y_index = -1;
        auto has_x = HasType(material, type_x, x_index);
        auto has_y = HasType(material, type_y, y_index);
        return (has_x && has_y && x_index == y_index);
    }

    std::vector<Image*> LoadMaterialImages(
        const aiScene *aiscene,
        aiMaterial *material,
        const aiTextureType &type,
        SurfaceType surfaceType
    )
    {
        std::vector<Image*> images;
        for (uint32_t i = 0; i < material->GetTextureCount(type); i++)
        {
            aiString str;
            material->GetTexture(type, i, &str);
            Image* image_ptr = nullptr;
            int32_t imageWidth = 0, imageHeight = 0;
            // Check if the image is embedded
            if (str.data[0] == '*')
            {
                uint32_t imageIndex = atoi(&str.data[1]);
                aiTexture *aiTex = aiscene->mTextures[imageIndex];
                if (aiTex->mHeight == 0)
                {
                    // The embedded image is compressed (e.g., PNG or JPG in memory)
                    image_ptr = STB_Image_Loader::Load({
                        .bytes = std::shared_ptr<char>((char*)aiTex->pcData, [](auto p) {}),
                        .bytes_length = aiTex->mWidth
                    });
                    image_ptr->surfaceType = surfaceType;
                }
                else
                {
                    // The embedded image is uncompressed raw data
                    throw std::runtime_error("A embedded image is uncompressed raw data, we currently only support compressed images such as PNG or JPG");
                }
            }
            else
            {
                // External image
                throw std::runtime_error("A image is an external image, we currently only support compressed embedded images such as PNG or JPG");
            }
            images.push_back(image_ptr);
        }
        return images;
    }

    void AddNode(AssimpContext& context, const Assimp_Info& info, aiNode* node, size_t parent_id) {
	    mat<float, 4, 4> transformation = AssimpConvert<aiMatrix4x4, mat<float, 4, 4>>(node->mTransformation);
        vec<float, 4> position;
        quat<float> rotation_quat;
        vec<float, 4> scale;
        transformation.decompose(position, rotation_quat, scale);
        auto rotation = quat_to_euler_xyz(rotation_quat);
        std::vector<int> mesh_indexes;
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            uint32_t aiMeshIndex = node->mMeshes[i];
            auto pair_it = context.mesh_index_pair_map.find(aiMeshIndex);
            if (pair_it != context.mesh_index_pair_map.end()) {
                mesh_indexes.push_back(pair_it->second.second);
            }
            auto& ai_mesh = context.scene_ptr->mMeshes[aiMeshIndex];
            vec<float, 4> albedo_color;
            // image data
            std::vector<Image*> images_vec;
            MaterialPair material_pair(0, -1);
            if (ai_mesh->mMaterialIndex >= 0)
            {
                aiMaterial *material = context.scene_ptr->mMaterials[ai_mesh->mMaterialIndex];
                std::string material_name(material->GetName().C_Str());
                // Load base color (albedo) image
                {
                std::vector<Image*> baseColorMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_BASE_COLOR, SurfaceType::BaseColor);
                auto count = 0;
                for (auto& image_ptr : baseColorMaps) {
                    images_vec.push_back(image_ptr);
                }
                }
                // Load diffuse images
                {
                std::vector<Image*> diffuseMaps;
                if (images_vec.empty())
                {
                    diffuseMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_DIFFUSE, SurfaceType::Diffuse);
                    auto count = 0;
                    for (auto& image_ptr : diffuseMaps) {
                        images_vec.push_back(image_ptr);
                    }
                }
                }
                // Load specular textures
                {
                auto specularMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_SPECULAR, SurfaceType::Specular);
                auto count = 0;
                for (auto& image_ptr : specularMaps) {
                    images_vec.push_back(image_ptr);
                }
                }
                // Load normal maps
                {
                auto normalMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_NORMALS, SurfaceType::Normal);
                auto count = 0;
                for (auto& image_ptr : normalMaps) {
                    images_vec.push_back(image_ptr);
                }
                }
                // Load height maps
                {
                auto heightMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_HEIGHT, SurfaceType::Height);
                auto count = 0;
                for (auto& image_ptr : heightMaps) {
                    images_vec.push_back(image_ptr);
                }
                }
                // Load ambient occlusion maps
                {
                auto aoMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_AMBIENT_OCCLUSION, SurfaceType::AmbientOcclusion);
                auto count = 0;
                for (auto& image_ptr : aoMaps) {
                    images_vec.push_back(image_ptr);
                }
                }
                bool is_combined_metal_rough = IsCombinedImage(context.scene_ptr, material, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_METALNESS);
                if (is_combined_metal_rough) {
                    // Load MetalnessRoughness maps
                    {
                    auto metalnessRoughnessMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_DIFFUSE_ROUGHNESS, SurfaceType::MetalnessRoughness);
                    auto count = 0;
                    for (auto& image_ptr : metalnessRoughnessMaps) {
                        images_vec.push_back(image_ptr);
                    }
                    }
                }
                else {
                    // Load Roughness maps
                    {
                    auto roughnessMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_DIFFUSE_ROUGHNESS, SurfaceType::DiffuseRoughness);
                    auto count = 0;
                    for (auto& image_ptr : roughnessMaps) {
                        images_vec.push_back(image_ptr);
                    }
                    }
                    // Load Metal maps
                    {
                    auto metalMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_METALNESS, SurfaceType::Metalness);
                    auto count = 0;
                    for (auto& image_ptr : metalMaps) {
                        images_vec.push_back(image_ptr);
                    }
                    }
                }
                // Load Shininess maps
                {
                auto shininessMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_SHININESS, SurfaceType::Shininess);
                auto count = 0;
                for (auto& image_ptr : shininessMaps) {
                    images_vec.push_back(image_ptr);
                }
                }
                float metalness = 0, roughness = 0;
                // Load Albedo Color
                {
                aiColor4D aicolor;
                if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &aicolor))
                {
                    albedo_color = AssimpConvert<aiColor4D, vec<float, 4>>(aicolor);
                }
                if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metalness)) { }
                if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughness)) { }
                }
                material_pair = info.add_material_function(material_name, images_vec, albedo_color, metalness, roughness);
            }
            // vertex data
            std::vector<TPosition> positions;
            positions.reserve(ai_mesh->mNumFaces * 3);

            std::vector<TUV2> uv2s;
            uv2s.reserve(ai_mesh->mNumFaces * 3);

            std::vector<TNormal> normals;
            normals.reserve(ai_mesh->mNumFaces * 3);

            std::vector<TTangent> tangents;
            tangents.reserve(ai_mesh->mNumFaces * 3);

            std::vector<TBitangent> bitangents;
            bitangents.reserve(ai_mesh->mNumFaces * 3);

            static constexpr auto i0 = 0;
            static constexpr auto i1 = 1;
            static constexpr auto i2 = 2;
            // ai_mesh->mTangents
            for (auto fi = 0; fi < ai_mesh->mNumFaces; ++fi) {
                auto& face = ai_mesh->mFaces[fi];
                if (face.mNumIndices == 3) {
                    auto i_0 = face.mIndices[i0];
                    auto i_1 = face.mIndices[i1];
                    auto i_2 = face.mIndices[i2];
                    positions.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mVertices[i_0]));
                    positions.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mVertices[i_1]));
                    positions.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mVertices[i_2]));
                    uv2s.push_back(AssimpConvert<aiVector3D, vec<float, 2>>(ai_mesh->mTextureCoords[0][i_0]));
                    uv2s.push_back(AssimpConvert<aiVector3D, vec<float, 2>>(ai_mesh->mTextureCoords[0][i_1]));
                    uv2s.push_back(AssimpConvert<aiVector3D, vec<float, 2>>(ai_mesh->mTextureCoords[0][i_2]));
                    normals.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mNormals[i_0]));
                    normals.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mNormals[i_1]));
                    normals.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mNormals[i_2]));
                    tangents.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mTangents[i_0]));
                    tangents.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mTangents[i_1]));
                    tangents.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mTangents[i_2]));
                    bitangents.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mBitangents[i_0]));
                    bitangents.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mBitangents[i_1]));
                    bitangents.push_back(AssimpConvert<aiVector3D, vec<float, 4>>(ai_mesh->mBitangents[i_2]));
                }
                else
			        throw std::runtime_error("expected number of indices in face to equal 3");
            }
            auto mesh_pair = info.add_mesh_function(ai_mesh->mName.C_Str(), material_pair.second, positions, uv2s, normals, tangents, bitangents);
            context.mesh_index_pair_map[aiMeshIndex] = mesh_pair;
            mesh_indexes.push_back(mesh_pair.second);
        }
        auto entity_id = info.add_entity_function(parent_id, node->mName.C_Str(), mesh_indexes, position, rotation, scale);
    }

    dz::loaders::SceneID AssimpLoad(AssimpContext& context, const Assimp_Info& info) {
        CountNodes(context, context.scene_ptr->mRootNode);
        auto scene_id = info.add_scene_function(info.parent_id, context.scene_ptr->mName.C_Str(), info.root_position, info.root_rotation, info.root_scale);
        AddNode(context, info, context.scene_ptr->mRootNode, scene_id);
        return scene_id;
    }
}
dz::loaders::SceneID dz::loaders::Assimp_Loader::Load(const info_type& info) {
    using namespace dz::loaders::assimp_loader;
    AssimpContext context;
    InitContext(context, info);
    return AssimpLoad(context, info);
}