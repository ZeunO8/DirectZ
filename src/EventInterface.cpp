#include <DirectZ.hpp>

namespace dz {
    static ImGuiKey MapToImGuiKey(KEYCODES keycode) {
        switch (keycode)
        {
            case KEYCODES::ESCAPE: return ImGuiKey_Escape;
            case KEYCODES::Delete: return ImGuiKey_Delete;
            case KEYCODES::UP: return ImGuiKey_UpArrow;
            case KEYCODES::DOWN: return ImGuiKey_DownArrow;
            case KEYCODES::LEFT: return ImGuiKey_LeftArrow;
            case KEYCODES::RIGHT: return ImGuiKey_RightArrow;
            case KEYCODES::HOME: return ImGuiKey_Home;
            case KEYCODES::END: return ImGuiKey_End;
            case KEYCODES::PGUP: return ImGuiKey_PageUp;
            case KEYCODES::PGDOWN: return ImGuiKey_PageDown;
            case KEYCODES::INSERT: return ImGuiKey_Insert;
            case KEYCODES::NUMLOCK: return ImGuiKey_NumLock;
            case KEYCODES::CAPSLOCK: return ImGuiKey_CapsLock;
            case KEYCODES::CTRL: return ImGuiKey_LeftCtrl;
            case KEYCODES::SHIFT: return ImGuiKey_LeftShift;
            case KEYCODES::ALT: return ImGuiKey_LeftAlt;
            case KEYCODES::PAUSE: return ImGuiKey_Pause;
            case KEYCODES::SUPER: return ImGuiKey_LeftSuper;

            case KEYCODES::BACKSPACE: return ImGuiKey_Backspace;
            case KEYCODES::TAB: return ImGuiKey_Tab;
            case KEYCODES::ENTER: return ImGuiKey_Enter;
            case KEYCODES::SPACE: return ImGuiKey_Space;
    
            case KEYCODES::SINGLEQUOTE:
            case KEYCODES::DOUBLEQUOTE:
                return ImGuiKey_Apostrophe;
            case KEYCODES::COMMA:
            case KEYCODES::LESSTHAN:
                return ImGuiKey_Comma;
            case KEYCODES::MINUS:
            case KEYCODES::UNDERSCORE:
                return ImGuiKey_Minus;
            case KEYCODES::PERIOD:
            case KEYCODES::GREATERTHAN:
                return ImGuiKey_Period;
            case KEYCODES::SLASH:
            case KEYCODES::QUESTIONMARK:
                return ImGuiKey_Slash;
            case KEYCODES::SEMICOLON:
            case KEYCODES::COLON:
                return ImGuiKey_Semicolon;
            case KEYCODES::EQUAL:
            case KEYCODES::PLUS:
                return ImGuiKey_Equal;
            case KEYCODES::LEFTBRACKET:
            case KEYCODES::LEFTBRACE:
                return ImGuiKey_LeftBracket;
            case KEYCODES::BACKSLASH:
            case KEYCODES::VERTICALBAR:
                return ImGuiKey_Backslash;
            case KEYCODES::RIGHTBRACKET:
            case KEYCODES::RIGHTBRACE:
                return ImGuiKey_RightBracket;
            case KEYCODES::GRAVEACCENT:
            case KEYCODES::TILDE:
                return ImGuiKey_GraveAccent;

            case KEYCODES::_0:
            case KEYCODES::RIGHTPARENTHESIS:
                return ImGuiKey_0;
            case KEYCODES::_1:
            case KEYCODES::EXCLAMATION:
                return ImGuiKey_1;
            case KEYCODES::_2:
            case KEYCODES::ATSIGN:
                return ImGuiKey_2;
            case KEYCODES::_3:
            case KEYCODES::HASHTAG:
                return ImGuiKey_3;
            case KEYCODES::_4:
            case KEYCODES::DOLLAR:
                return ImGuiKey_4;
            case KEYCODES::_5:
            case KEYCODES::PERCENTSIGN:
                return ImGuiKey_5;
            case KEYCODES::_6:
            case KEYCODES::CARET:
                return ImGuiKey_6;
            case KEYCODES::_7:
            case KEYCODES::AMPERSAND:
                return ImGuiKey_7;
            case KEYCODES::_8:
            case KEYCODES::ASTERISK:
                return ImGuiKey_8;
            case KEYCODES::_9:
            case KEYCODES::LEFTPARENTHESIS:
                return ImGuiKey_9;

            case KEYCODES::A:
            case KEYCODES::a:
                return ImGuiKey_A;
            case KEYCODES::B:
            case KEYCODES::b:
                return ImGuiKey_B;
            case KEYCODES::C:
            case KEYCODES::c:
                return ImGuiKey_C;
            case KEYCODES::D:
            case KEYCODES::d:
                return ImGuiKey_D;
            case KEYCODES::E:
            case KEYCODES::e:
                return ImGuiKey_E;
            case KEYCODES::F:
            case KEYCODES::f:
                return ImGuiKey_F;
            case KEYCODES::G:
            case KEYCODES::g:
                return ImGuiKey_G;
            case KEYCODES::H:
            case KEYCODES::h:
                return ImGuiKey_H;
            case KEYCODES::I:
            case KEYCODES::i:
                return ImGuiKey_I;
            case KEYCODES::J:
            case KEYCODES::j:
                return ImGuiKey_J;
            case KEYCODES::K:
            case KEYCODES::k:
                return ImGuiKey_K;
            case KEYCODES::L:
            case KEYCODES::l:
                return ImGuiKey_L;
            case KEYCODES::M:
            case KEYCODES::m:
                return ImGuiKey_M;
            case KEYCODES::N:
            case KEYCODES::n:
                return ImGuiKey_N;
            case KEYCODES::O:
            case KEYCODES::o:
                return ImGuiKey_O;
            case KEYCODES::P:
            case KEYCODES::p:
                return ImGuiKey_P;
            case KEYCODES::Q:
            case KEYCODES::q:
                return ImGuiKey_Q;
            case KEYCODES::R:
            case KEYCODES::r:
                return ImGuiKey_R;
            case KEYCODES::S:
            case KEYCODES::s:
                return ImGuiKey_S;
            case KEYCODES::T:
            case KEYCODES::t:
                return ImGuiKey_T;
            case KEYCODES::U:
            case KEYCODES::u:
                return ImGuiKey_U;
            case KEYCODES::V:
            case KEYCODES::v:
                return ImGuiKey_V;
            case KEYCODES::W:
            case KEYCODES::w:
                return ImGuiKey_W;
            case KEYCODES::X:
            case KEYCODES::x:
                return ImGuiKey_X;
            case KEYCODES::Y:
            case KEYCODES::y:
                return ImGuiKey_Y;
            case KEYCODES::Z:
            case KEYCODES::z:
                return ImGuiKey_Z;

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

    static void ImGui_ImplDZ_UpdateKeyModifiers(ImGuiIO& io, WINDOW* window)
    {
        auto& mod = *window->mod;
        io.AddKeyEvent(ImGuiMod_Ctrl, (mod & 1) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (mod & 2) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (mod & 4) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (mod & 8) != 0);
    }

    void EventInterface::cursor_move(float x, float y) {
        auto cursor = window->cursor.get();
        cursor[0] = x;
        cursor[1] = y;

        ImGuiIO& io = ImGui::GetIO();
		ImGuiLayer::FocusWindow(window, true);
        
        io.AddMousePosEvent(window->x + x, window->y + y);
        ImGui_ImplDZ_UpdateKeyModifiers(io, window);

        if (
            ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right)
        ) {
            window_set_capture(window, true);
        }
        else {
            window_set_capture(window, false);
        }
    }

    void EventInterface::key_press(KEYCODES key, int pressed) {
        window->keys.get()[(uint8_t)key] = pressed;

        auto ctrl = ((*window->mod) & 1) != 0;

        ImGuiIO& io = ImGui::GetIO();
		ImGuiLayer::FocusWindow(window, true);
        io.AddKeyEvent(MapToImGuiKey(key), pressed);
        if (pressed && !ctrl && std::isprint((uint8_t)key)) {
            io.AddInputCharacter((uint32_t)key);
        }
        ImGui_ImplDZ_UpdateKeyModifiers(io, window);
    }
    void EventInterface::cursor_press(int button, int pressed) {
        window->buttons.get()[button] = pressed;

        ImGuiIO& io = ImGui::GetIO();
		ImGuiLayer::FocusWindow(window, true);
        io.AddMouseButtonEvent(button, pressed);
        ImGui_ImplDZ_UpdateKeyModifiers(io, window);
    }
#endif
}