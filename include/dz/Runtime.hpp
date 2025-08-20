#pragma once

namespace dz {
    struct WINDOW;
    struct Image;
}

struct DirectRegistry;

#ifdef _WIN32
#define DZ_EXPORT extern "C" __declspec(dllexport)
#else
#define DZ_EXPORT extern "C" __attribute__((visibility("default")))
#endif

DZ_EXPORT dz::WINDOW* api_window_create(const char* title, float width, float height
#if defined(ANDROID)
, ANativeWindow* android_window, AAssetManager* android_asset_manager
#endif
    , bool headless = false, dz::Image* headless_image = nullptr
);

/**
 * @brief Initializes the runtime environment.
 * 
 * This function should be implemented by the platform-specific runtime (e.g., PC, Android, iOS).
 * It typically creates the main window, caches it for later use, and sets up runtime resources such as shaders and images.
 * 
 * @code
 * // Example implementation:
 * WINDOW* cached_window = 0;
 * DZ_EXPORT bool api_init(dz::WINDOW* window)
 * {
 *     // shader & image setup
 *     return true;
 * }
 * @endcode
 * 
 * @param window_info Structure containing window creation parameters.
 * @return int Status code indicating success or failure.
 */
DZ_EXPORT bool api_init(dz::WINDOW* window);

/**
 * @brief Polls for runtime events.
 * 
 * This function should be implemented by the runtime. It may simply wrap a call to window_poll_events.
 * 
 * @code
 * // Example implementation:
 * DZ_EXPORT bool api_poll_events()
 * {
 *     return windows_poll_events();
 * }
 * @endcode
 * 
 * @return true if events are still being processed; false if the application should exit.
 */
DZ_EXPORT bool api_poll_events();

/**
 * @brief Performs per-frame logic before rendering.
 * 
 * This function is intended for compute shader invocations and any CPU-side logic that should occur before rendering.
 */
DZ_EXPORT void api_update();

/**
 * @brief Renders the current frame.
 * 
 * This function should render the frame, typically by calling a platform-specific window render function.
 * 
 * @code
 * // Example implementation:
 * DZ_EXPORT bool api_render() {
 *     return windows_render();
 * }
 * @endcode
 */
DZ_EXPORT bool api_render();