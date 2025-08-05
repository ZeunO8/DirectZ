#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"

namespace dz::ecs {
    struct Scene : Provider<Scene> {
        int parent_index = -1;
        int parent_cid = 0;
        int transform_dirty = 1;
        int padding2 = 0;
        vec<float, 4> position = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);
        vec<float, 4> rotation = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);;
        vec<float, 4> scale = vec<float, 4>(1.0f, 1.0f, 1.0f, 1.0f);;
        mat<float, 4, 4> model = mat<float, 4, 4>(1.0f);
        
        inline static constexpr size_t PID = 1;
        inline static float Priority = 0.5f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsSceneProvider = true;
        inline static std::string ProviderName = "Scene";
        inline static std::string StructName = "Scene";
        inline static std::string GLSLStruct = R"(
struct Scene {
    int parent_index;
    int parent_cid;
    int transform_dirty;
    int padding2;
    vec4 position;
    vec4 rotation;
    vec4 scale;
    mat4 model;
};
)";

        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Compute, R"(
void GetSceneModel(int scene_index, out mat4 out_model, out int parent_index, out int parent_cid) {
    Scene scene = GetSceneData(scene_index);

    if (scene.transform_dirty == 0) {
        out_model = scene.model;
        parent_index = scene.parent_index;
        parent_cid = scene.parent_cid;
        return;
    }

    mat4 model = mat4(1.0);
    model[3] = vec4(scene.position.xyz, 1.0);

    mat4 scale = mat4(1.0);
    scale[0].xyz *= scene.scale.x;
    scale[1].xyz *= scene.scale.y;
    scale[2].xyz *= scene.scale.z;

    mat4 rotX = mat4(1.0);
    rotX[1][1] =  cos(scene.rotation.x);
    rotX[1][2] = -sin(scene.rotation.x);
    rotX[2][1] =  sin(scene.rotation.x);
    rotX[2][2] =  cos(scene.rotation.x);

    mat4 rotY = mat4(1.0);
    rotY[0][0] =  cos(scene.rotation.y);
    rotY[0][2] =  sin(scene.rotation.y);
    rotY[2][0] = -sin(scene.rotation.y);
    rotY[2][2] =  cos(scene.rotation.y);

    mat4 rotZ = mat4(1.0);
    rotZ[0][0] =  cos(scene.rotation.z);
    rotZ[0][1] = -sin(scene.rotation.z);
    rotZ[1][0] =  sin(scene.rotation.z);
    rotZ[1][1] =  cos(scene.rotation.z);

    mat4 rot = rotZ * rotY * rotX;

    out_model = model * rot * scale;

    parent_index = scene.parent_index;
    parent_cid = scene.parent_cid;

    scene.transform_dirty = 0;
}
)"}
        };

        struct SceneTransformReflectable : ::Reflectable {
        private:
            std::function<Scene*()> get_scene_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"Position", {0, 0}},
                {"Rotation", {1, 0}},
                {"Scale", {2, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "Position"},
                {1, "Rotation"},
                {2, "Scale"}
            };
            inline static std::vector<std::string> prop_names = {
                "Position",
                "Rotation",
                "Scale"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(vec<float, 3>),
                &typeid(vec<float, 3>),
                &typeid(vec<float, 3>)
            };

        public:
            SceneTransformReflectable(const std::function<Scene*()>& get_scene_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct SceneReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            std::vector<Reflectable*> reflectables;
            SceneReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Scene")
            {}
            SceneReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            ~SceneReflectableGroup() {
                ClearReflectables();
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Scene;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            std::vector<std::shared_ptr<ReflectableGroup>>& GetChildren() override {
                return reflectable_children;
            }
            void ClearReflectables() {
                if (reflectables.empty()) {
                    return;
                }
                delete reflectables[0];
                reflectables.clear();
            }
            void UpdateChildren() override {
                if (reflectables.empty()) {
                    reflectables.push_back(new SceneTransformReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "Scenes");
                        return ((struct Scene*)(buffer.get())) + index;
                    }));
                }
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