#include <dz/ECS/Shape.hpp>

namespace dz {

std::pair<int, int> ecs::RegisterPlaneShape()
{
    return Shape::RegisterShape(
        "Plane",
        R"(
vec3 GetPlaneVertex(in Entity entity)
{
    const vec3 plane_vertices[6] = vec3[6](
        vec3( 0.5,  0.5, 0),
        vec3(-0.5,  0.5, 0),
        vec3(-0.5, -0.5, 0),
        vec3(-0.5, -0.5, 0),
        vec3( 0.5, -0.5, 0),
        vec3( 0.5,  0.5, 0)
    );
    return plane_vertices[gl_VertexIndex];
}
)",
        R"(
vec3 GetPlaneNormal(in Entity entity)
{
    return vec3(0.0, 0.0, 1.0);
}
)"
    );
}

std::pair<int, int> ecs::RegisterCubeShape()
{
    return Shape::RegisterShape(
        "Cube",
        R"(
vec3 GetCubeVertex(in Entity entity)
{
    const vec3 cube_vertices[36] = vec3[36](
        vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5, -0.5,  0.5),
        vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3( 0.5,  0.5,  0.5),

        vec3(-0.5,  0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5, -0.5, -0.5),
        vec3( 0.5, -0.5, -0.5), vec3(-0.5, -0.5, -0.5), vec3(-0.5,  0.5, -0.5),

        vec3(-0.5,  0.5,  0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5, -0.5, -0.5),
        vec3(-0.5, -0.5, -0.5), vec3(-0.5, -0.5,  0.5), vec3(-0.5,  0.5,  0.5),

        vec3( 0.5,  0.5, -0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5, -0.5,  0.5),
        vec3( 0.5, -0.5,  0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5,  0.5, -0.5),

        vec3( 0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5,  0.5,  0.5),
        vec3(-0.5,  0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5,  0.5, -0.5),

        vec3( 0.5, -0.5,  0.5), vec3(-0.5, -0.5,  0.5), vec3(-0.5, -0.5, -0.5),
        vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5, -0.5,  0.5)
    );
    return cube_vertices[gl_VertexIndex];
}
)",
        R"(
vec3 GetCubeNormal(in Entity entity)
{
    const vec3 cube_normals[36] = vec3[36](
        vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1),
        vec3(0, 0, -1), vec3(0, 0, -1), vec3(0, 0, -1), vec3(0, 0, -1), vec3(0, 0, -1), vec3(0, 0, -1),
        vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0),
        vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0),
        vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0),
        vec3(0, -1, 0), vec3(0, -1, 0), vec3(0, -1, 0), vec3(0, -1, 0), vec3(0, -1, 0), vec3(0, -1, 0)
    );
    return cube_normals[gl_VertexIndex];
}
)"
    );
}

} // namespace dz
