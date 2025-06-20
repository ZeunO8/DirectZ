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
    auto cursor = window->cursor.get();
    switch (action)
    {
    case AMOTION_EVENT_ACTION_DOWN:
        window->buttons.get()[pointer_index] = 1;
        cursor[0] = x;
        cursor[1] = y;
        break;
    case AMOTION_EVENT_ACTION_UP:
        window->buttons.get()[pointer_index] = 0;
        break;
    case AMOTION_EVENT_ACTION_MOVE:
        cursor[0] = x;
        cursor[1] = y;
        break;
    case AMOTION_EVENT_ACTION_CANCEL:
        window->buttons.get()[pointer_index] = 0;
        break;
    case AMOTION_EVENT_ACTION_OUTSIDE:
        break;
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        break;
    case AMOTION_EVENT_ACTION_POINTER_UP:
        break;
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
        break;
    case AMOTION_EVENT_ACTION_SCROLL:
        break;
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
        break;
    case AMOTION_EVENT_ACTION_HOVER_EXIT:
        break;
    case AMOTION_EVENT_ACTION_BUTTON_PRESS:
        break;
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
#endif