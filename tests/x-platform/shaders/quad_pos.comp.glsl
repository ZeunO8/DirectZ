#version 450

layout(local_size_x = 1) in;

#include "shaders/quad.glsl"

#include "shaders/window.glsl"

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= Quads.quads.length())
        return;

    float wave = sin(Windows.windows[0].time + id * 0.3); // slight phase offset

    Quads.quads[id].position.y = wave * 0.5;
}