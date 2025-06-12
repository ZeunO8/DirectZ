#pragma once
#include <string>
#include "math.hpp"
#include "DrawListManager.hpp"
namespace dz
{
    struct WINDOW;
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

    WINDOW* window_create(const WindowCreateInfo& info);

    bool window_poll_events(WINDOW* window);

    void window_render(WINDOW* window);

    float& window_get_frametime_ref(WINDOW* window);

    void window_set_frametime_pointer(WINDOW* window, float* pointer);
    void window_set_keys_pointer(WINDOW* window, int32_t* pointer);
    void window_set_buttons_pointer(WINDOW* window, int32_t* pointer);
    void window_set_cursor_pointer(WINDOW* window, float* pointer);

    void window_set_frametime_pointer(WINDOW* window, const std::shared_ptr<float>& pointer);
    void window_set_keys_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer);
    void window_set_buttons_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer);
    void window_set_cursor_pointer(WINDOW* window, const std::shared_ptr<float>& pointer);

    int32_t& window_get_keypress_ref(WINDOW* window, uint8_t keycode);
    int32_t& window_get_buttonpress_ref(WINDOW* window, uint8_t button);

    std::shared_ptr<int32_t>& window_get_all_keypress_ref(WINDOW* window, uint8_t keycode);
    std::shared_ptr<int32_t>& window_get_all_buttonpress_ref(WINDOW* window, uint8_t button);

    float& window_get_width_ref(WINDOW* window);
    float& window_get_height_ref(WINDOW* window);
    
    void window_add_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group);
    void window_remove_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group);

    void window_free(WINDOW* window);
    
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
}