#version 450

#include "shaders/quad.glsl"

#include "shaders/camera.glsl"

void main() {
    mat4 model = get_quad_model(gl_InstanceIndex);
    Camera camera = Cameras.cameras[0];
    vec4 worldPos = camera.mvp * model * vec4(get_quad_position(gl_VertexIndex), 1);
    gl_Position = worldPos;
}
