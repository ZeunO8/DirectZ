#include <DirectZ.hpp>

DZ_EXPORT void api_set_direct_registry(DirectRegistry* new_dr_ptr) {
    set_direct_registry(new_dr_ptr);
}

DZ_EXPORT dz::WINDOW* api_window_create(const char* title, float width, float height
#if defined(ANDROID)
, ANativeWindow* android_window, AAssetManager* android_asset_manager
#endif
    , bool headless, Image* headless_image
) {
    return window_create({
        .title = title,
        .width = width,
        .height = height,
        .borderless = true,
        .vsync = true,
#if defined(ANDROID)
        .android_window = android_window,
        .android_asset_manager = android_asset_manager,
#endif
        .headless = headless,
        .headless_image = headless_image,
    });
}

struct Quad {
    float x;
    float y;
    float w;
    float h;
};

struct Window {
    double iTime;
    float width;
    float height;
};

dz::Shader* art_quad_shader = nullptr;
dz::BufferGroup* quad_buffer_group = nullptr;
dz::DrawListManager<Quad> draw_mg("Quads", [] (auto buffer_group, auto& quad) -> dz::DrawTuple {
    return {art_quad_shader, 6};
});

std::string GenerateArtQuadVertexShader();
std::string GenerateArtQuadFragmentShader();

DZ_EXPORT bool api_init(dz::WINDOW* window) {
    window_set_clear_color(window, {0.2, 0.5, 0.4, 1});

    quad_buffer_group = buffer_group_create("QuadBufferGroup");

    buffer_group_restrict_to_keys(quad_buffer_group, {"Quads", "Windows"});

    window_add_drawn_buffer_group(window, &draw_mg, quad_buffer_group);

    art_quad_shader = shader_create();

    shader_add_buffer_group(art_quad_shader, quad_buffer_group);

    shader_add_module(art_quad_shader, dz::ShaderModuleType::Vertex, GenerateArtQuadVertexShader());
    shader_add_module(art_quad_shader, dz::ShaderModuleType::Fragment, GenerateArtQuadFragmentShader());

    buffer_group_set_buffer_element_count(quad_buffer_group, "Quads", 1);
    buffer_group_set_buffer_element_count(quad_buffer_group, "Windows", 1);

    buffer_group_initialize(quad_buffer_group);

    auto window_view = buffer_group_get_buffer_element_view(quad_buffer_group, "Windows", 0);
    auto& gl_window = window_view.as_struct<Window>();

    window_set_double_iTime_pointer(window, std::shared_ptr<double>(&gl_window.iTime, [](auto ptr) { }));
    window_set_width_pointer(window, std::shared_ptr<float>(&gl_window.width, [](auto ptr) { }));
    window_set_height_pointer(window, std::shared_ptr<float>(&gl_window.height, [](auto ptr) { }));

    return true;
}

DZ_EXPORT bool api_poll_events() {
    return windows_poll_events();
}

DZ_EXPORT void api_update() {

}

DZ_EXPORT bool api_render() {
    return windows_render();
}

std::string GenerateArtQuadVertexShader() {
    std::string shader_string;
    shader_string += R"(
#version 450

layout(location = 0) out vec4 outPosition;

void main() {
    const vec4 quad_verts[6] = vec4[](
        vec4(1.0, 1.0, 0, 1),
        vec4(-1.0, 1.0, 0, 1),
        vec4(-1.0, -1.0, 0, 1),
        vec4(-1.0, -1.0, 0, 1),
        vec4(1.0, -1.0, 0, 1),
        vec4(1.0, 1.0, 0, 1)
    );
    gl_Position = outPosition = quad_verts[gl_VertexIndex];
}
)";
    return shader_string;
}

std::string GenerateArtQuadFragmentShader() {
    std::string shader_string;
    shader_string += R"(
#version 450

layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec4 FragColor;

struct Quad
{
    float x;
    float y;
    float w;
    float h;
};

layout(std430, binding = 0) buffer QuadsBuffer
{
    Quad data[];
} Quads;

struct Window {
    double iTime;
    float width;
    float height;
};

layout(std430, binding = 1) buffer WindowsBuffer {
    Window data[];
} Windows;

vec3 pallete(float t) {
    const vec3 a = vec3(0.5, 0.5, 0.5);
    const vec3 b = vec3(0.5, 0.5, 0.5);
    const vec3 c = vec3(1.0, 1.0, 1.0);
    const vec3 d = vec3(0.274, 0.428, 0.608);
    return a + b*cos(6.28318*(c*t+d));
}

void main() {
    Window window = Windows.data[0];
    float f_iTime = float(window.iTime);

    vec2 uv = inPosition.xy;
    uv.x *= window.width / window.height;

    vec2 uv0 = uv;

    vec3 finalColor = vec3(0);

    for (float i = 0.0; i < 4.0; i++) {
        uv = fract(uv * 1.7) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        vec3 col = pallete(length(uv0) + i * fract(.3835) + f_iTime * .43);

        d = sin(d*8. + f_iTime / 1.8)/8.;
        d = abs(d);

        d = pow(0.009 / d, 1.18425513);

        finalColor += col * d;
    }

    FragColor = vec4(finalColor, 1);
}
)";
    return shader_string;
}