#include <dz/ECS/Shape.hpp>
namespace dz {
    int ecs::RegisterPlaneShape() {
        return Shape::RegisterShape("Plane", R"(
    vec3 GetPlaneVertex(in Entity entity) {
        const vec3 plane_vertices[6] = vec3[6](
            vec3(-0.5, -0.5, 0),
            vec3(-0.5,  0.5, 0),
            vec3( 0.5,  0.5, 0),
            vec3( 0.5,  0.5, 0),
            vec3( 0.5, -0.5, 0),
            vec3(-0.5, -0.5, 0)
        );
        return plane_vertices[gl_VertexIndex];
    }
    )");
    }

    int ecs::RegisterCubeShape() {
        return Shape::RegisterShape("Cube", R"(
        vec3 GetCubeVertex(in Entity entity)
        {
            const vec3 cube_vertices[36] = vec3[36](
                // Front face
                vec3(-0.5, -0.5,  0.5),
                vec3(-0.5,  0.5,  0.5),
                vec3( 0.5,  0.5,  0.5),
                vec3( 0.5,  0.5,  0.5),
                vec3( 0.5, -0.5,  0.5),
                vec3(-0.5, -0.5,  0.5),

                // Back face
                vec3( 0.5, -0.5, -0.5),
                vec3( 0.5,  0.5, -0.5),
                vec3(-0.5,  0.5, -0.5),
                vec3(-0.5,  0.5, -0.5),
                vec3(-0.5, -0.5, -0.5),
                vec3( 0.5, -0.5, -0.5),

                // Left face
                vec3(-0.5, -0.5, -0.5),
                vec3(-0.5,  0.5, -0.5),
                vec3(-0.5,  0.5,  0.5),
                vec3(-0.5,  0.5,  0.5),
                vec3(-0.5, -0.5,  0.5),
                vec3(-0.5, -0.5, -0.5),

                // Right face
                vec3( 0.5, -0.5,  0.5),
                vec3( 0.5,  0.5,  0.5),
                vec3( 0.5,  0.5, -0.5),
                vec3( 0.5,  0.5, -0.5),
                vec3( 0.5, -0.5, -0.5),
                vec3( 0.5, -0.5,  0.5),

                // Top face
                vec3(-0.5,  0.5,  0.5),
                vec3(-0.5,  0.5, -0.5),
                vec3( 0.5,  0.5, -0.5),
                vec3( 0.5,  0.5, -0.5),
                vec3( 0.5,  0.5,  0.5),
                vec3(-0.5,  0.5,  0.5),

                // Bottom face
                vec3(-0.5, -0.5, -0.5),
                vec3(-0.5, -0.5,  0.5),
                vec3( 0.5, -0.5,  0.5),
                vec3( 0.5, -0.5,  0.5),
                vec3( 0.5, -0.5, -0.5),
                vec3(-0.5, -0.5, -0.5)
            );
            return cube_vertices[gl_VertexIndex];
        }
        )");
    }
}