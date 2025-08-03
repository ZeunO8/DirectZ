#include <dz/Loaders/Assimp_Loader.hpp>
#include "../Assimp/Assimp.hpp"
#include <dz/Loaders/STB_Image_Loader.hpp>
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

    std::vector<Image*> LoadMaterialImages(
        const aiScene *aiscene,
        aiMaterial *material,
        const aiTextureType &type,
        const std::string &typeName
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
            // image data
            std::vector<std::tuple<std::string, std::string, Image*>> keyed_images;
            if (ai_mesh->mMaterialIndex >= 0)
            {
                aiMaterial *material = context.scene_ptr->mMaterials[ai_mesh->mMaterialIndex];
                std::string material_name(material->GetName().C_Str());
                // Load base color (albedo) image
                std::vector<Image*> baseColorMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_BASE_COLOR, "baseColor");
                for (auto& image_ptr : baseColorMaps)
                {
                    keyed_images.push_back({"ColorTexture", material_name, image_ptr});
                }
                // Load diffuse images
                std::vector<Image*> diffuseMaps;
                if (!baseColorMaps.size())
                {
                    diffuseMaps = LoadMaterialImages(context.scene_ptr, material, aiTextureType_DIFFUSE, "diffuse");
                    for (auto& image_ptr : diffuseMaps)
                    {
                        keyed_images.push_back({"ColorTexture", material_name, image_ptr});
                    }
                }
            }
            std::vector<int> material_indexes;
            material_indexes.reserve(keyed_images.size());
            for (auto& keyed_tuple : keyed_images) {
                auto material_pair = info.add_material_function(std::get<1>(keyed_tuple), std::get<2>(keyed_tuple));
                material_indexes.push_back(material_pair.second);
            }
            // vertex data
            std::vector<TPosition> positions;
            positions.reserve(ai_mesh->mNumFaces * 3);
            std::vector<TUV2> uv2s;
            uv2s.reserve(ai_mesh->mNumFaces * 3);
            std::vector<TNormal> normals;
            normals.reserve(ai_mesh->mNumFaces * 3);
            static constexpr auto i0 = 0;
            static constexpr auto i1 = 1;
            static constexpr auto i2 = 2;
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
                }
                else
			        throw std::runtime_error("expected number of indices in face to equal 3");
            }
            auto material_index = material_indexes.empty() ? -1 : material_indexes[0];
            auto mesh_pair = info.add_mesh_function(ai_mesh->mName.C_Str(), material_index, positions, uv2s, normals);
            context.mesh_index_pair_map[aiMeshIndex] = mesh_pair;
            mesh_indexes.push_back(mesh_pair.second);
        }
        auto entity_id = info.add_entity_function(parent_id, node->mName.C_Str(), mesh_indexes, position, rotation, scale);
    }

    dz::loaders::SceneID AssimpLoad(AssimpContext& context, const Assimp_Info& info) {
        CountNodes(context, context.scene_ptr->mRootNode);
        auto scene_id = info.add_scene_function(info.parent_id, context.scene_ptr->mName.C_Str());
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