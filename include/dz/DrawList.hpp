/**
 * @file DrawList.hpp
 * @brief Defines data structures for GPU indirect draw commands and their organization per shader.
 */

#pragma once

#include <vector>
#include <unordered_map>

namespace dz
{
    struct Shader;
    struct Framebuffer;

    /**
     * @brief Represents a single indirect draw command for use with GPU draw calls.
     *
     * This structure corresponds to the layout expected by functions like glDrawArraysIndirect or vkCmdDrawIndirect.
     */
    struct DrawIndirectCommand
    {
        uint32_t vertexCount = 0;     /**< Number of vertices to draw. */
        uint32_t instanceCount = 0;   /**< Number of instances to draw. */
        uint32_t firstVertex = 0;     /**< Index of the first vertex. */
        uint32_t firstInstance = 0;   /**< Instance ID of the first instance. */
    };

    /**
     * @brief A list of draw commands.
     *
     * This represents a batch of DrawIndirectCommand structures, typically submitted to the GPU in one draw call.
     */
    using DrawList = std::vector<DrawIndirectCommand>;

    /**
     * @brief Maps each Shader pointer to its corresponding list of draw commands.
     *
     * This allows organizing draw commands by the shader used, enabling grouped rendering submissions.
     */
    using ShaderDrawList = std::unordered_map<Shader*, DrawList>;

    /**
     * @brief Maps each Framebuffer pointer to the ShaderDrawList
     */
    using FramebufferDrawList = std::unordered_map<Framebuffer*, ShaderDrawList>;

    /**
    * @brief A DrawTuple is the information required to produce a DrawList in a DrawListManager
    */
    using DrawTuple = std::tuple<Framebuffer*, Shader*, uint32_t>;
} // namespace dz
