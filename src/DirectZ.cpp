#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
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
    struct ShaderBuffer;
    void buffer_group_make_gpu_buffer(const std::string& name, ShaderBuffer& buffer);
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
    #include "D7Stream.cpp"
    #include "ImGuiLayer.cpp"
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