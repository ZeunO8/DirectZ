/**
 * @file DirectZ.hpp
 * @brief Includes the headers required to get started with a DirectZ instance, developers should include this one
 */
#pragma once

// Yes DirectZ just uses Vulkan (for v1.0 at the very least) 
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__) || defined(ANDROID)
#define RENDERER_VULKAN
#endif

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) && !defined(__ANDROID__)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(MACOS)
// #define VK_USE_PLATFORM_MACOS_MVK
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#endif

#include <filesystem>
#include <dz/BufferGroup.hpp>
#include <dz/size_ptr.hpp>
#include <dz/AssetPack.hpp>
#include <dz/Shader.hpp>
#include <dz/Window.hpp>
#include <dz/DrawListManager.hpp>
#include <dz/math.hpp>
#include <dz/FileHandle.hpp>
#include <dz/internal/memory_stream.hpp>
#include <dz/KeyValueStream.hpp>
#include <dz/ProgramArgs.hpp>
#include <dz/EventInterface.hpp>
#include <dz/zmalloc.hpp>
#include <dz/D7Stream.hpp>
#include <dz/ECS.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <dz/ImGuiLayer.hpp>

#ifdef _WIN32
#define DZ_EXPORT extern "C" __declspec(dllexport)
#else
#define DZ_EXPORT extern "C" __attribute__((visibility("default")))
#endif

using namespace dz;

inline static std::string DZ_GLSL_VERSION = "#version 450\n";

/**
 * @brief Initializes the runtime environment.
 * 
 * This function should be implemented by the platform-specific runtime (e.g., PC, Android, iOS).
 * It typically creates the main window, caches it for later use, and sets up runtime resources such as shaders and images.
 * 
 * @code
 * // Example implementation:
 * WINDOW* cached_window = 0;
 * DZ_EXPORT EventInterface* init(const WindowCreateInfo& window_info)
 * {
 *     cached_window = window_create(window_info);
 *     
 *     // shader & image setup
 *     return window_get_event_interface(cached_window);
 * }
 * @endcode
 * 
 * @param window_info Structure containing window creation parameters.
 * @return int Status code indicating success or failure.
 */
DZ_EXPORT EventInterface* init(const WindowCreateInfo& window_info);

/**
 * @brief Polls for runtime events.
 * 
 * This function should be implemented by the runtime. It may simply wrap a call to window_poll_events.
 * 
 * @code
 * // Example implementation:
 * DZ_EXPORT bool poll_events()
 * {
 *     return window_poll_events(cached_window);
 * }
 * @endcode
 * 
 * @return true if events are still being processed; false if the application should exit.
 */
DZ_EXPORT bool poll_events();

/**
 * @brief Performs per-frame logic before rendering.
 * 
 * This function is intended for compute shader invocations and any CPU-side logic that should occur before rendering.
 */
DZ_EXPORT void update();

/**
 * @brief Renders the current frame.
 * 
 * This function should render the frame, typically by calling a platform-specific window render function.
 * 
 * @code
 * // Example implementation:
 * DZ_EXPORT void render()
 * {
 *     window_render(cached_window);
 * }
 * @endcode
 */
DZ_EXPORT void render();

#ifdef __ANDROID__
#define LOG_TAG "DirectZ"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif
mat<float, 4, 4> window_mvp(WINDOW* window, const mat<float, 4, 4>& mvp);
mat<double, 4, 4> window_mvp(WINDOW* window, const mat<double, 4, 4>& mvp);