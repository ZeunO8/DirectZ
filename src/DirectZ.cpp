#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
namespace dz
{
    DirectRegistry*& get_direct_registry()
    {
        return dr_ptr;
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
        ImGuiLayer::Shutdown(dr);
        while (!dr.layoutQueue.empty()) {
            auto layout = dr.layoutQueue.front();
            dr.layoutQueue.pop();
            vkDestroyDescriptorSetLayout(dr.device, layout, 0);
        }
#ifdef __ANDROID__
        AConfiguration_delete(dr.android_config);
#endif
        dr.buffer_groups.clear();
        dr.uid_shader_map.clear();
        if (dr.device)
        {
            vkDestroyCommandPool(dr.device, dr.commandPool, 0);
            vkDestroyRenderPass(dr.device, dr.surfaceRenderPass, 0);
            vkDestroyDevice(dr.device, 0);
        }
        auto direct_registry = get_direct_registry();
        delete direct_registry;
    }
    std::vector<WINDOW*>::iterator dr_get_windows_begin() {
        return dr.window_ptrs.begin();
    }
    std::vector<WINDOW*>::iterator dr_get_windows_end() {
        return dr.window_ptrs.end();
    }
    std::vector<WindowReflectableGroup*>::iterator dr_get_window_reflectable_entries_begin() {
        return dr.window_reflectable_entries.begin();
    }
    std::vector<WindowReflectableGroup*>::iterator dr_get_window_reflectable_entries_end() {
        return dr.window_reflectable_entries.end();
    }
}

#include "env.cpp"
#include "path.cpp"

#include "FileHandle.cpp"
#include "AssetPack.cpp"
#include "Renderer.cpp"
#include "Window.cpp"
#include "Image.cpp"
#include "Framebuffer.cpp"
#include "Shader.cpp"
#include "BufferGroup.cpp"
#include "EventInterface.cpp"
#include "D7Stream.cpp"
#include "ImGuiLayer.cpp"
#include "Displays.cpp"

#include "ECS/Entity.cpp"
#include "ECS/Mesh.cpp"
#include "ECS/SubMesh.cpp"
#include "ECS/Material.cpp"
#include "ECS/Camera.cpp"
#include "ECS/Light.cpp"

#include "State.cpp"
#include "Reflectable.cpp"
#include "ImagePack.cpp"

#include "Loaders/STB_Image_Loader.cpp"

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