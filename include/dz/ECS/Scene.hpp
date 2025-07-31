#pragma once
#include "Provider.hpp"

namespace dz::ecs {
    struct Scene : Provider<Scene> {
        int parent_index = -1;
        int parent_cid = 0;
        
        inline static constexpr size_t PID = 6;
        inline static float Priority = 0.5f;
        inline static constexpr bool IsSceneProvider = true;
        inline static std::string ProviderName = "Scene";
        inline static std::string StructName = "Scene";
        inline static std::string GLSLStruct = R"(
struct Scene {
    int parent_index;
    int parent_cid;
};
)";

        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Compute, R"(
void GetSceneModel(int scene_index, out mat4 out_model, out int parent_index, out int parent_cid) {
    Scene scene = GetSceneData(scene_index);

    out_model = mat4(1.0);

    parent_index = scene.parent_index;
    parent_cid = scene.parent_cid;
}
)"}
        };

        struct SceneReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            SceneReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Scene")
            {}
            SceneReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Scene;
            }
            std::string& GetName() override {
                return name;
            }
            std::vector<std::shared_ptr<ReflectableGroup>>& GetChildren() override {
                return reflectable_children;
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                if (!BackupGroupVector(serial, reflectable_children))
                    return false;
                return true;
            }
            bool restore(Serial& serial) override{
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                if (!RestoreGroupVector(serial, reflectable_children, buffer_group))
                    return false;
                return true;
            }
        };

        using ReflectableGroup = SceneReflectableGroup;
    };
}