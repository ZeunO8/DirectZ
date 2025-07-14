#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__) || defined(ANDROID)
#define RENDERER_VULKAN
#endif

#if defined(_WIN32)
#define USING_VULKAN_1_2 true
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) && !defined(__ANDROID__)
#define USING_VULKAN_1_2 true
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(MACOS)
#define USING_VULKAN_1_2 true
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(IOS)
#define USING_VULKAN_1_2 true
#define VK_USE_PLATFORM_METAL_EXT
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
