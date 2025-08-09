#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
#include "../math.hpp"
#include "../Shader.hpp"
namespace dz::ecs {
    struct PhysicallyBasedLighting : Provider<PhysicallyBasedLighting> {
        inline static constexpr size_t PID = 9;
        inline static float Priority = 4.0f;
        inline static constexpr BufferHost BufferHostType = BufferHost::NoBuffer;
        inline static std::string ProviderName = "PhysicallyBasedLighting";
        inline static std::string StructName = "PhysicallyBasedLighting";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            {ShaderModuleType::Fragment, R"(
const vec3 Fdielectric = vec3(0.04);
const float PI = 3.141592;
const float Epsilon = 0.00001;

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

vec3 IBL(vec3 F0, vec3 N, vec3 V) {
    // irradiance / diffuse
    vec3 irradiance = SampleIrradiance(0, N).rgb;
    vec3 kd = (1.0 - F0) * (1.0 - mParams.metalness);
    vec3 diffuseBRDF = kd * mParams.albedo;
    vec3 diffuse = diffuseBRDF * irradiance;

    // reflection vector and radiance LOD
    vec3 R = reflect(-V, N);
    R.y = -R.y;
    float maxMip = float(textureQueryLevels(RadianceAtlas)) - 1.0;
    float lod = clamp(mParams.roughness * maxMip, 0.0, maxMip);
    vec3 radiance = SampleRadiance(0, R, lod).rgb;

    // safe-guard radiance (avoid NaN / negative)
    radiance = max(radiance, vec3(0.0));

    // --- BRDF LUT and Fresnel ---
    // clamp / bias coordinates to safe range
    // float safeNdotV = clamp(lParams.NdotV, 0.0001, 1.0); // avoid exact zero
    // float safeRoughness = clamp(mParams.roughness, 0.0, 1.0);
    vec2 brdfUV = vec2(lParams.NdotV, mParams.roughness);

    // Use explicit LOD 0 for BRDF LUT (typical LUT has no mips)
    vec2 brdf = textureLod(brdfLUT, brdfUV, 0.0).rg;

    // ensure brdf components are sane (clamp to avoid negative / huge values)
    brdf = clamp(brdf, vec2(0.0), vec2(16.0));

    vec3 F = fresnelSchlickRoughness(F0, lParams.NdotV, mParams.roughness);

    // combine
    vec3 specular = radiance * (F * brdf.x + brdf.y);

    // debug toggles — uncomment to diagnose:
    // return radiance;               // shows raw radiance
    // return vec3(brdf.x);           // visualise BRDF.x
    // return vec3(brdf.y);           // visualise BRDF.y
    // return vec3(F.x);              // visualise Fresnel (F)

    return diffuse + specular;
}

vec3 PBDL(vec3 F0, in Light light) {
    vec3 result = vec3(0.0);
    vec3 Li = normalize(-light.direction);
    vec3 Lradiance = light.color * (light.intensity / PI);
    vec3 Lh = normalize(Li + lParams.viewDirection);

    float cosLi = max(0.0, dot(lParams.normal, Li));
    float cosLh = max(0.0, dot(lParams.normal, Lh));

    vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, lParams.viewDirection)));
    float D = ndfGGX(cosLh, mParams.roughness);
    float G = gaSchlickGGX(cosLi, lParams.NdotV, mParams.roughness);

    vec3 kd = (1.0 - F) * (1.0 - mParams.metalness);

    vec3 diffuseBRDF = (kd * mParams.albedo) / (PI * 0.85);
    vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * lParams.NdotV);

    result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;

    return result;
    // vec3 result = vec3(0.0);
    
    // // Light direction vector (pointing from surface to light)
    // vec3 L = light.direction;//light.position - lParams.worldPosition;
    // // float distance = length(L);
    // vec3 Li = normalize(L);

    // // // Attenuation based on distance and range
    // // float attenuation = 1.0 / max(distance * distance, Epsilon); // Inverse square attenuation
    // // if (light.range > 0.0)
    // // {
    // //     float rangeAtten = clamp(1.0 - (distance * distance) / (light.range * light.range), 0.0, 1.0);
    // //     attenuation *= rangeAtten;
    // // }

    // // Radiance from light color and intensity, apply attenuation
    // vec3 Lradiance = light.color * (light.intensity / PI);// * attenuation;

    // // Half vector between light direction and view direction
    // vec3 Lh = normalize(Li + lParams.viewDirection);

    // float cosLi = max(0.0, dot(lParams.normal, Li));
    // float cosLh = max(0.0, dot(lParams.normal, Lh));

    // vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, lParams.viewDirection)));
    // float D = ndfGGX(cosLh, mParams.roughness);
    // float G = gaSchlickGGX(cosLi, lParams.NdotV, mParams.roughness);

    // vec3 kd = (1.0 - F) * (1.0 - mParams.metalness);

    // // Diffuse and specular BRDF terms
    // vec3 diffuseBRDF = (kd * mParams.albedo) / (PI * 0.85);
    // vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * lParams.NdotV);

    // // Final light contribution scaled by radiance and angle cosine
    // result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;

    // return result;
}
vec3 PBL(vec3 F0) {
    vec3 result = vec3(0.0);
    for (int light_index = 0; light_index < lParams.lightsSize; light_index++) {
        int lightTopIndex = -1;
        int lightTopCID = 0;
        GetTopNodeByCID(light_index, CID_Light, lightTopIndex, lightTopCID, CID_Scene);
        if (lightTopIndex != inTopNodeIndex)
            continue;
        switch (Lights.data[light_index].type) {
        case 0:
            result += PBDL(F0, Lights.data[light_index]);
            break;
        }
    }
    return result;
}
)" }
        };
        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {4.0f, R"(
	// Specular reflection vector
	vec3 Lr = 2.0 * lParams.NdotV * lParams.normal - lParams.viewDirection;
    
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, mParams.albedo, mParams.metalness);

    vec3 lightContribution = PBL(F0);
    vec3 iblContribution = IBL(F0, N, V);
    
    current_color = vec4(lightContribution + iblContribution, 1.0);
)", ShaderModuleType::Fragment}
        };

        struct PhysicallyBasedLightingReflectableGroup : ::ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            PhysicallyBasedLightingReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("PhysicallyBasedLighting")
            {}
            PhysicallyBasedLightingReflectableGroup(BufferGroup* buffer_group, Serial& serial):
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
            bool backup_virtual(Serial& serial) const override {
                serial << name;
                return true;
            }
            bool restore_virtual(Serial& serial) override {
                serial >> name;
                return true;
            }
        };

        using ReflectableGroup = PhysicallyBasedLightingReflectableGroup;
    };
}