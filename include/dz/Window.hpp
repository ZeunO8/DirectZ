/**
 * @file Window.hpp
 * @brief Defines functions to create, configure, and render Windows
 */
#pragma once
#include <string>
#include <functional>
#include "math.hpp"
#include "DrawListManager.hpp"
#include "Reflectable.hpp"
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

    struct WindowReflectableGroup : ReflectableGroup {

    private:
        WINDOW* window_ptr;
        std::vector<Reflectable*> reflectables;
    public:
        WindowReflectableGroup(WINDOW* window_ptr);
        ~WindowReflectableGroup();
        GroupType GetGroupType() override {
            return ReflectableGroup::Window;
        }
        std::string& GetName() override;
        const std::vector<Reflectable*>& GetReflectables() override;
    };

    /**
    * @brief Provides a mapping of ASCII keys
    *
    * @note Some non printable ASCII codes are used for keys such as RIGHT, HOME, PGUP, etc
    */
    enum class KEYCODES : uint8_t {

        NUL = 0,

        BACKSPACE = 8,
        TAB = 9,
        ENTER = 10,
        
        HOME = 14,
        END = 15,

        UP = 17,
        DOWN = 18,
        RIGHT = 19,
        LEFT = 20,

        PGUP = 21,
        PGDOWN = 22,
        INSERT = 23,
        NUMLOCK = 24,
        CAPSLOCK = 25,
        CTRL = 26,
        
        ESCAPE = 27,
        
        SHIFT = 28,
        ALT = 29,
        PAUSE = 30,
        SUPER = 31,
        
        SPACE = 32,
        EXCLAMATION = 33,
        DOUBLEQUOTE = 34,
        HASHTAG = 35,
        DOLLAR = 36,
        PERCENTSIGN = 37,
        AMPERSAND = 38,
        SINGLEQUOTE = 39,
        LEFTPARENTHESIS = 40,
        RIGHTPARENTHESIS = 41,
        ASTERISK = 42,
        PLUS = 43,
        COMMA = 44,
        MINUS = 45,
        PERIOD = 46,
        SLASH = 47,
        
        _0 = 48,
        _1 = 49,
        _2 = 50,
        _3 = 51,
        _4 = 52,
        _5 = 53,
        _6 = 54,
        _7 = 55,
        _8 = 56,
        _9 = 57,
        
        COLON = 58,
        SEMICOLON = 59,
        LESSTHAN = 60,
        EQUAL = 61,
        GREATERTHAN = 62,
        QUESTIONMARK = 63,
        ATSIGN = 64,
        
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        
        LEFTBRACKET = 91,
        BACKSLASH = 92,
        RIGHTBRACKET = 93,
        CARET = 94,
        UNDERSCORE = 95,
        GRAVEACCENT = 96,
        
        a = 97,
        c = 99,
        b = 98,
        d = 100,
        e = 101,
        f = 102,
        g = 103,
        h = 104,
        i = 105,
        j = 106,
        k = 107,
        l = 108,
        m = 109,
        n = 110,
        o = 111,
        p = 112,
        q = 113,
        r = 114,
        s = 115,
        t = 116,
        u = 117,
        v = 118,
        w = 119,
        x = 120,
        y = 121,
        z = 122,
        
        LEFTBRACE = 123,
        VERTICALBAR = 124,
        RIGHTBRACE = 125,
        TILDE = 126,
        Delete = 127
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
    bool window_poll_events(WINDOW*);

    /**
    * @brief Renders the specified window based on its context configuration.
    */
    void window_render(WINDOW*);

    /**
    * @brief Gets the ImGuiLayer of a window
    */
    ImGuiLayer& window_get_ImGuiLayer(WINDOW*);

    /**
    * @brief Returns a reference to the title
    */
    const std::string& window_get_title_ref(WINDOW*);

    /**
    * @brief sets the title of a given WINDOW
    */
    void window_set_title(WINDOW*, const std::string&);

    /**
    * @brief Returns a reference to the id
    */
    size_t window_get_id_ref(WINDOW*);

    /**
    * @brief Returns a reference to the frametime value as a float.
    */
    float& window_get_float_frametime_ref(WINDOW*);

    /**
    * @brief Returns a reference to the frametime value as a double.
    */
    double& window_get_double_frametime_ref(WINDOW*);

    // Raw pointer setters

    /**
    * @brief Sets the underlying frametime float pointer, useful for binding to GPU memory.
    */
    void window_set_float_frametime_pointer(WINDOW*, float* pointer);

    /**
    * @brief Sets the underlying frametime double pointer, useful for binding to GPU memory.
    */
    void window_set_double_frametime_pointer(WINDOW*, double* pointer);

    /**
    * @brief Sets the underlying keys pointer, useful for binding to GPU memory.
    * 
    * @note Keys expects the pointer to have a size of 256 * sizeof(int32_t).
    */
    void window_set_keys_pointer(WINDOW*, int32_t* pointer);

    /**
    * @brief Sets the underlying buttons pointer, useful for binding to GPU memory.
    * 
    * @note Buttons expects the pointer to have a size of 8 * sizeof(int32_t).
    */
    void window_set_buttons_pointer(WINDOW*, int32_t* pointer);

    /**
    * @brief Sets the underlying cursor pointer, useful for binding to GPU memory.
    * 
    * @note Cursor expects the pointer to have a size of 2 * sizeof(float).
    */
    void window_set_cursor_pointer(WINDOW*, float* pointer);

    /**
    * @brief Sets the underlying mod pointer, useful for binding to GPU memory.
    */
    void window_set_mod_pointer(WINDOW*, int32_t* pointer);

    /**
    * @brief Sets the underlying focused pointer, useful for binding to GPU memory.
    */
    void window_set_focused_pointer(WINDOW*, int32_t* pointer);

    /**
    * @brief Sets the underlying width pointer, useful for binding to GPU memory.
    */
    void window_set_width_pointer(WINDOW*, float* pointer);

    /**
    * @brief Sets the underlying height pointer, useful for binding to GPU memory.
    */
    void window_set_height_pointer(WINDOW*, float* pointer);

    // std::shared_ptr setters

    /**
    * @brief Sets the underlying frametime float pointer, useful for binding to GPU memory.
    */
    void window_set_float_frametime_pointer(WINDOW*, const std::shared_ptr<float>& pointer);

    /**
    * @brief Sets the underlying frametime double pointer, useful for binding to GPU memory.
    */
    void window_set_double_frametime_pointer(WINDOW*, const std::shared_ptr<double>& pointer);

    /**
    * @brief Sets the underlying keys pointer, useful for binding to GPU memory.
    * 
    * @note Keys expects the pointer to have a size of 256 * sizeof(int32_t).
    */
    void window_set_keys_pointer(WINDOW*, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying buttons pointer, useful for binding to GPU memory.
    * 
    * @note Buttons expects the pointer to have a size of 8 * sizeof(int32_t).
    */
    void window_set_buttons_pointer(WINDOW*, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying cursor pointer, useful for binding to GPU memory.
    * 
    * @note Cursor expects the pointer to have a size of 2 * sizeof(float).
    */
    void window_set_cursor_pointer(WINDOW*, const std::shared_ptr<float>& pointer);

    /**
    * @brief Sets the underlying mod pointer, useful for binding to GPU memory.
    */
    void window_set_mod_pointer(WINDOW*, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying focused pointer, useful for binding to GPU memory.
    */
    void window_set_focused_pointer(WINDOW*, const std::shared_ptr<int32_t>& pointer);

    /**
    * @brief Sets the underlying width pointer, useful for binding to GPU memory.
    */
    void window_set_width_pointer(WINDOW*, const std::shared_ptr<float>& pointer);

    /**
    * @brief Sets the underlying height pointer, useful for binding to GPU memory.
    */
    void window_set_height_pointer(WINDOW*, const std::shared_ptr<float>& pointer);

    /**
    * @brief Gets a reference to the specified key in the underlying keys pointer.
    * 
    * @note If the keys pointer is updated (e.g., to GPU memory), a previously fetched reference becomes invalid.
    */
    int32_t& window_get_keypress_ref(WINDOW*, uint8_t keycode);

    /**
    * @brief overload of window_get_keypress_ref with KEYCODES enum
    */
    int32_t& window_get_keypress_ref(WINDOW*, KEYCODES keycode);

    /**
    * @brief Gets a reference to the specified button in the underlying buttons pointer.
    * 
    * @note If the buttons pointer is updated (e.g., to GPU memory), a previously fetched reference becomes invalid.
    */
    int32_t& window_get_buttonpress_ref(WINDOW*, uint8_t button);

    /**
    * @brief Gets a shared pointer reference to all key values.
    */
    std::shared_ptr<int32_t>& window_get_all_keypress_ref(WINDOW*, uint8_t keycode);

    /**
    * @brief Gets a shared pointer reference to all button values.
    */
    std::shared_ptr<int32_t>& window_get_all_buttonpress_ref(WINDOW*, uint8_t button);

    /**
    * @brief Gets a shared pointer reference to window width.
    */
    std::shared_ptr<float>& window_get_width_ref(WINDOW*);

    /**
    * @brief Gets a shared pointer reference to window height.
    */
    std::shared_ptr<float>& window_get_height_ref(WINDOW*);

    /**
    * @brief Adds a drawn buffer group.
    * 
    * Registers a DrawListManager along with a compatible buffer group so draw commands can be computed dynamically based on buffer data.
    */
    void window_add_drawn_buffer_group(WINDOW*, IDrawListManager* mgr, BufferGroup* buffer_group);

    /**
    * @brief Removes a drawn buffer group.
    */
    void window_remove_drawn_buffer_group(WINDOW*, IDrawListManager* mgr, BufferGroup* buffer_group);

    /**
    * @brief Returns the EventInterface for a given WINDOW
    */
    EventInterface* window_get_event_interface(WINDOW*);

    /**
    * @brief Adds a destroy event callback function
    */
    void window_register_free_callback(WINDOW*, const std::function<void()>&);

    /**
    * @brief sets Window mouse capture (for mouse capture outside window)
    */
    void window_set_capture(WINDOW* window_ptr, bool should_capture);
}