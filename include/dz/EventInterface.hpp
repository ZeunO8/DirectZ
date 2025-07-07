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
#elif defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
        void cursor_move(float x, float y);
        void key_press(KEYCODES key, int pressed);
        void cursor_press(int button, int pressed);
#endif
    };
}