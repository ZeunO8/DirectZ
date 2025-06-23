#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
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
#ifdef __ANDROID__
        dr->android_config = AConfiguration_new();
#endif
        return dr;
    }
    
    void free_direct_registry()
    {
        auto direct_registry = get_direct_registry();
#ifdef __ANDROID__
        AConfiguration_delete(direct_registry->android_config);
#endif
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
    void create_surface(Renderer* renderer);
    bool create_swap_chain(Renderer* renderer);
    void create_image_views(Renderer* renderer);
    void create_framebuffers(Renderer* renderer);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);
    #include "env.cpp"
    #include "path.cpp"
    #include "FileHandle.cpp"
    #include "AssetPack.cpp"
    #include "Window.cpp"
    #include "Renderer.cpp"
    #include "Image.cpp"
    #include "Shader.cpp"
    #include "BufferGroup.cpp"
    #include "EventInterface.cpp"
}
#include "runtime.cpp"
template<typename T>
mat<T, 4, 4> window_mvp_T(WINDOW* window, const mat<T, 4, 4>& mvp)
{
    mat<T, 4, 4> pre_rotate_mat(1.0);
    vec<T, 3> rotation_axis(0, 0, 1);
    switch (window->renderer->currentTransform)
    {
    case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
        pre_rotate_mat.rotate(radians<T>(90.0), rotation_axis);
        break;
    case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
        pre_rotate_mat.rotate(radians<T>(270.0), rotation_axis);
        break;
    case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
        pre_rotate_mat.rotate(radians<T>(180.0), rotation_axis);
        break;
    default: break;
    }
    return pre_rotate_mat * mvp;
}
mat<float, 4, 4> window_mvp(WINDOW* window, const mat<float, 4, 4>& mvp)
{
    return window_mvp_T<float>(window, mvp);
}
mat<double, 4, 4> window_mvp(WINDOW* window, const mat<double, 4, 4>& mvp)
{
    return window_mvp_T<double>(window, mvp);
}