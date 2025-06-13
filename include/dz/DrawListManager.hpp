#pragma once
#include "DrawList.hpp"
#include <functional>
#include "Shader.hpp"
namespace dz
{
    struct Shader;
    struct IDrawListManager
    {
        ~IDrawListManager() = default;
        virtual ShaderDrawList genDrawList(BufferGroup* buffer_group) = 0;
    };
    template<typename DrawT>
    struct DrawListManager : IDrawListManager
    {
        using DrawTToVertexCountFunction = std::function<std::pair<Shader*, uint32_t>(BufferGroup*, DrawT&)>;
    private:
        DrawTToVertexCountFunction fn_determineDrawTVertexCount;
        std::string draw_key;
    public:
        DrawListManager(const std::string& draw_key, const DrawTToVertexCountFunction& fn_determineDrawTVertexCount):
            fn_determineDrawTVertexCount(fn_determineDrawTVertexCount),
            draw_key(draw_key)
        {}
        ShaderDrawList genDrawList(BufferGroup* buffer_group) override
        {
            const size_t draw_elements = buffer_group_get_buffer_element_count(buffer_group, draw_key);

            ShaderDrawList shaderDrawList;

            // Track running instance offset per shader
            std::unordered_map<Shader*, uint32_t> shader_instance_counter;

            for (size_t i = 0; i < draw_elements;)
            {
                auto element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, i);
                auto& element = element_view.template as_struct<DrawT>();
                auto [shader, vertexCount] = fn_determineDrawTVertexCount(buffer_group, element);

                // Count how many contiguous elements share this shader and vertexCount
                uint32_t run_start = static_cast<uint32_t>(i);
                uint32_t instance_count = 1;
                size_t j = i + 1;

                while (j < draw_elements)
                {
                    auto next_element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, j);
                    auto& next_element = next_element_view.template as_struct<DrawT>();
                    auto [next_shader, next_vertexCount] = fn_determineDrawTVertexCount(buffer_group, next_element);

                    if (next_shader != shader || next_vertexCount != vertexCount)
                    {
                        break;
                    }

                    instance_count += 1;
                    j += 1;
                }

                // Add draw command to that shader's draw list
                DrawIndirectCommand cmd;
                cmd.vertexCount = vertexCount;
                cmd.instanceCount = instance_count;
                cmd.firstVertex = 0;
                cmd.firstInstance = run_start;

                shaderDrawList[shader].push_back(cmd);

                i = j;
            }

            return shaderDrawList;
        }
    };
}