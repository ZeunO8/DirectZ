#include <DirectZ.hpp>
#include <dz/Shader.hpp>
using namespace dz;
#include <atomic>
#include <algorithm>
#include <map>
#include <unordered_map>
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
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(MACOS)
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <windows.h>
#elif defined(__linux__)
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
#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <Metal/Metal.h>
#include <ApplicationServices/ApplicationServices.h>
#include <mach-o/dyld.h>
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
};
#if defined(MACOS)
#include "WINDOWDelegate.mm"
#endif
struct DirectRegistry;
extern "C" DirectRegistry* dz_registry_global;
namespace dz
{
    inline DirectRegistry*& get_direct_registry()
    {
        return dz_registry_global;
    }
    inline void set_direct_registry(DirectRegistry* ptr)
    {
        get_direct_registry() = ptr;
    }
    uint8_t get_window_type_platform();
    void direct_registry_ensure_instance(DirectRegistry* direct_registry);
    DirectRegistry* make_direct_registry()
    {
        auto dr = new DirectRegistry;
        dr->windowType = get_window_type_platform();
        return dr;
    }
    
    void free_direct_registry()
    {
        auto direct_registry = get_direct_registry();
        direct_registry->buffer_groups.clear();
        direct_registry->uid_shader_map.clear();
        if (direct_registry->device)
        {
            vkDestroyCommandPool(direct_registry->device, direct_registry->commandPool, 0);
            vkDestroyRenderPass(direct_registry->device, direct_registry->surfaceRenderPass, 0);
            vkDestroyDevice(direct_registry->device, 0);
        }
        delete direct_registry;
    }
    struct Renderer;
    struct WINDOW;
    Renderer* renderer_init(WINDOW* window);
    void renderer_render(Renderer* renderer);
    void renderer_free(Renderer* renderer);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);
    #include "env.cpp"
    #include "path.cpp"
    #include "FileHandle.cpp"
    #include "AssetPack.cpp"
    #include "Window.mm"
    #include "Renderer.cpp"
    #include "Image.cpp"
    #include "Shader.cpp"
    #include "BufferGroup.cpp"
}
#if defined(MACOS)
#include "WINDOWDelegateImpl.mm"
#endif
#include "runtime.cpp"