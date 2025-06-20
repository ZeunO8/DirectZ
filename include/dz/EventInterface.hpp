#pragma once
#include "Window.hpp"
namespace dz
{
    struct EventInterface
    {
    private:
        WINDOW* window;
    public:
        EventInterface(WINDOW* window);
#ifdef __ANDROID__
        void touch_event(int action, int pointer_index, int pointer_id, float x, float y, float pressure, float size);
        void recreate_window(ANativeWindow* android_window, float width, float height);
        void destroy_surface();
#endif
    };
}