#include <dz/Shader.hpp>
using namespace dz;
#include <atomic>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <optional>
#include <array>
#include <chrono>
#include <set>
#include <dz/GlobalUID.hpp>
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
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
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <windows.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/keysymdef.h>
#include <xcb/xfixes.h>
#include <xkbcommon/xkbcommon.h>
#include <dlfcn.h>
#elif defined(MACOS)
#include <dlfcn.h>
#elif defined(__ANDROID__)
#include <dlfcn.h>
#endif
#undef min
#undef max
static std::unordered_map<ShaderModuleType, shaderc_shader_kind> stageEShaderc = {
	{ShaderModuleType::Vertex, shaderc_vertex_shader},
	{ShaderModuleType::Fragment, shaderc_fragment_shader},
	{ShaderModuleType::Compute, shaderc_compute_shader}};
static std::unordered_map<ShaderModuleType, VkShaderStageFlagBits> stageFlags = {
	{ShaderModuleType::Vertex, VK_SHADER_STAGE_VERTEX_BIT},
	{ShaderModuleType::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
	{ShaderModuleType::Compute, VK_SHADER_STAGE_COMPUTE_BIT}};
struct DirectRegistry
{
    uint8_t windowType;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceFormatKHR firstSurfaceFormat = {};
    VkRenderPass surfaceRenderPass = VK_NULL_HANDLE;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer* commandBuffer = 0;
    VkCommandBuffer computeCommandBuffer = VK_NULL_HANDLE;
    VkSampleCountFlagBits maxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
    std::map<size_t, std::shared_ptr<Shader>> uid_shader_map;
    std::unordered_map<std::string, std::shared_ptr<BufferGroup>> buffer_groups;
    bool swiftshader_fallback = false;
    std::atomic<uint32_t> window_count = 0;
#ifdef __ANDROID__
    AAssetManager* android_asset_manager = 0;
    AConfiguration* android_config = 0;
#endif
};
struct DirectRegistry;