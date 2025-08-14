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
        .vsync = false,
#if defined(ANDROID)
        .android_window = android_window,
        .android_asset_manager = android_asset_manager,
#endif
        .headless = headless,
        .headless_image = headless_image,
    });
}

SharedLibrary* current_sl = nullptr;
dz::WINDOW* project_window = nullptr;
#define TEST_WINDOW_WIDTH 640
#define TEST_WINDOW_HEIGHT 480

using APIWindowCreate = dz::WINDOW*(*)(const char*, float, float
#if defined(ANDROID)
    , ANativeWindow* android_window, AAssetManager* android_asset_manager
#endif
    , bool headless, Image* headless_image
);
using APIInit = bool(*)(dz::WINDOW*);
using APISetDirectRegistry = void(*)(DirectRegistry*);
using APIPollEvents = bool(*)();
using APIUpdate = void(*)();
using APIRender = void(*)();

APIWindowCreate sl_api_window_create = nullptr;
APIInit sl_api_init = nullptr;
APISetDirectRegistry sl_api_set_direct_registry = nullptr;
APIPollEvents sl_api_poll_events = nullptr;
APIUpdate sl_api_update = nullptr;
APIRender sl_api_render = nullptr;

void initialize_project_library(const char*);
void initialize_imgui();

DZ_EXPORT bool api_init(dz::WINDOW* window) {
    initialize_project_library("test-lib.dll");
    sl_api_set_direct_registry(get_direct_registry());

    project_window = sl_api_window_create("Test", TEST_WINDOW_WIDTH, TEST_WINDOW_HEIGHT, true, nullptr);
    sl_api_init(project_window);

    initialize_imgui();
    return true;
}

DZ_EXPORT bool api_poll_events() {
    return windows_poll_events();
}

DZ_EXPORT void api_update() {

}

DZ_EXPORT bool api_render() {
    return windows_render();
}

void initialize_project_library(const char* lib_path) {
    current_sl = sl_create(lib_path);
    sl_api_window_create = sl_get_proc_tmpl<APIWindowCreate>(current_sl, "api_window_create");
    sl_api_init = sl_get_proc_tmpl<APIInit>(current_sl, "api_init");
    sl_api_set_direct_registry = sl_get_proc_tmpl<APISetDirectRegistry>(current_sl, "api_set_direct_registry");
    sl_api_poll_events = sl_get_proc_tmpl<APIPollEvents>(current_sl, "api_poll_events");
    sl_api_update = sl_get_proc_tmpl<APIUpdate>(current_sl, "api_update");
    sl_api_render = sl_get_proc_tmpl<APIRender>(current_sl, "api_render");
}

void initialize_imgui() {
    auto& imgui = get_ImGuiLayer();
    imgui.AddImmediateDrawFunction(1.0, "AnAction", [](auto& layer) {
        if (ImGui::Button("Action Button")) {
            std::cout << "Action Button pressed" << std::endl;
        }
    });
    imgui.AddImmediateDrawFunction(2.0, "Target", [](auto& layer) {
        if (!ImGui::Begin("Target")) {
            ImGui::End();
            return;
        }
        auto panel_pos = ImGui::GetCursorScreenPos();
        auto panel_size = ImGui::GetContentRegionAvail();
        
        ImGui::Image((ImTextureID)window_get_headless_ds(project_window), panel_size, ImVec2(0, 1), ImVec2(1, 0));
        window_set_size(project_window, panel_size.x, panel_size.y);

        ImGui::End();
    });
}