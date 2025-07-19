#pragma once
#include "Provider.hpp"
#include "../math.hpp"
namespace dz::ecs {
    struct Light : Provider<Light> {
        enum LightType : uint8_t {
            Directional,
            Spot,
            Point
        };
        int type = 0;
        float intensity;          // scalar
        float range;              // for point/spot
        float innerCone;          // for spot
        vec<float, 3> position;
        int padding1 = 0;
        vec<float, 3> direction;  // normalized, for directional/spot
        int padding2 = 0;
        vec<float, 3> color;      // RGBA
        float outerCone;          // for spot
        inline static float Priority = 3.5f;
        inline static std::string ProviderName = "Light";
        inline static std::string StructName = "Light";
        inline static std::string GLSLStruct = R"(
struct Light {
    int type;
    float intensity;
    float range;
    float innerCone;
    vec3 position;
    int padding1;
    vec3 direction;
    int padding2;
    vec3 color;
    float outerCone;
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            {ShaderModuleType::Fragment, R"(
vec3 CalculateLight(in vec3 normal, vec3 frag_pos, vec3 view_dir, in Light light) {
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
            {3.5f, R"(
    vec3 frag_pos = vec3(inPosition);
    vec3 view_dir = normalize(camera.position - frag_pos);
    int lights_size = Lights.data.length();
    vec3 light_color = vec3(0.0);
    for (int light_index = 0; light_index < lights_size; light_index++) {
        light_color += CalculateLight(inNormal, frag_pos, view_dir, Lights.data[light_index]);
    }
    current_color = vec4(light_color, 1.0) * current_color;
)", ShaderModuleType::Fragment}
        };
    };
        
    struct LightMetaReflectable : Reflectable {
        
    private:
        std::function<Light*()> get_light_function;
        std::function<void()> reset_reflectables_function;
        int uid;
        std::string name;
        inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
            { "type", {0, 0}},
            { "intensity", {1, 0}},
            { "range", {2, 0}},
            { "innerCone", {3, 0}},
            { "position", {4, 0}},
            { "direction", {5, 0}},
            { "color", {6, 0}},
            { "outerCone", {7, 0}}
        };
        inline static std::unordered_map<int, std::string> prop_index_names = {
            { 0, "type" },
            { 1, "intensity"},
            { 2, "range"},
            { 3, "innerCone"},
            { 4, "position"},
            { 5, "direction"},
            { 6, "color"},
            { 7, "outerCone"}
        };
        inline static std::vector<std::string> prop_names = {
            "type",
            "intensity",
            "range",
            "innerCone",
            "position",
            "direction",
            "color",
            "outerCone"
        };
        inline static const std::vector<const std::type_info*> typeinfos = {
            &typeid(Light::LightType),
            &typeid(float),
            &typeid(float),
            &typeid(float),
            &typeid(vec<float, 3>),
            &typeid(vec<float, 3>),
            &typeid(vec<float, 3>),
            &typeid(float)
        };

    public:
        LightMetaReflectable(
            const std::function<Light*()>& get_light_function,
            const std::function<void()>& reset_reflectables_function
        );
        int GetID() override;
        std::string& GetName() override;
        DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
        DEF_GET_PROPERTY_NAMES(prop_names);
        void* GetVoidPropertyByIndex(int prop_index) override;
        DEF_GET_VOID_PROPERTY_BY_NAME;
        DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
        void NotifyChange(int prop_index) override;
    };
}