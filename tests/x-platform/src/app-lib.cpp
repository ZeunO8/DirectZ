#include <DirectZ.hpp>

WINDOW* cached_window = 0;

int dz_init(WINDOW* window)
{
    cached_window = window;
    Shader* compute_shader = shader_create();
    Shader* raster_shader = shader_create();

    // See "Shaders" section for information on creating shaders
    return 0;
}

void dz_update()
{
    // Do any CPU side logic
}