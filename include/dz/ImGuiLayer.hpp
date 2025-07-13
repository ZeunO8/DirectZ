#pragma once
#include "Window.hpp"
#include "DirectRegistry.hpp"
#include <map>
#include <queue>
namespace dz {
    struct ImGuiLayer {
        friend WINDOW;
        using ImmediateDrawFunction = std::function<void(ImGuiLayer&)>;
        using ImmediateDrawPair = std::pair<std::string, ImmediateDrawFunction>;
    private:
        inline static bool initialized = false;
        inline static bool vulkan_initialized = false;
        inline static VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
        inline static std::queue<VkDescriptorSetLayout> layout_queue;
        std::unordered_map<size_t, float> id_priority_map;
        std::map<float, std::map<size_t, ImmediateDrawPair>> priority_immediate_draw_fn_map;
        ImDrawData* GetDrawData(WINDOW&);
        ImGuiViewport* GetViewport(WINDOW*);
    public:
        static void FocusWindow(WINDOW*, bool);
        static bool Init();
        static bool VulkanInit();
        static bool Shutdown(DirectRegistry&);
        void Render(WINDOW& window);
        size_t AddImmediateDrawFunction(float priority, const std::string& key, const ImmediateDrawFunction& fn);
        bool RemoveImmediateDrawFunction(size_t id);
        std::pair<VkDescriptorSetLayout, VkDescriptorSet> CreateDescriptorSet(Image* image);
    };
}