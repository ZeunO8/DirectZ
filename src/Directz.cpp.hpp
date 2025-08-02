#pragma once
#include <DirectZ.hpp>
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
#include <queue>
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
#include "WindowImpl.hpp"
#include "RendererImpl.hpp"
namespace dz {
    /**
    * @brief Creates a Window given a Serial interface
    */
    WINDOW* window_create_from_serial(Serial& serial);

    DirectRegistry* make_direct_registry();
    void free_direct_registry();

    void set_env(const std::string& key, const std::string& value);
    std::string get_env(const std::string& key);
    void append_vk_icd_filename(const std::string& swiftshader_icd_path);

    std::filesystem::path getUserDirectoryPath();
    std::filesystem::path getProgramDirectoryPath();
    std::filesystem::path getProgramDataPath();
    std::filesystem::path getExecutableName();
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
    std::queue<VkDescriptorSetLayout> layoutQueue;
#ifdef _WIN32
    HWND hwnd_root;
#endif
#ifdef __ANDROID__
    AAssetManager* android_asset_manager = 0;
    AConfiguration* android_config = 0;
#endif
};
namespace dz
{
    struct Renderer;
    struct WINDOW;
    Renderer* renderer_init(WINDOW* window);
    void renderer_render(Renderer* renderer);
    void renderer_free(Renderer* renderer);
    void create_surface(Renderer* renderer);
    bool create_swap_chain(Renderer* renderer);
    void create_image_views(Renderer* renderer);
    void create_framebuffers(Renderer* renderer);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);
    struct ShaderBuffer;
    void buffer_group_make_gpu_buffer(const std::string& name, ShaderBuffer& buffer);
    std::vector<WINDOW*>::iterator dr_get_windows_begin();
    std::vector<WINDOW*>::iterator dr_get_windows_end();
    bool vk_check(const char* fn, VkResult result);
    void vk_log(const char* fn, VkResult result);
    bool recreate_swap_chain(Renderer* renderer);

	struct QueueFamilyIndices
	{
		int32_t graphicsAndComputeFamily = -1;
		int32_t presentFamily = -1;
		bool isComplete() { return graphicsAndComputeFamily > -1 && presentFamily > -1; };
	};
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
    
	VkSampleCountFlagBits get_max_usable_sample_count(DirectRegistry* direct_registry, Renderer* renderer);
	void direct_registry_ensure_physical_device(DirectRegistry* direct_registry, Renderer* renderer);
	uint32_t rate_device_suitability(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
	bool is_device_suitable(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
	QueueFamilyIndices find_queue_families(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
	void direct_registry_ensure_logical_device(DirectRegistry* direct_registry, Renderer* renderer);
	SwapChainSupportDetails query_swap_chain_support(Renderer* renderer, VkPhysicalDevice device);
	VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR choose_swap_present_mode(Renderer* renderer, const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D choose_swap_extent(Renderer* renderer, VkSurfaceCapabilitiesKHR capabilities);
	void ensure_command_pool(Renderer* renderer);
	void ensure_command_buffers(Renderer* renderer);
	void ensure_render_pass(Renderer* renderer);
	void create_sync_objects(Renderer* renderer);
	void pre_begin_render_pass(Renderer* renderer);
	void begin_render_pass(Renderer* renderer);
	void post_render_pass(Renderer* renderer);
	bool swap_buffers(Renderer* renderer);
	void renderer_draw_commands(Renderer* renderer, Shader* shader, const std::vector<DrawIndirectCommand>& commands);
	void renderer_destroy(Renderer* renderer);
	void destroy_swap_chain(Renderer* renderer);
	void createBuffer(Renderer* renderer,
		VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkBuffer& buffer, VkDeviceMemory& bufferMemory);
}
extern "C" DirectRegistry* dr_ptr;
extern "C" DirectRegistry& dr;