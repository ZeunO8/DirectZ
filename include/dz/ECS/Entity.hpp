#pragma once
#include "Provider.hpp"
#include "Shape.hpp"
#include "../math.hpp"

#define ECS_MAX_COMPONENTS 8
namespace dz::ecs {
    struct Entity : Provider<Entity> {
        int id = 0;
        int shape_index;
        int componentsCount = 0;
        int parent_index = -1;
        int components[ECS_MAX_COMPONENTS] = {0};
        vec<float, 4> position = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);
        vec<float, 4> rotation = vec<float, 4>(0.0f, 0.0f, 0.0f, 1.0f);;
        vec<float, 4> scale = vec<float, 4>(1.0f, 1.0f, 1.0f, 1.0f);;
        mat<float, 4, 4> model;
        
        inline static float Priority = 0.5f;
        inline static std::string ProviderName = "Entity";
        inline static std::string StructName = "Entity";
        inline static std::string GLSLStruct = R"(
struct Entity {
    int id;
    int shape_index;
    int componentsCount;
    int parent_index;
    int components[ECS_MAX_COMPONENTS];
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
    };
        
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
}