struct Quad {
   vec4 position;
};

layout(std430, binding = 0) buffer QuadsBuffer {
    Quad quads[];
} Quads;

vec3 get_quad_position(int index)
{
    const vec3 positions[6] = vec3[6](
        vec3(0.5, 0.5, 0),
        vec3(-0.5, 0.5, 0),
        vec3(-0.5, -0.5, 0),
        vec3(-0.5, -0.5, 0),
        vec3(0.5, -0.5, 0),
        vec3(0.5, 0.5, 0)
    );

    return positions[index % 6];
}

vec2 get_quad_uv(int index)
{
    const vec2 uvs[6] = vec2[6](
        vec2(1, 1),
        vec2(0, 1),
        vec2(0, 0),
        vec2(0, 0),
        vec2(1, 0),
        vec2(1, 1)
    );

    return uvs[index % 6];
}

mat4 get_quad_model(int index)
{
    mat4 model = mat4(1.0);
    Quad quad = Quads.quads[index];
    model[3][0] = quad.position.x;
    model[3][1] = quad.position.y;
    model[3][2] = quad.position.z;
    return model;
}