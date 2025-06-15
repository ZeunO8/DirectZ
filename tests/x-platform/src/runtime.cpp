#include <DirectZ.hpp>
int main()
{
    auto window = window_create({
        .title = "Example Window",
        .x = 128,
        .y = 128,
        .width = 640,
        .height = 480
    });

    int ret = 0;

    // call app-lib implementation of dz_init
    if ((ret = dz_init(window)))
        return ret;
    while (window_poll_events(window))
    {
        // update and render
        dz_update();
        window_render(window);
    }
    return ret;
}