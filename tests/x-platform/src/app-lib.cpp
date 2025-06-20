#include <DirectZ.hpp>

WINDOW* cached_window = 0;

// #define DOUBLE_PRECISION
#if defined(DOUBLE_PRECISION)
using Real = double;
#else
using Real = float;
#endif
using vec3 = vec<Real, 3>;
using vec4 = vec<Real, 4>;
using mat4 = mat<Real, 4, 4>;

struct Quad
{
    vec4 position;
};

struct Window
{
    float deltaTime;
    float time;
};

struct Camera
{
    mat4 view;
    mat4 proj;
};

std::shared_ptr<DrawListManager<Quad>> quad_draw_list_mg_ptr;
Shader* raster_shader = 0;
Shader* compute_shader = 0;
std::vector<Quad*> quad_ptrs;
std::vector<Window*> window_ptrs;
std::vector<Camera*> camera_ptrs;

DZ_EXPORT EventInterface* init(const WindowCreateInfo& window_info)
{
    cached_window = window_create(window_info);

    FileHandle asset_pack_handle{
        .location = FileHandle::ASSET,
        .path = "pack.bin"
    };
    auto asset_pack = create_asset_pack(asset_pack_handle);
    
    auto main_buffer_group = buffer_group_create("main_buffer_group");
    
    auto windows_buffer_group = buffer_group_create("windows_buffer_group");
    
    auto cameras_buffer_group = buffer_group_create("cameras_buffer_group");

    buffer_group_restrict_to_keys(main_buffer_group, {"Quads"});

    buffer_group_restrict_to_keys(windows_buffer_group, {"Windows"});

    buffer_group_restrict_to_keys(cameras_buffer_group, {"Cameras"});
    
    raster_shader = shader_create();

    shader_add_buffer_group(raster_shader, main_buffer_group);

    shader_add_buffer_group(raster_shader, cameras_buffer_group);

    shader_include_asset_pack(raster_shader, asset_pack);

    quad_draw_list_mg_ptr = std::make_shared<DrawListManager<Quad>>("Quads", [&](auto buffer_group, auto& quad) -> std::pair<Shader*, uint32_t> {
        auto chosen_shader = raster_shader;
        uint32_t vert_count = 6;
        return {chosen_shader, vert_count};
    });

    auto& quad_draw_list_mg = *quad_draw_list_mg_ptr;

    window_add_drawn_buffer_group(cached_window, &quad_draw_list_mg, main_buffer_group);

    Asset raster_vert_glsl;
    auto asset_available = get_asset(asset_pack, "shaders/raster.vert.glsl", raster_vert_glsl);
    
    std::string raster_vert_string(raster_vert_glsl.ptr);
    shader_add_module(raster_shader, ShaderModuleType::Vertex, raster_vert_string);

    Asset raster_frag_glsl;
    asset_available = get_asset(asset_pack, "shaders/raster.frag.glsl", raster_frag_glsl);
    
    std::string raster_frag_string(raster_frag_glsl.ptr);
    shader_add_module(raster_shader, ShaderModuleType::Fragment, raster_frag_string);
    
    compute_shader = shader_create();

    shader_add_buffer_group(compute_shader, main_buffer_group);
    shader_add_buffer_group(compute_shader, windows_buffer_group);

    shader_include_asset_pack(compute_shader, asset_pack);

    Asset quad_pos_glsl;
    asset_available = get_asset(asset_pack, "shaders/quad_pos.comp.glsl", quad_pos_glsl);
    
    std::string quad_pos_string(quad_pos_glsl.ptr);
    shader_add_module(compute_shader, ShaderModuleType::Compute, quad_pos_string);

    quad_ptrs.resize(1);
    buffer_group_set_buffer_element_count(main_buffer_group, "Quads", quad_ptrs.size());

    window_ptrs.resize(1);
    buffer_group_set_buffer_element_count(windows_buffer_group, "Windows", window_ptrs.size());

    camera_ptrs.resize(1);
    buffer_group_set_buffer_element_count(cameras_buffer_group, "Cameras", camera_ptrs.size());
    
    buffer_group_initialize(main_buffer_group);
    buffer_group_initialize(windows_buffer_group);
    buffer_group_initialize(cameras_buffer_group);

    auto& window_width_ptr = window_get_width_ref(cached_window);
    auto& window_width = *window_width_ptr;
    auto& window_height_ptr = window_get_height_ref(cached_window);
    auto& window_height = *window_height_ptr;

    for (size_t i = 0; i < quad_ptrs.size(); ++i)
    {
        auto quad_view = buffer_group_get_buffer_element_view(main_buffer_group, "Quads", i);
        auto& quad = quad_view.as_struct<Quad>();
        quad_ptrs[i] = &quad;
    }

    for (auto qp : quad_ptrs)
    {
        qp->position = vec4(0.2, 0, 0, 1);
    }

    for (size_t i = 0; i < window_ptrs.size(); ++i)
    {
        auto window_view = buffer_group_get_buffer_element_view(windows_buffer_group, "Windows", i);
        auto& window = window_view.as_struct<Window>();
        window_ptrs[i] = &window;
    }

    for (auto wp : window_ptrs)
    {
        window_set_float_frametime_pointer(cached_window, &wp->deltaTime);
    }

    for (size_t i = 0; i < camera_ptrs.size(); ++i)
    {
        auto camera_view = buffer_group_get_buffer_element_view(cameras_buffer_group, "Cameras", i);
        auto& camera = camera_view.as_struct<Camera>();
        camera_ptrs[i] = &camera;
    }

    for (auto cp : camera_ptrs)
    {
        cp->view = lookAt<Real>({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
        cp->proj = perspective<Real>(radians(81.f), window_width / window_height, 0.01, 100.f);
    }

    free_asset_pack(asset_pack);

    return window_get_event_interface(cached_window);
}

DZ_EXPORT bool poll_events()
{
    return window_poll_events(cached_window);
}

DZ_EXPORT void update()
{
    auto& window = *window_ptrs[0];
    auto dt = window.deltaTime;
    auto& t = window.time;
    t += dt;
    // Do any CPU side logic
}

DZ_EXPORT void render()
{
    shader_dispatch(compute_shader, {quad_ptrs.size(), 1, 1});
    window_render(cached_window);
}