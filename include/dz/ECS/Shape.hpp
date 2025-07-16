#pragma once
#include "Provider.hpp"

namespace dz::ecs {
    struct Shape : Provider<Shape> {
        int type = 0;
        int vertex_count = 0;
        inline static float Priority = 1.0f;
        inline static std::string ProviderName = "Shape";
        inline static std::string StructName = "Shape";
        inline static std::string GLSLStruct = R"(
struct Shape {
    int type;
    int vertex_count;
};
)";
        inline static std::string GLSLMethods = R"(
vec3 GetShapeVertex(in Entity entity) {
    Shape shape = Shapes.data[entity.shape_index];
    switch (shape.type) {
    // [INSERT_SHAPE_VERTEX_CASE]
    default: return vec3(0, 0, 0);
    }
}
vec3 GetShapeNormal(in Entity entity) {
    Shape shape = Shapes.data[entity.shape_index];
    switch (shape.type) {
    // [INSERT_SHAPE_NORMAL_CASE]
    default: return vec3(0, 0, 0);
    }
}
)";
        inline static std::vector<std::pair<float, std::string>> GLSLMain = {
            {0.0, R"(
    vec3 shape_vertex = GetShapeVertex(entity);
    vec3 shape_normal = GetShapeNormal(entity);
)"}
        };

        inline static std::unordered_map<std::string, size_t> registered_shapes = {};
        inline static int RegisterShape(
            const std::string& shape_name,
            const std::string& get_vertex_fn_string,
            const std::string& get_normal_fn_string
        ) {
            auto shape_id = GlobalUID::GetNew("ECS:Shape");
            registered_shapes[shape_name] = shape_id;

            {
                // Generate and insert vertex case
                static std::string vertex_find_str = "[INSERT_SHAPE_VERTEX_CASE]\n";
                auto find_pos = GLSLMethods.find(vertex_find_str);
                assert(find_pos != std::string::npos);
                auto insert_pos = find_pos + vertex_find_str.size();
                std::string insert_type_case_string = R"(
    case )" + std::to_string(shape_id) + R"(:
        return Get)" + shape_name + R"(Vertex(entity);
)";
                GLSLMethods.insert(GLSLMethods.begin() + insert_pos,
                    insert_type_case_string.begin(),
                    insert_type_case_string.end());
            }
            {
                // Generate and insert normal case
                static std::string normal_find_str = "[INSERT_SHAPE_NORMAL_CASE]\n";
                auto find_pos = GLSLMethods.find(normal_find_str);
                assert(find_pos != std::string::npos);
                auto insert_pos = find_pos + normal_find_str.size();
                std::string insert_type_case_string = R"(
    case )" + std::to_string(shape_id) + R"(:
        return Get)" + shape_name + R"(Normal(entity);
)";
                GLSLMethods.insert(GLSLMethods.begin() + insert_pos,
                    insert_type_case_string.begin(),
                    insert_type_case_string.end());
            }

            // Insert vertex + normal fns
            GLSLMethods.insert(GLSLMethods.begin(),
                get_normal_fn_string.begin(),
                get_normal_fn_string.end());

            GLSLMethods.insert(GLSLMethods.begin(),
                get_vertex_fn_string.begin(),
                get_vertex_fn_string.end());

            return int(shape_id);
        }
    };

    int RegisterPlaneShape();
    int RegisterCubeShape();
}