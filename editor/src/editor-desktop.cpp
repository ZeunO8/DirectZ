#include <dz/Runtime.hpp>
namespace dz {
    struct WINDOW;
};
dz::WINDOW* main_window = nullptr;
int main() {
    main_window = api_window_create("DirectZ Editor", 1280, 768);
    api_init(main_window);
    while(api_poll_events()) {
        if (api_render()) {
            break;
        }
    }
}