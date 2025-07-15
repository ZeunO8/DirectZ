/**
 * @file DrawListManager.hpp
 * @brief Defines an interface and template manager for producing shader draw lists from buffer groups.
 */
#pragma once
#include "DrawList.hpp"
#include <functional>
#include <tuple>
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
        virtual DrawInformation& ensureDrawInformation(BufferGroup* buffer_group) = 0;
    };

    /**
     * @brief Template draw list manager for producing draw commands from a struct buffer.
     * 
     * @tparam DrawT Struct type used for draw data.
     */
    template<typename DrawT>
    struct DrawListManager : IDrawListManager
    {
        using Determine_DrawT_DrawTuples_Function = std::function<DrawTuples(BufferGroup*, DrawT&)>;
        using Determine_CameraTuple_Function = std::function<CameraTuple(BufferGroup*, int)>;
    private:
        Determine_DrawT_DrawTuples_Function fn_determine_DrawT_DrawTuples;
        Determine_CameraTuple_Function fn_determine_CameraTuple;
        std::string draw_key;
        std::string camera_key;
        DrawInformation drawInformation;
        bool draw_list_dirty = true;
    public:
        /**
         * @brief Constructs a DrawListManager with a draw key and logic function.
         * 
         * @param draw_key Buffer key to iterate over.
         * @param fn_determine_DrawT_DrawTuples Function that maps a DrawT instance to shader and vertex count.
         */
        DrawListManager(
            const std::string& draw_key, const Determine_DrawT_DrawTuples_Function& fn_determine_DrawT_DrawTuples,
            const std::string& camera_key = "", const Determine_CameraTuple_Function& fn_determine_CameraTuple = {}
        ):
            fn_determine_DrawT_DrawTuples(fn_determine_DrawT_DrawTuples),
            fn_determine_CameraTuple(fn_determine_CameraTuple),
            draw_key(draw_key),
            camera_key(camera_key)
        {}

        /**
         * @brief Ensures DrawInformation
         *
         * @note used internally in DirectZ
         * 
         * @param buffer_group Pointer to a BufferGroup.
         *
         * @return A reference to the ensured DrawInformation of this DrawListManager
         */
        DrawInformation& ensureDrawInformation(BufferGroup* buffer_group) override
        {
            if (!draw_list_dirty)
                return drawInformation;

            // Cameras
            if (!camera_key.empty())
            {
                const size_t camera_elements = buffer_group_get_buffer_element_count(buffer_group, camera_key);

                for (size_t i = 0; i < camera_elements; ++i)
                {
                    auto [camera_index, framebuffer, camera_pre_render_fn] = fn_determine_CameraTuple(buffer_group, i);
                    drawInformation.cameras[camera_index] = { framebuffer, camera_pre_render_fn };
                }
            }

            if (drawInformation.cameras.empty())
            {
                drawInformation.cameras[-1] = { nullptr, {} };
            }

            auto drawTuplesMatch = [](const auto& a, const auto& b) -> bool
            {
                if (a.size() != b.size())
                    return false;
                for (size_t k = 0; k < a.size(); ++k)
                {
                    if (std::get<0>(a[k]) != std::get<0>(b[k]) || std::get<1>(a[k]) != std::get<1>(b[k]))
                        return false;
                }
                return true;
            };

            const size_t draw_elements = buffer_group_get_buffer_element_count(buffer_group, draw_key);

            size_t i = 0;
            while (i < draw_elements)
            {
                auto element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, i);
                auto& element = element_view.template as_struct<DrawT>();
                auto draw_tuples = fn_determine_DrawT_DrawTuples(buffer_group, element);

                uint32_t run_start = static_cast<uint32_t>(i);
                uint32_t instance_count = 1;
                size_t j = i + 1;

                while (j < draw_elements)
                {
                    auto next_element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, j);
                    auto& next_element = next_element_view.template as_struct<DrawT>();
                    auto next_draw_tuples = fn_determine_DrawT_DrawTuples(buffer_group, next_element);

                    if (!drawTuplesMatch(draw_tuples, next_draw_tuples))
                        break;

                    instance_count += 1;
                    j += 1;
                }

                for (auto& [shader, vertexCount] : draw_tuples)
                {
                    DrawIndirectCommand cmd;
                    cmd.vertexCount = vertexCount;
                    cmd.instanceCount = instance_count;
                    cmd.firstVertex = 0;
                    cmd.firstInstance = run_start;

                    drawInformation.shaderDrawList[shader].push_back(cmd);
                }

                i = j;
            }

            draw_list_dirty = false;

            return drawInformation;
        }

        void SetDirty() {
            draw_list_dirty = true;
        }
    };
}
