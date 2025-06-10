#include <DirectZ.hpp>
#include <dz/Shader.hpp>
#include <dz/Singleton.hpp>
using namespace dz;
// for now
#include <map>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <array>
#include <chrono>
#include <set>
#include <dz/GlobalUID.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <windows.h>
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
    VkQueue presentQueue;
    VkSampleCountFlagBits maxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
    std::map<size_t, std::shared_ptr<Shader>> uid_shader_map;
    std::unordered_map<std::string, std::shared_ptr<BufferGroup>> buffer_groups;
};
namespace dz
{
    void direct_registry_create_instance(DirectRegistry* direct_registry);
    std::shared_ptr<DirectRegistry> make_direct_registry()
    {
        auto dr = std::shared_ptr<DirectRegistry>(new DirectRegistry, [](DirectRegistry* dr) {
            dr->buffer_groups.clear();
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
    struct Window;
    Renderer* renderer_init(Window* window);
    void renderer_render(Renderer* renderer);
    void renderer_free(Renderer* renderer);
    #include "Window.cpp"
    #include "Renderer.cpp"
    #include "Shader.cpp"
    #include "BufferGroup.cpp"
}