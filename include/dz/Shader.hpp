/**
 * @file Shader.hpp
 * @brief Defines functions to create, configure, and bind shaders with SPIR-V and GLSL inputs.
 */
#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include "ReflectedStructView.hpp"
#include "math.hpp"
#include "BlendState.hpp"

namespace dz
{
    struct Renderer;
    enum class ShaderModuleType
    {
        Vertex = 1,   /**< Vertex shader module. */
        Fragment = 2, /**< Fragment shader module. */
        Compute = 3   /**< Compute shader module. */
    };

    enum class ShaderTopology
    {
        PointList = 0,
        LineList = 1,
        LineStrip = 2,
        TriangleList = 3,
        TriangleStrip = 4,
        TriangleFan = 5
    };

    struct Shader;
    struct Framebuffer;

    /**
     * @brief Creates a new Shader object.
     * 
     * @return A pointer to the newly created Shader.
     */
    Shader* shader_create(ShaderTopology topology = ShaderTopology::TriangleList);

    /**
    * @brief Sets the shaders RenderPass to the passed Framebuffers RenderPass
    *
    * @note must be called before shader is compiled (call this before adding modules)
    */
    void shader_set_render_pass(Shader*, Framebuffer*);

    struct AssetPack;
    /**
     * @brief Sets the include path to an asset_pack for include lookup
     */
    void shader_include_asset_pack(Shader*, AssetPack* asset_pack);

    /**
     * @brief Adds a GLSL source module to the shader.
     * 
     * @param shader Pointer to the Shader.
     * @param module_type The type of shader module.
     * @param glsl_source GLSL source code string.
     */
    void shader_add_module(Shader*, ShaderModuleType module_type, const std::string& glsl_source);

    /**
     * @brief Adds a GLSL source module loaded from a file.
     * 
     * @param shader Pointer to the Shader.
     * @param file_path Path to the file containing GLSL source code.
     */
    void shader_add_module_from_file(Shader*, const std::filesystem::path& file_path);

    struct BufferGroup;
    /**
     * @brief Binds a BufferGroup to the shader.
     * 
     * @param shader Pointer to the Shader.
     * @param buffer_group Pointer to the BufferGroup.
     */
    void shader_add_buffer_group(Shader*, BufferGroup* buffer_group);

    /**
     * @brief Unbinds a BufferGroup from the shader.
     * 
     * @param shader Pointer to the Shader.
     * @param buffer_group Pointer to the BufferGroup.
     */
    void shader_remove_buffer_group(Shader*, BufferGroup* buffer_group);

    /**
     * @brief Dispatches the compute shader with the given thread group layout.
     * 
     * @param shader Pointer to the Shader.
     * @param dispatch_layout Layout dimensions as vec<int32_t, 3>.
     */
    void shader_dispatch(Shader*, vec<int32_t, 3> dispatch_layout);

    /**
     * @brief Compiles the shader source code to SPIR-V.
     * 
     * @param shader Pointer to the Shader.
     */
    void shader_compile(Shader* shader);

    /**
     * @brief Allocates GPU resources for the shader.
     * 
     * @param shader Pointer to the Shader.
     */
    void shader_create_resources(Shader* shader);

    /**
     * @brief Updates descriptor sets associated with the shader.
     * 
     * @param shader Pointer to the Shader.
     */
    void shader_update_descriptor_sets(Shader* shader);

    /**
     * @brief Initializes the shader for usage after resource creation and compilation.
     * 
     * @param shader Pointer to the Shader.
     */
    void shader_initialize(Shader* shader);

    /**
     * @brief Sets a preprocessor definition for GLSL compilation.
     * 
     * @param shader Pointer to the Shader.
     * @param key Define macro name.
     * @param value Optional value for the macro.
     */
    void shader_set_define(Shader*, const std::string& key, const std::string& value = "");

    /**
     * @brief Binds the shader to the current render pipeline.
     * 
     * @param shader Pointer to the Shader.
     */
    void shader_bind(Shader* shader);

    /**
    * @brief Emplaces an override when constructing images from reflection data based on key and provided image
    *
    * @note Should be called before adding any modules (directly after create)
    */
    void shader_use_image(Shader*, const std::string& sampler_key, Image* image_override);

    /**
    * @brief Returns a VkDescriptorSet given a key
    */
    VkDescriptorSet shader_get_descriptor_set(Shader*, const std::string&);

    /**
    * @brief gets a push constant index given the layout(push_constant) member name
    */
    int32_t shader_get_push_constant_index(Shader*, const std::string& pc_member_name);

    /**
    * @brief Updates a push_constant by index given data and size
    */
    void shader_update_push_constant(Shader*, uint32_t pc_index, void* data, uint32_t size);

    /**
     * @brief Sets the line width for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_line_width(Shader*, float line_width);

    /**
     * @brief Enables/Disables depth testing in a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_depth_test(Shader*, bool enabled);

    /**
     * @brief Enables/Disables depth writing in a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_depth_write(Shader*, bool write_enabled);

    /**
     * @brief Enables/Disables depth clamping in a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_depth_clamp(Shader*, bool clamp_enabled);

    /**
     * @brief Sets the polygon mode for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_polygon_mode(Shader*, VkPolygonMode polygon_mode);

    /**
     * @brief Sets the compare of of a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_depth_compare_op(Shader*, VkCompareOp compare_op);

    /**
     * @brief Sets the cull mode for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_cull_mode(Shader*, VkCullModeFlags cull_mode);

    /**
     * @brief Sets the front face for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_front_face(Shader*, VkFrontFace front_face);

    /**
     * @brief Sets the BlendState for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_blend_state(Shader*, BlendState blend_state);

    /**
     * @brief Sets depth_bounds_test for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_depth_bounds_test(Shader*, bool enabled);
    /**
     * @brief Sets stencil_test for a Shader
     * 
     * @note must be called before the Shader is compiled
     */
    void shader_set_stencil_test(Shader*, bool enabled);
} // namespace dz