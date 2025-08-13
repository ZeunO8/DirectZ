#include <dz/Runtime.hpp>
int main()
{
    auto main_window = api_window_create("Example Window", 640, 480);

    if (!api_init(main_window))
        return 1;

    while (api_poll_events())
    {
        api_update();
        api_render();
    }

    return 0;
}
