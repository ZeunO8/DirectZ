#include <dz/Loaders/Assimp_Loader.hpp>
#include "../Assimp/Assimp.hpp"

namespace dz::loaders::assimp_loader {
    Assimp::Importer importer;
    struct AssimpContext {
        const aiScene* scene_ptr = 0;
        size_t totalNodes = 0;
    };
    void InitContext(AssimpContext& context, const Assimp_Info& info) {
        if (!info.path.empty()) {
            const aiScene *scene_ptr = importer.ReadFile(info.path.c_str(), aiProcess_Triangulate | aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!scene_ptr || scene_ptr->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_ptr->mRootNode)
            {
                std::cerr << importer.GetErrorString() << std::endl;
                return nullptr;
            }
            context.scene_ptr = scene_ptr;
        }
        else if (info.bytes && info.bytes_length) {
            const aiScene *scene_ptr = importer.ReadFileFromMemory(info.bytes.get(), info.bytes_length, aiProcess_Triangulate | aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs | aiProcess_CalcTangentSpace, nullptr);
            if (!scene_ptr || scene_ptr->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene_ptr->mRootNode)
            {
                std::cerr << importer.GetErrorString() << std::endl;
                return nullptr;
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
            totalNodes(context, node->mChildren[i]);
        }
    }

    dz::loaders::MeshPair ProcessNode(AssimpContext& context, aiNode* node) {
	    mat<float, 4, 4> transformation = AssimpConvert<aiMatrix4x4, mat<float, 4, 4>>(node->mTransformation);
    }

    dz::loaders::MeshPair AssimpLoad(AssimpContext& context, const Assimp_Info& info) {
        CountNodes(context, info.scene_ptr->mRootNode);
        return ProcessNode(context, info.scene_ptr->mRootNode);
    }
}
dz::loaders::MeshPair dz::loaders::Assimp_Loader::Load(const info_type& info) {
    using namespace dz::loaders::assimp_loader;
    AssimpContext context;
    InitContext(context, info);
    value_type value(0, -1);
    return AssimpLoad(context, info);
}