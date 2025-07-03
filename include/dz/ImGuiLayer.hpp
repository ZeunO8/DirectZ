#pragma once
#include "Window.hpp"
#include <map>
namespace dz {
    struct ImGuiLayer {
        friend WINDOW;
        using ImmediateDrawFunction = std::function<void(ImGuiLayer&)>;
        using ImmediateDrawPair = std::pair<std::string, ImmediateDrawFunction>;
    private:
        std::unordered_map<size_t, float> id_priority_map;
        std::map<float, std::map<size_t, ImmediateDrawPair>> priority_immediate_draw_fn_map;\
    public:
        bool Init(WINDOW& window);
        void Render(WINDOW& window);
        size_t AddImmediateDrawFunction(float priority, const std::string& key, const ImmediateDrawFunction& fn);
        bool RemoveImmediateDrawFunction(size_t id);
    };
}