#include <DirectZ.hpp>

DZ_EXPORT void api_set_direct_registry(DirectRegistry* new_dr_ptr) {
    set_direct_registry(new_dr_ptr);
}

DZ_EXPORT dz::WINDOW* api_window_create(const char* title, float width, float height
#if defined(ANDROID)
, ANativeWindow* android_window, AAssetManager* android_asset_manager
#endif
    , bool headless, Image* headless_image
) {
    return window_create({
        .title = title,
        .width = width,
        .height = height,
        .borderless = true,
        .vsync = true,
#if defined(ANDROID)
        .android_window = android_window,
        .android_asset_manager = android_asset_manager,
#endif
        .headless = headless,
        .headless_image = headless_image,
    });
}

DZ_EXPORT bool api_init(dz::WINDOW* window) {
    window_set_clear_color(window, {1, 0, 0, 1});
    return true;
}

DZ_EXPORT bool api_poll_events() {
    return windows_poll_events();
}

DZ_EXPORT void api_update() {

}

DZ_EXPORT void api_render() {
    windows_render();
}