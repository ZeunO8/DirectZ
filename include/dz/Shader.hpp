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

namespace dz
{
    enum class ShaderModuleType
    {
        Vertex = 1,   /**< Vertex shader module. */
        Fragment = 2, /**< Fragment shader module. */
        Compute = 3   /**< Compute shader module. */
    };

    struct Renderer;
    struct Shader;

    /**
     * @brief Creates a new Shader object.
     * 
     * @return A pointer to the newly created Shader.
     */
    Shader* shader_create();

    /**
     * @brief Adds a GLSL source module to the shader.
     * 
     * @param shader Pointer to the Shader.
     * @param module_type The type of shader module.
     * @param glsl_source GLSL source code string.
     */
    void shader_add_module(Shader* shader, ShaderModuleType module_type, const std::string& glsl_source);

    /**
     * @brief Adds a GLSL source module loaded from a file.
     * 
     * @param shader Pointer to the Shader.
     * @param file_path Path to the file containing GLSL source code.
     */
    void shader_add_module_from_file(Shader* shader, const std::filesystem::path& file_path);

    /**
     * @brief Binds a BufferGroup to the shader.
     * 
     * @param shader Pointer to the Shader.
     * @param buffer_group Pointer to the BufferGroup.
     */
    void shader_add_buffer_group(Shader* shader, BufferGroup* buffer_group);

    /**
     * @brief Unbinds a BufferGroup from the shader.
     * 
     * @param shader Pointer to the Shader.
     * @param buffer_group Pointer to the BufferGroup.
     */
    void shader_remove_buffer_group(Shader* shader, BufferGroup* buffer_group);

    /**
     * @brief Dispatches the compute shader with the given thread group layout.
     * 
     * @param shader Pointer to the Shader.
     * @param dispatch_layout Layout dimensions as vec<int32_t, 3>.
     */
    void shader_dispatch(Shader* shader, vec<int32_t, 3> dispatch_layout);

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
    void shader_set_define(Shader* shader, const std::string& key, const std::string& value = "");

    /**
     * @brief Binds the shader to the current render pipeline.
     * 
     * @param shader Pointer to the Shader.
     */
    void shader_bind(Shader* shader);
} // namespace dz