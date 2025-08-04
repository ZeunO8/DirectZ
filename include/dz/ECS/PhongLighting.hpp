#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
#include "../math.hpp"
#include "../Shader.hpp"
namespace dz::ecs {
    struct PhongLighting : Provider<PhongLighting> {
        inline static constexpr size_t PID = 8;
        inline static float Priority = 4.0f;
        inline static constexpr bool RequiresBuffer = false;
        inline static std::string ProviderName = "PhongLighting";
        inline static std::string StructName = "PhongLighting";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            {ShaderModuleType::Fragment, R"(
vec3 CalculatePhongLighting(in vec3 normal, vec3 frag_pos, vec3 view_dir, in Light light) {
    vec3 light_dir;
    float attenuation = 1.0;

    if (light.type == 0)
    {
        light_dir = normalize(-light.direction);
    }
    else
    {
        light_dir = normalize(light.position - frag_pos);
        float dist = length(light.position - frag_pos);
        attenuation = clamp(1.0 - (dist / light.range), 0.0, 1.0);
    }

    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 4.0);//shininess);

    float spotlight_factor = 1.0;
    if (light.type == 2)
    {
        float theta = dot(light_dir, normalize(-light.direction));
        float epsilon = light.innerCone - light.outerCone;
        spotlight_factor = clamp((theta - light.outerCone) / epsilon, 0.0, 1.0);
    }

    vec3 diffuse = diff * light.color * light.intensity;
    vec3 specular = spec * light.color * light.intensity;

    return (diffuse + specular) * attenuation * spotlight_factor;
}
)" }
        };
        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {4.0f, R"(
    vec3 light_color = vec3(0.0);
    for (int light_index = 0; light_index < lights_size; light_index++) {
        light_color += CalculatePhongLighting(current_normal, frag_pos, view_dir, Lights.data[light_index]);
    }
    current_color = vec4(light_color, 1.0) * current_color;
)", ShaderModuleType::Fragment}
        };

        struct PhongLightingReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            PhongLightingReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("PhongLighting")
            {}
            PhongLightingReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Light;
            }
            std::string& GetName() override {
                return name;
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                return true;
            }
            bool restore(Serial& serial) override {
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                return true;
            }
        };

        using ReflectableGroup = PhongLightingReflectableGroup;
    };
}