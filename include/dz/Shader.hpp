#pragma once
#include <string>
#include <memory>
#include "ReflectedStructView.hpp"
#include "math.hpp"
namespace dz
{
    enum class ShaderModuleType
    {
        Vertex = 1,
        Fragment = 2,
        Compute = 3
    };
    struct Renderer;
    struct Shader;
    Shader* shader_create();
    void shader_add_module(
        Shader* shader,
        ShaderModuleType module_type,
        const std::string& glsl_source
    );
    void shader_add_buffer_group(Shader* shader, BufferGroup* buffer_group);
    void shader_remove_buffer_group(Shader* shader, BufferGroup* buffer_group);
    void shader_dispatch(Shader* shader, vec<int32_t, 3> dispatch_layout);
    void shader_compile(Shader* shader);
    void shader_create_resources(Shader* shader);
    void shader_update_descriptor_sets(Shader* shader);
    void shader_initialize(Shader* shader);
    void shader_set_define(Shader* shader, const std::string& key, const std::string& value = "");
    void shader_bind(Shader* shader);
}