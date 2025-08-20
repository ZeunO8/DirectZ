#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
namespace dz
{
    DirectRegistry* get_direct_registry() {
        return dr_ptr;
    }
    uint8_t get_direct_registry_window_type() {
        return dr_ptr->windowType;
    }
    _GUID_ get_direct_registry_guid() {
        return dr_ptr->guid;
    }
    void load_direct_registry_guid(const std::string& guid) {
        if (!dr_shm.Open("DZ_DirectRegistry" + guid)) {
            throw std::runtime_error("Unable to open shared DirectRegistry!");
        }
        dr_ptr = dr_shm.ptr;
    }
    uint8_t get_window_type_platform();
    void direct_registry_ensure_instance(DirectRegistry* direct_registry);
    bool init_direct_registry(SharedMemoryPtr<DirectRegistry>& dr_shm)
    {
        auto guid = GlobalGUID::GetNew();
        if (!dr_shm.Create("DZ_DirectRegistry" + guid.to_string())) {
            throw std::runtime_error("Unable to create shared DirectRegistry!");
        }
        dr_shm.ptr->windowType = get_window_type_platform();
#ifdef __ANDROID__
        dr_shm.ptr->android_config = AConfiguration_new();
#endif
        dr_shm.ptr->guid = guid;
        return true;
    }
    
    void free_direct_registry()
    {
        if (!dr_ptr)
            return;
        while (!dr_ptr->layoutQueue.empty()) {
            auto layout = dr_ptr->layoutQueue.front();
            dr_ptr->layoutQueue.pop();
            vkDestroyDescriptorSetLayout(dr_ptr->device, layout, 0);
        }
#ifdef __ANDROID__
        AConfiguration_delete(dr_ptr->android_config);
#endif
        dr_ptr->buffer_groups.clear();
        dr_ptr->uid_shader_map.clear();
        if (dr_ptr->device)
        {
            if (dr_ptr->computeCommandBuffer)
    		    vkFreeCommandBuffers(dr_ptr->device, dr_ptr->commandPool, 1, &dr_ptr->computeCommandBuffer);
            if (dr_ptr->copyCommandBuffer)
    		    vkFreeCommandBuffers(dr_ptr->device, dr_ptr->commandPool, 1, &dr_ptr->copyCommandBuffer);
            if (dr_ptr->transitionCommandBuffer)
    		    vkFreeCommandBuffers(dr_ptr->device, dr_ptr->commandPool, 1, &dr_ptr->transitionCommandBuffer);

            vkDestroyCommandPool(dr_ptr->device, dr_ptr->commandPool, 0);
            vkDestroyRenderPass(dr_ptr->device, dr_ptr->surfaceRenderPass, 0);
            vkDestroyDevice(dr_ptr->device, 0);
        }

        dr_shm.Destroy();
        dr_ptr = nullptr;
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

#include "GlobalUID.cpp"
#include "GlobalGUID.cpp"

#include "runtime.cpp"

#include "Compiler/Clang.cpp"

#include "CMake.cpp"

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