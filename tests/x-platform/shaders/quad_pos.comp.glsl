#version 450

layout(local_size_x = 1) in;

#include "shaders/quad.glsl"

#include "shaders/window.glsl"

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= Quads.quads.length())
        return;
    Window window = Windows.windows[0];
    if (window.buttons[0] == 1)
    {
        float x = ((window.cursor[0] / window.width) - 0.5) * 5.0;
        float y = ((window.cursor[1] / window.height) - 0.5) * 5.0;
        Quads.quads[id].position.x = x;
        Quads.quads[id].position.y = y;
    }
    else
    {
        float wave = sin(window.time + id * 0.3);
        Quads.quads[id].position.y = wave * 0.5;
    }
}