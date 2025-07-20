#pragma once
#include "Provider.hpp"
#include "Shape.hpp"

#define ECS_MAX_COMPONENTS 8
namespace dz::ecs {
    struct Entity : Provider<Entity> {
        int id = 0;
        int shape_index;
        int componentsCount = 0;
        int components[ECS_MAX_COMPONENTS] = {0};
        
        inline static float Priority = 0.5f;
        inline static std::string ProviderName = "Entity";
        inline static std::string StructName = "Entity";
        inline static std::string GLSLStruct = R"(
struct Entity {
    int id;
    int shape_index;
    int componentsCount;
    int components[ECS_MAX_COMPONENTS];
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
}