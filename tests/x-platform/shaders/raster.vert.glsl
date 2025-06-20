#version 450

#include "shaders/quad.glsl"

#include "shaders/camera.glsl"

void main() {
    mat4 model = get_quad_model(gl_InstanceIndex);
    Camera camera = Cameras.cameras[0];
    vec4 worldPos = model * vec4(get_quad_position(gl_VertexIndex), 1);
    vec4 viewPos  = camera.view * worldPos;
    vec4 clipPos  = camera.proj * viewPos;
    gl_Position = vec4(clipPos);
}
