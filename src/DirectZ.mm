#include <DirectZ.hpp>
#include <dz/Shader.hpp>
#include <dz/Singleton.hpp>
using namespace dz;
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
#elif defined(MACOS)
#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <Metal/Metal.h>
#include <ApplicationServices/ApplicationServices.h>
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
};
#if defined(MACOS)
#include "WINDOWDelegate.mm"
#endif
namespace dz
{
    void append_vk_icd_filename(const std::string& swiftshader_icd_path);
    uint8_t get_window_type_platform();
    void direct_registry_create_instance(DirectRegistry* direct_registry);
    std::shared_ptr<DirectRegistry> make_direct_registry()
    {
        append_vk_icd_filename((getProgramDirectoryPath() / "Windows" / "vk_swiftshader_icd.json").string());
        auto dr = std::shared_ptr<DirectRegistry>(new DirectRegistry, [](DirectRegistry* dr) {
            dr->buffer_groups.clear();
	        vkDestroyCommandPool(dr->device, dr->commandPool, 0);
	        vkDestroyRenderPass(dr->device, dr->surfaceRenderPass, 0);
	        vkDestroyDevice(dr->device, 0);
            delete dr;
        });
        auto direct_registry = dr.get();
        direct_registry->windowType = get_window_type_platform();
        direct_registry_create_instance(direct_registry);
        return dr;
    }
    std::shared_ptr<DirectRegistry> DZ_RGY = make_direct_registry();
    struct Renderer;
    struct WINDOW;
    Renderer* renderer_init(WINDOW* window);
    void renderer_render(Renderer* renderer);
    void renderer_free(Renderer* renderer);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);
    #include "Window.mm"
    #include "Renderer.cpp"
    #include "Image.cpp"
    #include "Shader.cpp"
    #include "BufferGroup.cpp"
    void set_env(const std::string& key, const std::string& value)
    {
    #if defined(_WIN32)
        std::string full = key + "=" + value;
        if (_putenv(full.c_str()) != 0)
        {
            throw std::runtime_error("Failed to set environment variable on Windows: " + key);
        }
    #elif defined(__linux__) || defined(__APPLE__)
        if (setenv(key.c_str(), value.c_str(), 1) != 0)
        {
            throw std::runtime_error("Failed to set environment variable on POSIX system: " + key);
        }
    #else
        #error "Unsupported platform for set_env"
    #endif
    }

    std::string get_env(const std::string& key)
    {
    #if defined(_WIN32)
        size_t requiredSize = 0;
        getenv_s(&requiredSize, nullptr, 0, key.c_str());
        if (requiredSize == 0)
        {
            return "";
        }
        std::string value(requiredSize, '\0');
        getenv_s(&requiredSize, &value[0], requiredSize, key.c_str());
        if (!value.empty() && value.back() == '\0')
        {
            value.pop_back();
        }
        return value;
    #elif defined(__linux__) || defined(__APPLE__)
        const char* val = std::getenv(key.c_str());
        return val ? std::string(val) : "";
    #else
        #error "Unsupported platform for get_env"
    #endif
    }

    void append_vk_icd_filename(const std::string& swiftshader_icd_path)
    {
        std::string key = "VK_ICD_FILENAMES";
        std::string current = get_env(key);
        std::string separator =
    #if defined(_WIN32)
            ";";
    #else
            ":";
    #endif

        if (!current.empty())
        {
            if (current.find(swiftshader_icd_path) == std::string::npos)
            {
                current += separator + swiftshader_icd_path;
            }
        }
        else
        {
            current = swiftshader_icd_path;
        }

        set_env(key, current);
    }
    
    std::filesystem::path getUserDirectoryPath() { return std::filesystem::path(getenv("HOME")); }
    std::filesystem::path getProgramDirectoryPath()
    {
        std::filesystem::path exePath;
    #if defined(_WIN32)
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        exePath = path;
    #elif defined(MACOS)
        char path[1024];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
            exePath = path;
    #elif defined(__linux__)
        exePath = std::filesystem::canonical("/proc/self/exe");
    #endif
        return exePath.parent_path();
    }
    std::filesystem::path getProgramDataPath() { return std::filesystem::temp_directory_path(); }
    std::filesystem::path getExecutableName() { return std::filesystem::path(getenv("_")).filename(); }
}
#if defined(MACOS)
#include "WINDOWDelegateImpl.mm"
#endif