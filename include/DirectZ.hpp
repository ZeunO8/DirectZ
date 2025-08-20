/**
 * @file DirectZ.hpp
 * @brief Includes the headers required to get started with a DirectZ instance, developers should include this one
 */
#pragma once

#include <dz/Runtime.hpp>
#include <dz/Renderer.hpp>
#include <filesystem>
#include <dz/BufferGroup.hpp>
#include <dz/size_ptr.hpp>
#include <dz/AssetPack.hpp>
#include <dz/Shader.hpp>
#include <dz/Window.hpp>
#include <dz/DrawListManager.hpp>
#include <dz/math.hpp>
#include <dz/FileHandle.hpp>
#include <dz/internal/memory_stream.hpp>
#include <dz/KeyValueStream.hpp>
#include <dz/ProgramArgs.hpp>
#include <dz/EventInterface.hpp>
#include <dz/zmalloc.hpp>
#include <dz/D7Stream.hpp>
#include <dz/ECS.hpp>
#include <dz/ImGuiLayer.hpp>
#include <dz/Reflectable.hpp>
#include <dz/Displays.hpp>
#include <dz/State.hpp>
#include <dz/TypeLoader.hpp>
#include <dz/Loaders/STB_Image_Loader.hpp>
#include <dz/Loaders/Assimp_Loader.hpp>
#include <dz/SharedLibrary.hpp>
#include <dz/SharedMemory.hpp>
#include <dz/SharedMemoryPtr.hpp>
#include <dz/Process.hpp>
#include <dz/GlobalUID.hpp>
#include <dz/GlobalGUID.hpp>

using namespace dz;

namespace dz {
    DirectRegistry* get_direct_registry();
    _GUID_ get_direct_registry_guid();
    uint8_t get_direct_registry_window_type();
    void load_direct_registry_guid(const std::string& guid);
}

inline static std::string DZ_GLSL_VERSION = "#version 450\n";

#ifdef __ANDROID__
#define LOG_TAG "DirectZ"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif
mat<float, 4, 4> window_mvp(WINDOW* window, const mat<float, 4, 4>& mvp);
mat<double, 4, 4> window_mvp(WINDOW* window, const mat<double, 4, 4>& mvp);

#ifndef USING_VULKAN_1_2
    #define UsingAttachmentDescription VkAttachmentDescription
    #define UsingAttachmentReference VkAttachmentReference
    #define UsingSubpassDescription VkSubpassDescription
    #define UsingSubpassDependency VkSubpassDependency
    #define UsingRenderPassCreateInfo VkRenderPassCreateInfo
#else
    #define UsingAttachmentDescription VkAttachmentDescription2
    #define UsingAttachmentReference VkAttachmentReference2
    #define UsingSubpassDescription VkSubpassDescription2
    #define UsingSubpassDependency VkSubpassDependency2
    #define UsingRenderPassCreateInfo VkRenderPassCreateInfo2
#endif