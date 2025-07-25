#pragma once
#include "Provider.hpp"
#include "Shape.hpp"
#include "../math.hpp"
#include <set>

#define ECS_MAX_COMPONENTS 8
namespace dz::ecs {
    struct Entity : Provider<Entity> {
        int id = 0;
        int shape_index;
        int parent_index = -1;
        int enabled_components = 0;
        vec<float, 4> position = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);
        vec<float, 4> rotation = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);;
        vec<float, 4> scale = vec<float, 4>(1.0f, 1.0f, 1.0f, 1.0f);;
        mat<float, 4, 4> model;
        
        inline static constexpr size_t PID = 1;
        inline static float Priority = 0.5f;
        inline static constexpr bool IsDrawProvider = true;
        inline static std::string ProviderName = "Entity";
        inline static std::string StructName = "Entity";
        inline static std::string GLSLStruct = R"(
struct Entity {
    int id;
    int shape_index;
    int parent_index;
    int enabled_components;
    vec4 position;
    vec4 rotation;
    vec4 scale;
    mat4 model;
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
vec4 GetEntityVertexColor(in Entity entity) {
    return vec4(0, 0, 1, 0.8);
}
)" }
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    vec4 vertex_position = vec4(shape_vertex, 1.0);
    final_color = GetEntityVertexColor(entity);
)", ShaderModuleType::Vertex}
        };

        uint32_t GetVertexCount(BufferGroup* buffer_group, Entity& entity) {
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, "Shapes");
            auto& shape = *(Shape*)(buffer_ptr.get() + (sizeof(Shape) * entity.shape_index));
            if (shape.vertex_count == -1 /* && mesh */)
            {
                // Lookup mesh?
            }
            return shape.vertex_count;
        }
        
        struct EntityTransformReflectable : Reflectable {

        private:
            std::function<Entity*()> get_entity_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"position", {0, 0}},
                {"rotation", {1, 0}},
                {"scale", {2, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "position"},
                {1, "rotation"},
                {2, "scale"}
            };
            inline static std::vector<std::string> prop_names = {
                "position",
                "rotation",
                "scale"
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
            int parent_id = -1;
            BufferGroup* buffer_group = 0;
            std::string name;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            std::vector<std::shared_ptr<ReflectableGroup>> component_groups;
            std::set<int> children;
            std::vector<Reflectable*> reflectables;
            EntityReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Entity")    
            {}
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
            bool serialize(Serial& ioSerial) const {
                ioSerial << parent_id << name;
                ioSerial << disabled << id << index << is_child;
                // ioSerial << components.size();
                // for (auto& [component_id, component_ptr] : components) {
                //     ioSerial << component_id;
                //     auto& root_data = ecs.GetComponentRootData(component_ptr->index);
                //     ioSerial << root_data.type;
                //     if (!component_ptr->serialize(ioSerial))
                //         return false;
                // }
                ioSerial << children.size();
                for (auto& child_id : children)
                    ioSerial << child_id;
                return true;
            }
            bool deserialize(Serial& ioSerial) {
                ioSerial >> parent_id >> name;
                ioSerial >> disabled >> id >> index >> is_child;
                // auto components_size = components.size();
                // ioSerial >> components_size;
                // for (size_t component_count = 1; component_count <= components_size; ++component_count) {
                //     int component_id;
                //     ioSerial >> component_id;
                //     int component_type;
                //     ioSerial >> component_type;
                //     auto& component_reg = ecs.registered_component_map[component_type];
                //     auto& component_ptr = (components[component_id] = component_reg.construct_component_fn());
                //     if (!component_ptr->deserialize(ioSerial))
                //         return false;
                // }
                auto children_size = children.size();
                ioSerial >> children_size;
                for (size_t child_count = 1; child_count <= children_size; ++child_count) {
                    int child_id;
                    ioSerial >> child_id;
                    children.insert(child_id);
                }
                return true;
            }
        };

        using ReflectableGroup = EntityReflectableGroup;

        inline static std::shared_ptr<ReflectableGroup> MakeGroup(BufferGroup* buffer_group) {
            return std::make_shared<EntityReflectableGroup>(buffer_group);
        }
    };
}