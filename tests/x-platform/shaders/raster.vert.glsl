#version 450

#include "shaders/quad.glsl"

void main() {
    mat4 model = get_quad_model(gl_InstanceIndex);
    gl_Position = model * vec4(get_quad_position(gl_VertexIndex), 1);
}
