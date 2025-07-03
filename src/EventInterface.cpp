#include <DirectZ.hpp>

namespace dz {
    static ImGuiKey MapToImGuiKey(int my_keycode)
    {
        switch (my_keycode)
        {
            case KEYCODE_ESCAPE: return ImGuiKey_Escape;
            case KEYCODE_DELETE: return ImGuiKey_Delete;
            case KEYCODE_UP: return ImGuiKey_UpArrow;
            case KEYCODE_DOWN: return ImGuiKey_DownArrow;
            case KEYCODE_LEFT: return ImGuiKey_LeftArrow;
            case KEYCODE_RIGHT: return ImGuiKey_RightArrow;
            case KEYCODE_HOME: return ImGuiKey_Home;
            case KEYCODE_END: return ImGuiKey_End;
            case KEYCODE_PGUP: return ImGuiKey_PageUp;
            case KEYCODE_PGDOWN: return ImGuiKey_PageDown;
            case KEYCODE_INSERT: return ImGuiKey_Insert;
            case KEYCODE_NUMLOCK: return ImGuiKey_NumLock;
            case KEYCODE_CAPSLOCK: return ImGuiKey_CapsLock;
            case KEYCODE_CTRL: return ImGuiKey_LeftCtrl;
            case KEYCODE_SHIFT: return ImGuiKey_LeftShift;
            case KEYCODE_ALT: return ImGuiKey_LeftAlt;
            case KEYCODE_PAUSE: return ImGuiKey_Pause;
            case KEYCODE_SUPER: return ImGuiKey_LeftSuper;

            case 8: return ImGuiKey_Backspace;
            case 9: return ImGuiKey_Tab;
            case 10: return ImGuiKey_Enter;
            case 32: return ImGuiKey_Space;
            case 39: return ImGuiKey_Apostrophe;
            case 44: return ImGuiKey_Comma;
            case 45: return ImGuiKey_Minus;
            case 46: return ImGuiKey_Period;
            case 47: return ImGuiKey_Slash;
            case 59: return ImGuiKey_Semicolon;
            case 61: return ImGuiKey_Equal;
            case 91: return ImGuiKey_LeftBracket;
            case 92: return ImGuiKey_Backslash;
            case 93: return ImGuiKey_RightBracket;
            case 96: return ImGuiKey_GraveAccent;

            case 48: return ImGuiKey_0;
            case 49: return ImGuiKey_1;
            case 50: return ImGuiKey_2;
            case 51: return ImGuiKey_3;
            case 52: return ImGuiKey_4;
            case 53: return ImGuiKey_5;
            case 54: return ImGuiKey_6;
            case 55: return ImGuiKey_7;
            case 56: return ImGuiKey_8;
            case 57: return ImGuiKey_9;

            case 65: return ImGuiKey_A;
            case 66: return ImGuiKey_B;
            case 67: return ImGuiKey_C;
            case 68: return ImGuiKey_D;
            case 69: return ImGuiKey_E;
            case 70: return ImGuiKey_F;
            case 71: return ImGuiKey_G;
            case 72: return ImGuiKey_H;
            case 73: return ImGuiKey_I;
            case 74: return ImGuiKey_J;
            case 75: return ImGuiKey_K;
            case 76: return ImGuiKey_L;
            case 77: return ImGuiKey_M;
            case 78: return ImGuiKey_N;
            case 79: return ImGuiKey_O;
            case 80: return ImGuiKey_P;
            case 81: return ImGuiKey_Q;
            case 82: return ImGuiKey_R;
            case 83: return ImGuiKey_S;
            case 84: return ImGuiKey_T;
            case 85: return ImGuiKey_U;
            case 86: return ImGuiKey_V;
            case 87: return ImGuiKey_W;
            case 88: return ImGuiKey_X;
            case 89: return ImGuiKey_Y;
            case 90: return ImGuiKey_Z;

            default:
                return ImGuiKey_None;
        }
    }

    EventInterface::EventInterface(WINDOW* window):
        window(window)
    {}
#ifdef __ANDROID__
    void EventInterface::touch_event(int action, int pointer_index, int pointer_id, float x, float y, float pressure, float size)
    {
        if (pointer_index >= 8)
        {
            LOGW("pointer_index >= 8; skipping");
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        auto cursor = window->cursor.get();

        switch (action)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                window->buttons.get()[pointer_index] = 1;
                io.MouseDown[pointer_index] = true;
                cursor[0] = x;
                cursor[1] = y;
                io.MousePos = ImVec2(x, y);
                break;

            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
            case AMOTION_EVENT_ACTION_CANCEL:
                window->buttons.get()[pointer_index] = 0;
                io.MouseDown[pointer_index] = false;
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                cursor[0] = x;
                cursor[1] = y;
                io.MousePos = ImVec2(x, y);
                break;

            case AMOTION_EVENT_ACTION_OUTSIDE:
            case AMOTION_EVENT_ACTION_HOVER_MOVE:
            case AMOTION_EVENT_ACTION_SCROLL:
            case AMOTION_EVENT_ACTION_HOVER_ENTER:
            case AMOTION_EVENT_ACTION_HOVER_EXIT:
            case AMOTION_EVENT_ACTION_BUTTON_PRESS:
            case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
                break;
        }
    }
    void EventInterface::destroy_surface()
    {
        window->renderer->destroy_surface();
    }
    void EventInterface::recreate_window(ANativeWindow* android_window, float width, float height)
    {
        window->recreate_android(android_window, width, height);
    }
#elif defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    void EventInterface::cursor_move(float x, float y) {
        auto cursor = window->cursor.get();
        cursor[0] = x;
        cursor[1] = y;

        ImGuiIO& io = ImGui::GetIO();
        
        io.AddMousePosEvent(x, y);
    }

    void EventInterface::key_press(int key, int pressed) {
        window->keys.get()[key] = pressed;

        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(MapToImGuiKey(key), pressed);
    }
    void EventInterface::cursor_press(int button, int pressed) {
        window->buttons.get()[button] = pressed;

        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseButtonEvent(button, pressed);
    }
#endif
}