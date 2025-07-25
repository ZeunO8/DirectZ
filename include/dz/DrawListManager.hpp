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
        using Determine_VisibleDraws_Function = std::function<std::vector<int>(BufferGroup*, int camera_index)>;
    private:
        Determine_DrawT_DrawTuples_Function fn_determine_DrawT_DrawTuples;
        Determine_CameraTuple_Function fn_determine_CameraTuple;
        Determine_VisibleDraws_Function fn_get_visible_draws;
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
            const std::string& camera_key = "", const Determine_CameraTuple_Function& fn_determine_CameraTuple = {},
            const Determine_VisibleDraws_Function& fn_get_visible_draws = {}
        ):
            fn_determine_DrawT_DrawTuples(fn_determine_DrawT_DrawTuples),
            fn_determine_CameraTuple(fn_determine_CameraTuple),
            draw_key(draw_key),
            camera_key(camera_key)
        {
            if (fn_get_visible_draws) {
                this->fn_get_visible_draws = fn_get_visible_draws;
            }
            else {
                this->fn_get_visible_draws = [&](auto buffer_group, auto camera_index) {
                    auto draw_count = buffer_group_get_buffer_element_count(buffer_group, this->draw_key);
                    std::vector<int> visible(draw_count, 0);
                    auto visible_size = visible.size();
                    auto visible_data = visible.data();
                    for (size_t i = 1; i < visible_size; ++i)
                        visible_data[i] = i;
                    return visible;
                };
            }
        }

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
                    CameraDrawInformation cameraDrawInfo{
                        .camera_index = camera_index,
                        .framebuffer = framebuffer,
                        .pre_render_fn = camera_pre_render_fn
                    };
                    drawInformation.cameraDrawInfos.push_back(cameraDrawInfo);
                }
            }

            if (drawInformation.cameraDrawInfos.empty())
            {
                CameraDrawInformation cameraDrawInfo{
                    .camera_index = -1,
                    .framebuffer = nullptr,
                    .pre_render_fn = {}
                };
                drawInformation.cameraDrawInfos.push_back(cameraDrawInfo);
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

            for (auto& cameraDrawInfo : drawInformation.cameraDrawInfos) {
                auto camera_index = cameraDrawInfo.camera_index;
                auto visible_entity_indices = fn_get_visible_draws(buffer_group, camera_index);
                auto visible_entity_indices_data = visible_entity_indices.data();
                auto visible_entity_indices_size = visible_entity_indices.size();
                size_t vi = 0;
                size_t i = 0;
                for (; vi < visible_entity_indices_size;) {
                    i = visible_entity_indices_data[vi];

                    auto element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, i);
                    auto& element = element_view.template as_struct<DrawT>();
                    auto draw_tuples = fn_determine_DrawT_DrawTuples(buffer_group, element);

                    uint32_t run_start = static_cast<uint32_t>(i);
                    uint32_t instance_count = 1;
                    size_t vj = vi + 1;
                    size_t j = 0;

                    while (vj < visible_entity_indices_size)
                    {
                        j = visible_entity_indices_data[vj];
                        auto next_element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, j);
                        auto& next_element = next_element_view.template as_struct<DrawT>();
                        auto next_draw_tuples = fn_determine_DrawT_DrawTuples(buffer_group, next_element);

                        if (!drawTuplesMatch(draw_tuples, next_draw_tuples))
                            break;

                        instance_count += 1;
                        vj += 1;
                    }

                    for (auto& [shader, vertexCount] : draw_tuples)
                    {
                        DrawIndirectCommand cmd;
                        cmd.vertexCount = vertexCount;
                        cmd.instanceCount = instance_count;
                        cmd.firstVertex = 0;
                        cmd.firstInstance = run_start;

                        cameraDrawInfo.shaderDrawList[shader].push_back(cmd);
                    }

                    vi = vj;
                }
            }

            draw_list_dirty = false;

            return drawInformation;
        }

        void SetDirty() {
            draw_list_dirty = true;
        }
    };
}
