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
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <windows.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <X11/extensions/Xfixes.h>
#include <xkbcommon/xkbcommon.h>
#include <dlfcn.h>
#elif defined(MACOS) || defined(IOS)
#include <dlfcn.h>
#elif defined(__ANDROID__)
#include <dlfcn.h>
#endif
#undef min
#undef max
namespace dz {
	#include "WindowImpl.hpp"
	#include "RendererImpl.hpp"
    /**
    * @brief Creates a Window given a Serial interface
    */
    WINDOW* window_create_from_serial(Serial& serial);
}
static std::unordered_map<ShaderModuleType, shaderc_shader_kind> stageEShaderc = {
	{ShaderModuleType::Vertex, shaderc_vertex_shader},
	{ShaderModuleType::Fragment, shaderc_fragment_shader},
	{ShaderModuleType::Compute, shaderc_compute_shader}};
static std::unordered_map<ShaderModuleType, VkShaderStageFlagBits> stageFlags = {
	{ShaderModuleType::Vertex, VK_SHADER_STAGE_VERTEX_BIT},
	{ShaderModuleType::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
	{ShaderModuleType::Compute, VK_SHADER_STAGE_COMPUTE_BIT}};

struct StateHolder {
    struct Restorer {
        Restorable* restorable_ptr = nullptr;
        bool owned_by_state_holder = false;
        int cid = 0;
    };
    inline static std::unordered_map<int, std::function<Restorable*(Serial&)>> c_id_fn_map = { 
        { CID_WINDOW, [](Serial& serial) -> Restorable* {
            return window_create_from_serial(serial);
        } }
    };
    std::filesystem::path path;
    std::istream* istream_ptr = nullptr;
    bool use_istream = false;
    std::ostream* ostream_ptr = nullptr;
    bool use_ostream = false;
    std::vector<Restorer> restorables;
    std::map<int, std::function<bool(Serial&)>> static_restores = {
        { GlobalUID::SID, GlobalUID::RestoreFunction }
    };
    std::map<int, std::function<bool(Serial&)>> static_backups = {
        { GlobalUID::SID, GlobalUID::BackupFunction }
    };
    bool loaded = false;
};
struct DirectRegistry
{
    StateHolder stateHolder;
    uint8_t windowType;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceFormatKHR firstSurfaceFormat = {};
    VkRenderPass surfaceRenderPass = VK_NULL_HANDLE;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkQueue presentQueue;
    int32_t graphicsAndComputeFamily = -1;
    int32_t presentFamily = -1;
    Renderer* currentRenderer = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer* commandBuffer = 0;
    VkCommandBuffer computeCommandBuffer = VK_NULL_HANDLE;
    VkSampleCountFlagBits maxMSAASamples = VK_SAMPLE_COUNT_1_BIT;
    std::vector<WINDOW*> window_ptrs;
    std::vector<WindowReflectableGroup*> window_reflectable_entries;
    std::map<size_t, std::shared_ptr<Shader>> uid_shader_map;
    std::unordered_map<std::string, std::shared_ptr<BufferGroup>> buffer_groups;
    bool swiftshader_fallback = false;
    std::atomic<uint32_t> window_count = 0;
	ImGuiLayer imguiLayer;
    std::queue<VkDescriptorSetLayout> layout_queue;
#ifdef _WIN32
    HWND hwnd_root;
#endif
#ifdef __ANDROID__
    AAssetManager* android_asset_manager = 0;
    AConfiguration* android_config = 0;
#endif
};
struct DirectRegistry;
namespace dz
{
    bool vk_check(const char* fn, VkResult result);
    void vk_log(const char* fn, VkResult result);
    bool recreate_swap_chain(Renderer* renderer);
}