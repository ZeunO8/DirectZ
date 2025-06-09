#pragma once
#include <string>
#include "math.hpp"
namespace dz
{
    struct Window;
    struct WindowCreateInfo
    {
        std::string title;
        float x;
        float y;
        float width;
        float height;
        bool borderless = false;
        bool vsync = true;
    };
#define KEYCODE_ESCAPE 27
#define KEYCODE_DELETE 127
#define KEYCODE_UP 17
#define KEYCODE_DOWN 18
#define KEYCODE_RIGHT 19
#define KEYCODE_LEFT 20
#define KEYCODE_HOME 0x80
#define KEYCODE_END 0x81
#define KEYCODE_PGUP 0x82
#define KEYCODE_PGDOWN 0x83
#define KEYCODE_INSERT 0x84
#define KEYCODE_NUMLOCK 0x85
#define KEYCODE_CAPSLOCK 0x86
#define KEYCODE_CTRL 0x87
#define KEYCODE_SHIFT 0x88
#define KEYCODE_ALT 0x89
#define KEYCODE_PAUSE 0x87
#define KEYCODE_SUPER 0x88
#define LAST_UNDEFINED_ASCII_IN_RANGE 0x9F
    Window* window_create(const WindowCreateInfo& info);
    bool window_poll_events(Window* window);
    void window_render(Window* window);
    float& window_get_frametime_ref(Window* window);
    bool& window_get_keypress_ref(Window* window, uint8_t keycode);
    vec<bool, 256>& window_get_all_keypress_ref(Window* window, uint8_t keycode);
    void window_free(Window* window);
}