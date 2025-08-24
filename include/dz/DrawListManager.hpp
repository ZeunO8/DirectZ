/**
 * @file DrawListManager.hpp
 * @brief Defines an interface and template manager for producing shader draw lists from buffer groups.
 */
#pragma once
#include "DrawList.hpp"
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
        using Determine_DrawT_DrawTuple_Function = dz::function<DrawTuple(BufferGroup*, DrawT&)>;
        using Determine_CameraTuple_Function = dz::function<CameraTuple(BufferGroup*, int)>;
        using Determine_VisibleDraws_Function = dz::function<std::vector<int>(BufferGroup*, int camera_index)>;
    private:
        Determine_DrawT_DrawTuple_Function fn_determine_DrawT_DrawTuple;
        Determine_CameraTuple_Function fn_determine_CameraTuple;
        Determine_VisibleDraws_Function fn_get_visible_draws;
        std::string draw_key;
        std::string camera_key;
        DrawInformation drawInformation;
        bool draw_list_dirty = true;
    public:
        bool enable_global_camera_if_cameras_empty;

        /**
         * @brief Constructs a DrawListManager with a draw key and logic function.
         * 
         * @param draw_key Buffer key to iterate over.
         * @param fn_determine_DrawT_DrawTuple Function that maps a DrawT instance to shader and vertex count.
         */
        DrawListManager(
            const std::string& draw_key, const Determine_DrawT_DrawTuple_Function& fn_determine_DrawT_DrawTuple,
            const std::string& camera_key = "", const Determine_CameraTuple_Function& fn_determine_CameraTuple = {},
            const Determine_VisibleDraws_Function& fn_get_visible_draws = {},
            bool enable_global_camera_if_cameras_empty = true
        ):
            fn_determine_DrawT_DrawTuple(fn_determine_DrawT_DrawTuple),
            fn_determine_CameraTuple(fn_determine_CameraTuple),
            draw_key(draw_key),
            camera_key(camera_key),
            enable_global_camera_if_cameras_empty(enable_global_camera_if_cameras_empty)
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

                drawInformation.cameraDrawInfos.resize(camera_elements);
                auto cameraDrawInfos_data = drawInformation.cameraDrawInfos.data();

                for (size_t i = 0; i < camera_elements; ++i)
                {
                    auto [camera_index, framebuffer, camera_pre_render_fn, inactive] = fn_determine_CameraTuple(buffer_group, i);
                    cameraDrawInfos_data[i] = CameraDrawInformation{
                        .camera_index = camera_index,
                        .framebuffer = framebuffer,
                        .pre_render_fn = camera_pre_render_fn,
                        .inactive = inactive
                    };
                }
            }

            if (enable_global_camera_if_cameras_empty && drawInformation.cameraDrawInfos.empty())
            {
                CameraDrawInformation cameraDrawInfo{
                    .camera_index = -1,
                    .framebuffer = nullptr,
                    .pre_render_fn = {}
                };
                drawInformation.cameraDrawInfos.push_back(cameraDrawInfo);
            }

            auto drawTupleMatch = [](const auto& a, const auto& b) -> bool
            {
                if (std::get<0>(a) != std::get<0>(b) || std::get<1>(a) != std::get<1>(b))
                    return false;
                return true;
            };

            for (auto& cameraDrawInfo : drawInformation.cameraDrawInfos) {
                if (cameraDrawInfo.inactive)
                    continue;
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
                    auto draw_tuple = fn_determine_DrawT_DrawTuple(buffer_group, element);

                    uint32_t run_start = static_cast<uint32_t>(i);
                    uint32_t instance_count = 1;
                    size_t vj = vi + 1;
                    size_t j = 0;

                    while (vj < visible_entity_indices_size)
                    {
                        j = visible_entity_indices_data[vj];
                        auto next_element_view = buffer_group_get_buffer_element_view(buffer_group, draw_key, j);
                        auto& next_element = next_element_view.template as_struct<DrawT>();
                        auto next_draw_tuple = fn_determine_DrawT_DrawTuple(buffer_group, next_element);

                        if (!drawTupleMatch(draw_tuple, next_draw_tuple))
                            break;

                        instance_count += 1;
                        vj += 1;
                    }

                    auto& [shader, vertexCount] = draw_tuple;
                    
                    DrawIndirectCommand cmd;
                    cmd.vertexCount = vertexCount;
                    cmd.instanceCount = instance_count;
                    cmd.firstVertex = 0;
                    cmd.firstInstance = run_start;

                    cameraDrawInfo.shaderDrawList[shader].push_back(cmd);

                    vi = vj;
                }
            }

            draw_list_dirty = false;

            return drawInformation;
        }

        void MarkDirty() {
            draw_list_dirty = true;
        }
    };
}
