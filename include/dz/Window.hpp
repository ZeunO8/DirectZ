/**
 * @file Window.hpp
 * @brief Defines functions to create, configure, and render Windows
 */
#pragma once
#include <string>
#include "math.hpp"
#include "DrawListManager.hpp"
#ifdef __ANDROID__
#include <android/native_window_jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/input.h>
#include <android/configuration.h>
#endif
namespace dz
{
    struct WINDOW;
    struct EventInterface;
    struct ImGuiLayer;
    struct WindowCreateInfo
    {
        std::string title;
        float x;
        float y;
        float width;
        float height;
        bool borderless = false;
        bool vsync = true;
#ifdef __ANDROID__
        ANativeWindow* android_window = 0;
        AAssetManager* android_asset_manager = 0;
#endif
    };

    /**
    * @brief Creates a Window given the specified information and sets up a render context.
    * 
    * This function must be called before creating any resources in your program.
    */
    WINDOW* window_create(const WindowCreateInfo& info);

    /**
    * @brief Polls a window for events.
    * 
    * This should be called in a loop before any update and render code. Returns true if the window is still active, false if it has been closed.
    */
    bool window_poll_events(WINDOW* window);

    /**
    * @brief Renders the specified window based on its context configuration.
    */
    void window_render(WINDOW* window);

    /**
    * @brief Gets the ImGuiLayer of a window
    */
    ImGuiLayer& window_get_ImGuiLayer(WINDOW* window);

    /**
    * @brief Returns a reference to the frametime value as a float.
    */
    float& window_get_float_frametime_ref(WINDOW* window);

    /**
    * @brief Returns a reference to the frametime value as a double.
    */
    double& window_get_double_frametime_ref(WINDOW* window);

    // Raw pointer setters

    /**
    * @brief Sets the underlying frametime float pointer, useful for binding to GPU memory.
    */
    void window_set_float_frametime_pointer(WINDOW* window, float* pointer);

    /**
    * @brief Sets the underlying frametime double pointer, useful for binding to GPU memory.
    */
    void window_set_double_frametime_pointer(WINDOW* window, double* pointer);

    /**
    * @brief Sets the underlying keys pointer, useful for binding to GPU memory.
    * 
    * @note Keys expects the pointer to have a size of 256 * sizeof(int32_t).
    */
    void window_set_keys_pointer(WINDOW* window, int32_t* pointer);

    /**
    * @brief Sets the underlying buttons pointer, useful for binding to GPU memory.
    * 
    * @note Buttons expects the pointer to have a size of 8 * sizeof(int32_t).
    */
    void window_set_buttons_pointer(WINDOW* window, int32_t* pointer);

    /**
    * @brief Sets the underlying cursor pointer, useful for binding to GPU memory.
    * 
    * @note Cursor expects the pointer to have a size of 2 * sizeof(float).
    */
    void window_set_cursor_pointer(WINDOW* window, float* pointer);

    /**
    * @brief Sets the underlying mod pointer, useful for binding to GPU memory.
    */
    void window_set_mod_pointer(WINDOW* window, int32_t* pointer);

    /**
    * @brief Sets the underlying focused pointer, useful for binding to GPU memory.
    */
    void window_set_focused_pointer(WINDOW* window, int32_t* pointer);

    /**
    * @brief Sets the underlying width pointer, useful for binding to GPU memory.
    */
    void window_set_width_pointer(WINDOW* window, float* pointer);

    /**
    * @brief Sets the underlying height pointer, useful for binding to GPU memory.
    */
    void window_set_height_pointer(WINDOW* window, float* pointer);

    // std::shared_ptr setters

    /**
    * @brief Sets the underlying frametime float pointer, useful for binding to GPU memory.
    */
    void window_set_float_frametime_pointer(WINDOW* window, const std::shared_ptr<float>& pointer);

    /**
    * @brief Sets the underlying frametime double pointer, useful for binding to GPU memory.
    */
    void window_set_double_frametime_pointer(WINDOW* window, const std::shared_ptr<double>& pointer);

    /**
    * @brief Sets the underlying keys pointer, useful for binding to GPU memory.
    * 
    * @note Keys expects the pointer to have a size of 256 * sizeof(int32_t).
    */
    void window_set_keys_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying buttons pointer, useful for binding to GPU memory.
    * 
    * @note Buttons expects the pointer to have a size of 8 * sizeof(int32_t).
    */
    void window_set_buttons_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying cursor pointer, useful for binding to GPU memory.
    * 
    * @note Cursor expects the pointer to have a size of 2 * sizeof(float).
    */
    void window_set_cursor_pointer(WINDOW* window, const std::shared_ptr<float>& pointer);

    /**
    * @brief Sets the underlying mod pointer, useful for binding to GPU memory.
    */
    void window_set_mod_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying focused pointer, useful for binding to GPU memory.
    */
    void window_set_focused_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying width pointer, useful for binding to GPU memory.
    */
    void window_set_width_pointer(WINDOW* window, const std::shared_ptr<float>& pointer);

    /**
    * @brief Sets the underlying height pointer, useful for binding to GPU memory.
    */
    void window_set_height_pointer(WINDOW* window, const std::shared_ptr<float>& pointer);

    /**
    * @brief Gets a reference to the specified key in the underlying keys pointer.
    * 
    * @note If the keys pointer is updated (e.g., to GPU memory), a previously fetched reference becomes invalid.
    */
    int32_t& window_get_keypress_ref(WINDOW* window, uint8_t keycode);

    /**
    * @brief Gets a reference to the specified button in the underlying buttons pointer.
    * 
    * @note If the buttons pointer is updated (e.g., to GPU memory), a previously fetched reference becomes invalid.
    */
    int32_t& window_get_buttonpress_ref(WINDOW* window, uint8_t button);

    /**
    * @brief Gets a shared pointer reference to all key values.
    */
    std::shared_ptr<int32_t>& window_get_all_keypress_ref(WINDOW* window, uint8_t keycode);

    /**
    * @brief Gets a shared pointer reference to all button values.
    */
    std::shared_ptr<int32_t>& window_get_all_buttonpress_ref(WINDOW* window, uint8_t button);

    /**
    * @brief Gets a shared pointer reference to window width.
    */
    std::shared_ptr<float>& window_get_width_ref(WINDOW* window);

    /**
    * @brief Gets a shared pointer reference to window height.
    */
    std::shared_ptr<float>& window_get_height_ref(WINDOW* window);

    /**
    * @brief Adds a drawn buffer group.
    * 
    * Registers a DrawListManager along with a compatible buffer group so draw commands can be computed dynamically based on buffer data.
    */
    void window_add_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group);

    /**
    * @brief Removes a drawn buffer group.
    */
    void window_remove_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group);

    /**
    * @brief Returns the EventInterface for a given WINDOW
    */
    EventInterface* window_get_event_interface(WINDOW* window);

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
#define KEYCODE_PAUSE 0x8A
#define KEYCODE_SUPER 0x8B
#define LAST_UNDEFINED_ASCII_IN_RANGE 0x9F
}