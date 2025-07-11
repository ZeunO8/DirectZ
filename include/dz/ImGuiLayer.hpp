#pragma once
#include "Window.hpp"
#include <map>
#include <queue>
struct DirectRegistry;
namespace dz {
    struct ImGuiLayer {
        friend WINDOW;
        using ImmediateDrawFunction = std::function<void(ImGuiLayer&)>;
        using ImmediateDrawPair = std::pair<std::string, ImmediateDrawFunction>;
    private:
        inline static bool ensured = false;
        inline static VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
        inline static std::queue<VkDescriptorSetLayout> layout_queue;
        std::unordered_map<size_t, float> id_priority_map;
        std::map<float, std::map<size_t, ImmediateDrawPair>> priority_immediate_draw_fn_map;\
    public:
        static bool Init();
        static bool Shutdown(DirectRegistry&);
        void Render(WINDOW& window);
        size_t AddImmediateDrawFunction(float priority, const std::string& key, const ImmediateDrawFunction& fn);
        bool RemoveImmediateDrawFunction(size_t id);
        std::pair<VkDescriptorSetLayout, VkDescriptorSet> CreateDescriptorSet(Image* image);
    };
}