#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
namespace dz
{
    DirectRegistry*& get_direct_registry()
    {
        return dr_ptr;
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
    
    void free_direct_registry(DirectRegistry* free_dr_ptr)
    {
        if (!free_dr_ptr)
            return;
        ImGuiLayer::Shutdown(*free_dr_ptr);
        while (!free_dr_ptr->layoutQueue.empty()) {
            auto layout = free_dr_ptr->layoutQueue.front();
            free_dr_ptr->layoutQueue.pop();
            vkDestroyDescriptorSetLayout(free_dr_ptr->device, layout, 0);
        }
#ifdef __ANDROID__
        AConfiguration_delete(free_dr_ptr->android_config);
#endif
        free_dr_ptr->buffer_groups.clear();
        free_dr_ptr->uid_shader_map.clear();
        if (free_dr_ptr->device)
        {
            vkDestroyCommandPool(free_dr_ptr->device, free_dr_ptr->commandPool, 0);
            vkDestroyRenderPass(free_dr_ptr->device, free_dr_ptr->surfaceRenderPass, 0);
            vkDestroyDevice(free_dr_ptr->device, 0);
        }
        delete free_dr_ptr;
    }
    std::vector<WINDOW*>::iterator dr_get_windows_begin() {
        return dr_ptr->window_ptrs.begin();
    }
    std::vector<WINDOW*>::iterator dr_get_windows_end() {
        return dr_ptr->window_ptrs.end();
    }
    std::vector<WindowReflectableGroup*>::iterator dr_get_window_reflectable_entries_begin() {
        return dr_ptr->window_reflectable_entries.begin();
    }
    std::vector<WindowReflectableGroup*>::iterator dr_get_window_reflectable_entries_end() {
        return dr_ptr->window_reflectable_entries.end();
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

#include "ECS/Scene.cpp"
#include "ECS/Entity.cpp"
#include "ECS/Mesh.cpp"
#include "ECS/SubMesh.cpp"
#include "ECS/Material.cpp"
#include "ECS/HDRI.cpp"
#include "ECS/Camera.cpp"
#include "ECS/Light.cpp"

#include "State.cpp"
#include "Reflectable.cpp"
#include "ImagePack.cpp"

#include "Loaders/STB_Image_Loader.cpp"
#include "Loaders/Assimp_Loader.cpp"

#include "SharedLibrary.cpp"

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