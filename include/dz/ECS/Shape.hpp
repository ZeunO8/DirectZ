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
        // [INSERT_SHAPE_CASE]
        default: return vec3(0, 0, 0);
        }
    }
    )";
        inline static std::vector<std::pair<float, std::string>> GLSLMain = {};

        inline static std::unordered_map<std::string, size_t> registered_shapes = {};
        inline static int RegisterShape(
            const std::string& shape_name,
            const std::string& get_vertex_fn_string
        ) {
            auto shape_id = GlobalUID::GetNew("ECS:Shape");
            registered_shapes[shape_name] = shape_id;
            static std::string find_str = "[INSERT_SHAPE_CASE]\n";
            auto find_pos = GLSLMethods.find(find_str);
            assert(find_pos != std::string::npos);
            auto insert_pos = find_pos + find_str.size();
            std::string insert_type_case_string = R"(
        case )" + std::to_string(shape_id) + R"(:
            return Get)" + shape_name + R"(Vertex(entity);
    )";
            GLSLMethods.insert(GLSLMethods.begin() + insert_pos,
                insert_type_case_string.begin(),
                insert_type_case_string.end());

            GLSLMethods.insert(GLSLMethods.begin(),
                get_vertex_fn_string.begin(),
                get_vertex_fn_string.end());

            return int(shape_id);
        }
    };

    int RegisterPlaneShape();
    int RegisterCubeShape();
}