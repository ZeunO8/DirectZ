#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
#include "../Shader.hpp"
namespace dz::ecs {
    struct GammaCorrection : Provider<GammaCorrection> {
        inline static constexpr size_t PID = 12;
        inline static float Priority = 1000.0f;
        inline static constexpr BufferHost BufferHostType = BufferHost::NoBuffer;
        inline static std::string ProviderName = "GammaCorrection";
        inline static std::string StructName = "GammaCorrection";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            {ShaderModuleType::Fragment, R"(
vec3 LinearToSRGBAccurate(in vec3 linearColor)
{
    vec3 srgbLow  = linearColor * 12.92;
    vec3 srgbHigh = pow(linearColor, vec3(1.0 / 2.4)) * 1.055 - 0.055;
    bvec3 cutoff = lessThanEqual(linearColor, vec3(0.0031308));
    return mix(srgbHigh, srgbLow, cutoff);
}
)" }
        };
        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {1000.0f, R"(
    current_color = vec4(LinearToSRGBAccurate(vec3(current_color)), 1.0);
)", ShaderModuleType::Fragment}
        };

        struct GammaCorrectionReflectableGroup : ::ReflectableGroup {
            std::string name;
            GammaCorrectionReflectableGroup(BufferGroup* buffer_group):
                name("GammaCorrection")
            {}
            GammaCorrectionReflectableGroup(BufferGroup* buffer_group, Serial& serial)
            {
                restore(serial);
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Generic;
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

        using ReflectableGroup = GammaCorrectionReflectableGroup;
    };
}