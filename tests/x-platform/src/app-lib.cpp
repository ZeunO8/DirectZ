#include <DirectZ.hpp>

WINDOW* cached_window = 0;

DZ_EXPORT int init(const WindowCreateInfo& window_info)
{
    cached_window = window_create(window_info);

    auto main_buffer_group = buffer_group_create("main_buffer_group");

    Shader* raster_shader = shader_create();
    
    shader_add_module(raster_shader, ShaderModuleType::Vertex,
    DZ_GLSL_VERSION + R"(
    struct Quad {
        vec4 viewport;
    };

    layout(std430, binding = 0) buffer QuadsBuffer {
        Quad quads[];
    } Quads;

    vec3 get_quad_position()
    {
        vec3 pos;
        switch (gl_VertexIndex)
        {
        case 0: pos = vec3(1.0, 1.0, 0);    break;
        case 1: pos = vec3(-1.0, 1.0, 0);   break;
        case 2: pos = vec3(-1.0, -1.0, 0);  break;
        case 3: pos = vec3(-1.0, -1.0, 0);  break;
        case 4: pos = vec3(1.0, -1.0, 0);   break;
        case 5: pos = vec3(1.0, 1.0, 0);    break;
        }
        // vec4 viewport = Quads.quads[gl_InstanceIndex].viewport;
        return pos;
    }

    vec2 get_quad_uv()
    {
        switch (gl_VertexIndex)
        {
        case 0: return vec2(1, 1);
        case 1: return vec2(0, 1);
        case 2: return vec2(0, 0);
        case 3: return vec2(0, 0);
        case 4: return vec2(1, 0);
        case 5: return vec2(1, 1);
        }
        return vec2(0);
    }

    void main() {
        gl_Position = vec4(get_quad_position(), 1);
    }
    )");

    shader_add_module(raster_shader, ShaderModuleType::Fragment,
    DZ_GLSL_VERSION + R"(
    layout(location = 0) out vec4 outColor;
    void main() {
        outColor = vec4(1, 0, 0, 1);
    }
    )");
    
    Shader* compute_shader = shader_create();
    return 0;
}

DZ_EXPORT bool poll_events()
{
    return window_poll_events(cached_window);
}

DZ_EXPORT void update()
{
    // Do any CPU side logic
}

DZ_EXPORT void render()
{
    window_render(cached_window);
}