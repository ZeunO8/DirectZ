#pragma once
#include "Provider.hpp"
#include "../math.hpp"
#include <set>

namespace dz::ecs {
    struct Entity : Provider<Entity> {
        int parent_index = -1;
        int parent_cid = 0;
        int enabled_components = 0;
        int transform_dirty = 1;
        vec<float, 4> position = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);
        vec<float, 4> rotation = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);;
        vec<float, 4> scale = vec<float, 4>(1.0f, 1.0f, 1.0f, 1.0f);;
        mat<float, 4, 4> model = mat<float, 4, 4>(1.0f);
        
        inline static constexpr size_t PID = 2;
        inline static float Priority = 0.5f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsEntityProvider = true;
        inline static std::string ProviderName = "Entity";
        inline static std::string StructName = "Entity";
        inline static std::string GLSLStruct = R"(
struct Entity {
    int parent_index;
    int parent_cid;
    int enabled_components;
    int transform_dirty;
    vec4 position;
    vec4 rotation;
    vec4 scale;
    mat4 model;
};
)";

        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
)" },
            { ShaderModuleType::Compute, R"(
void GetEntityModel(int entity_index, out mat4 out_model, out int parent_index, out int parent_cid) {
    Entity entity = GetEntityData(entity_index);

    if (entity.transform_dirty == 0) {
        out_model = entity.model;
        parent_index = entity.parent_index;
        parent_cid = entity.parent_cid;
        return;
    }

    mat4 model = mat4(1.0);
    model[3] = vec4(entity.position.xyz, 1.0);

    mat4 scale = mat4(1.0);
    scale[0].xyz *= entity.scale.x;
    scale[1].xyz *= entity.scale.y;
    scale[2].xyz *= entity.scale.z;

    mat4 rotX = mat4(1.0);
    rotX[1][1] =  cos(entity.rotation.x);
    rotX[1][2] = -sin(entity.rotation.x);
    rotX[2][1] =  sin(entity.rotation.x);
    rotX[2][2] =  cos(entity.rotation.x);

    mat4 rotY = mat4(1.0);
    rotY[0][0] =  cos(entity.rotation.y);
    rotY[0][2] =  sin(entity.rotation.y);
    rotY[2][0] = -sin(entity.rotation.y);
    rotY[2][2] =  cos(entity.rotation.y);

    mat4 rotZ = mat4(1.0);
    rotZ[0][0] =  cos(entity.rotation.z);
    rotZ[0][1] = -sin(entity.rotation.z);
    rotZ[1][0] =  sin(entity.rotation.z);
    rotZ[1][1] =  cos(entity.rotation.z);

    mat4 rot = rotZ * rotY * rotX;

    out_model = model * rot * scale;

    parent_index = entity.parent_index;
    parent_cid = entity.parent_cid;

    entity.transform_dirty = 0;
}
)"}
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    vec4 vertex_position = vec4(mesh_vertex, 1.0);
)", ShaderModuleType::Vertex}
        };
        
        struct EntityTransformReflectable : Reflectable {

        private:
            std::function<Entity*()> get_entity_function;
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
            EntityTransformReflectable(const std::function<Entity*()>& get_entity_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct EntityReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            std::vector<std::shared_ptr<ReflectableGroup>> component_groups;
            std::vector<Reflectable*> reflectables;
            EntityReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Entity")
            {}
            EntityReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            ~EntityReflectableGroup() {
                ClearReflectables();
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Entity;
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
            void UpdateReflectables() { }
            void ClearReflectables() {
                if (reflectables.empty()) {
                    return;
                }
                delete reflectables[0];
                reflectables.clear();
            }
            void UpdateChildren() override {
                if (reflectables.empty()) {
                    reflectables.push_back(new EntityTransformReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "Entitys");
                        return ((struct Entity*)(buffer.get())) + index;
                    }));
                }
                else {
                    for (size_t i = 1; i < reflectables.size();)
                        reflectables.erase(reflectables.begin() + i);
                }
                for (auto& component_group_ptr : component_groups) {
                    auto& component_group = *component_group_ptr;
                    for (auto& component_reflectable_ptr : component_group.GetReflectables()) 
                        reflectables.push_back(component_reflectable_ptr);
                }
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                if (!BackupGroupVector(serial, reflectable_children))
                    return false;
                if (!BackupGroupVector(serial, component_groups))
                    return false;
                return true;
            }
            bool restore(Serial& serial) override {
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                if (!RestoreGroupVector(serial, reflectable_children, buffer_group))
                    return false;
                if (!RestoreGroupVector(serial, component_groups, buffer_group))
                    return false;
                return true;
            }
        };

        using ReflectableGroup = EntityReflectableGroup;
    };
}