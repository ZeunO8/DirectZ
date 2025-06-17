/**
 * @file DrawListManager.hpp
 * @brief Defines an interface and template manager for producing shader draw lists from buffer groups.
 */
#pragma once
#include "DrawList.hpp"
#include <functional>
#include "Shader.hpp"

namespace dz
{
    struct Shader;

    /**
     * @brief Interface for draw list manager implementations.
     */
    struct IDrawListManager
    {
        virtual ~IDrawListManager() = default;

        /**
         * @brief Generates a ShaderDrawList using the provided buffer group.
         * 
         * @param buffer_group Pointer to a BufferGroup.
         * @return A map of shaders to draw command lists.
         */
        virtual ShaderDrawList genDrawList(BufferGroup* buffer_group) = 0;
    };

    /**
     * @brief Template draw list manager for producing draw commands from a struct buffer.
     * 
     * @tparam DrawT Struct type used for draw data.
     */
    template<typename DrawT>
    struct DrawListManager : IDrawListManager
    {
        using DrawTToVertexCountFunction = std::function<std::pair<Shader*, uint32_t>(BufferGroup*, DrawT&)>;
    private:
        DrawTToVertexCountFunction fn_determineDrawTVertexCount;
        std::string draw_key;
    public:
        /**
         * @brief Constructs a DrawListManager with a draw key and logic function.
         * 
         * @param draw_key Buffer key to iterate over.
         * @param fn_determineDrawTVertexCount Function that maps a DrawT instance to shader and vertex count.
         */
        DrawListManager(const std::string& draw_key, const DrawTToVertexCountFunction& fn_determineDrawTVertexCount):
            fn_determineDrawTVertexCount(fn_determineDrawTVertexCount),
            draw_key(draw_key)
        {}

        /**
         * @brief Generates a ShaderDrawList by scanning elements in the buffer group.
         * 
         * @param buffer_group Pointer to a BufferGroup.
         * @return A map from Shader* to lists of DrawIndirectCommand.
         */
        ShaderDrawList genDrawList(BufferGroup* buffer_group) override
        {
            const size_t draw_elements = buffer_group_get_buffer_element_count(buffer_group, draw_key);

            ShaderDrawList shaderDrawList;

            std::unordered_map<Shader*, uint32_t> shader_instance_counter;

            for (size_t i = 0; i < draw_elements;)
            {
                auto element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, i);
                auto& element = element_view.template as_struct<DrawT>();
                auto [shader, vertexCount] = fn_determineDrawTVertexCount(buffer_group, element);

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
