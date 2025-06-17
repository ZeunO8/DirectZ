#include <DirectZ.hpp>

WINDOW* cached_window = 0;

DZ_EXPORT int init(const WindowCreateInfo& window_info)
{
    cached_window = window_create(window_info);
    Shader* compute_shader = shader_create();
    Shader* raster_shader = shader_create();

    // See "Shaders" section for information on creating shaders
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