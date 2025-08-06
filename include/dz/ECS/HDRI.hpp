#pragma once
#include "Provider.hpp"
#include "../Shader.hpp"
#include "../Reflectable.hpp"
#include "../BufferGroup.hpp"
#include "../Image.hpp"

namespace dz::ecs {

    struct HDRIIndexReflectable {
        int hdri_index = 0;
    };

    struct HDRI : Provider<HDRI> {
        vec<float, 4> hdri_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> irradiance_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};

        inline static constexpr size_t PID = 11;
        inline static float Priority = 2.6f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsHDRIProvider = true;
        inline static std::string ProviderName = "HDRI";
        inline static std::string StructName = "HDRI";
        inline static std::string GLSLStruct = R"(
struct HDRI {
    vec4 hdri_atlas_pack;
    vec4 irradiance_atlas_pack;
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
)" },
            { ShaderModuleType::Fragment, R"(
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
vec4 SampleHDRI(in int hdri_index, in vec3 v) {
    vec2 image_size = HDRIs.data[hdri_index].hdri_atlas_pack.xy;
    if (image_size.x == -1.0)
        return vec4(0.0);
    vec2 packed_rect = HDRIs.data[hdri_index].hdri_atlas_pack.zw;
    return SampleAtlas(SampleSphericalMap(v), image_size, packed_rect, HDRIAtlas);
}
vec4 SampleIrradiance(in int hdri_index, in vec3 v) {
    vec2 image_size = HDRIs.data[hdri_index].irradiance_atlas_pack.xy;
    if (image_size.x == -1.0)
        return vec4(0.0);
    vec2 packed_rect = HDRIs.data[hdri_index].irradiance_atlas_pack.zw;
    return SampleAtlas(SampleSphericalMap(v), image_size, packed_rect, IrradianceAtlas);
}
)" }
        };

        inline static std::vector<std::string> GLSLBindings = {
                R"(
layout(binding = @BINDING@) uniform sampler2D HDRIAtlas;
layout(binding = @BINDING@) uniform sampler2D IrradianceAtlas;
)"
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
)", ShaderModuleType::Vertex},
            {0.5f, R"(
    vec4 hdri_sample = SampleHDRI(0, inLocalPosition);
    vec4 irradiance_sample = SampleIrradiance(0, inLocalPosition);
)", ShaderModuleType::Fragment}
        };
        
        struct HDRIReflectable : ::Reflectable {

        private:
            std::function<HDRI*()> get_hdri_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
            };
            inline static std::vector<std::string> prop_names = {
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
            };

        public:
            HDRIReflectable(const std::function<HDRI*()>& get_hdri_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct HDRIReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;

            Image* hdri_image = nullptr;
            VkDescriptorSet hdri_frame_image_ds = VK_NULL_HANDLE;

            Image* irradiance_image = nullptr;
            VkDescriptorSet irradiance_frame_image_ds = VK_NULL_HANDLE;

            HDRIReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("HDRI")
            {}
            HDRIReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Generic;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            void ClearReflectables() {
                if (reflectables.empty()) {
                    return;
                }
                delete reflectables[0];
                reflectables.clear();
            }
            void UpdateChildren() override {
                if (reflectables.empty()) {
                    reflectables.push_back(new HDRIReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "HDRIs");
                        return ((struct HDRI*)(buffer.get())) + index;
                    }));
                }
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                return true;
            }
            bool restore(Serial& serial) override{
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                return true;
            }

        };

        using ReflectableGroup = HDRIReflectableGroup;

        template<typename TECS>
        void Initialize(TECS& ecs, ::ReflectableGroup& generic_group) {
            auto& hdri_group = dynamic_cast<HDRIReflectableGroup&>(generic_group);
            if (!hdri_group.hdri_image)
                return;
            if (hdri_group.irradiance_image)
                return;
            auto hdri_width = image_get_width(hdri_group.hdri_image);
            auto hdri_height = image_get_height(hdri_group.hdri_image);
            hdri_group.irradiance_image = image_create({
                .width = hdri_width,
                .height = hdri_height,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            });

            hdri_group.irradiance_frame_image_ds = image_create_descriptor_set(hdri_group.irradiance_image).second;
            ecs.irradiance_atlas_pack.addImage(hdri_group.irradiance_image);

            struct ControlBlock {
                int u_mode; 
                float u_roughness;
            };
            static ControlBlock controlBlock;
            auto irradianceGenerationShader = GetIrradianceGenerationShader(ecs);
            shader_use_image(irradianceGenerationShader, "inEnvironmentMap", hdri_group.hdri_image);
            shader_use_image(irradianceGenerationShader, "outputImage", hdri_group.irradiance_image);
            shader_update_descriptor_sets(irradianceGenerationShader);
            controlBlock.u_mode = 1;
            controlBlock.u_roughness = 0.0f;
            transition_image_layout(hdri_group.irradiance_image, VK_IMAGE_LAYOUT_GENERAL);
            shader_dispatch(irradianceGenerationShader, image_get_width(hdri_group.hdri_image) / 8, image_get_height(hdri_group.hdri_image) / 8, 1, [&]() mutable {
                shader_update_push_constant(irradianceGenerationShader, 0, (void*)&controlBlock.u_mode, sizeof(int));
                shader_update_push_constant(irradianceGenerationShader, 1, (void*)&controlBlock.u_roughness, sizeof(float));
                shader_ensure_push_constants(irradianceGenerationShader);
            });
            transition_image_layout(hdri_group.irradiance_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        static BufferGroup* GetIrradianceBufferGroup() {
            static BufferGroup* buffer_group = nullptr;

            if (buffer_group)
                return buffer_group;

            buffer_group = buffer_group_create("IrradianceGroup");

            buffer_group_restrict_to_keys(buffer_group, {
                "inEnvironmentMap",
                "outputImage"
            });

            return buffer_group;
        }

        template<typename TECS>
        static Shader* GetIrradianceGenerationShader(TECS& ecs) {
            static Shader* shader = nullptr;

            if (shader)
                return shader;

            auto buffer_group = GetIrradianceBufferGroup();

            shader = shader_create();

            shader_add_buffer_group(shader, buffer_group);

            shader_add_module(shader, ShaderModuleType::Compute, GenerateIrradianceGenerationShader(ecs));

            shader_initialize(shader);

            return shader;
        }

        template<typename TECS>
        static std::string GenerateIrradianceGenerationShader(TECS& ecs) {
            std::string shader_string;

            shader_string += R"(
#version 450 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

layout(binding = 0) uniform sampler2D inEnvironmentMap;
layout(binding = 1, rgba32f) uniform writeonly image2D outputImage;

// Control parameters
layout (push_constant) uniform ControlBlock {
    // 0 for Radiance (specular), 1 for Irradiance (diffuse)
    int u_mode;
    // Roughness value (from 0.0 to 1.0) for the radiance map generation.
    // This will correspond to the mip level.
    float u_roughness;
} pControl;

// --- Helper Functions ---

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

// Generates a sample vector for importance sampling the GGX distribution.
// This function orients the sample vector around the normal N.
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    // Calculate the angle phi based on the first random number.
    float phi = 2.0 * PI * Xi.x;
    
    // Use the second random number to calculate the cosine of the angle theta.
    // This formula is derived from the inverse of the cumulative distribution
    // function of the GGX NDF.
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // Convert from spherical to Cartesian coordinates in tangent space.
    vec3 H; // Half-vector
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // Transform the half-vector from tangent space to world space.
    // Create an orthonormal basis around the normal N.
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    // TBN matrix transforms from tangent space to world space.
    mat3 TBN = mat3(tangent, bitangent, N);
    return normalize(TBN * H);
}

// Generates a uniformly distributed sample vector on a hemisphere oriented around N.
vec3 importanceSampleHemisphere(vec2 Xi, vec3 N) {
    float phi = TWO_PI * Xi.x;
    float cosTheta = Xi.y;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    mat3 TBN = mat3(tangent, bitangent, N);
    return normalize(TBN * H);
}


// Generates a low-discrepancy sequence (Hammersley) for quasi-Monte Carlo integration.
// This provides better sample distribution than pure random numbers.
vec2 hammersley(uint i, uint N) {
    // Radical inverse with base 2
    return vec2(float(i) / float(N), float(bitfieldReverse(i)) * 2.3283064365386963e-10);
}

void main() {
    // Get the pixel coordinates for this invocation.
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(outputImage);

    // Prevent writing out of bounds.
    if (storePos.x >= size.x || storePos.y >= size.y) {
        return;
    }

    // Calculate the normalized UV coordinates for the output pixel.
    vec2 uv = (vec2(storePos) + 0.5) / vec2(size);
    
    // Convert the UV coordinates to a 3D direction vector.
    // This represents the view direction V or the normal N depending on the context.
    float theta = uv.y * PI;
    float phi = (1.0 - uv.x) * TWO_PI - PI / 2.0; // Remap x from [0,1] to [-PI/2, 3PI/2]

    vec3 N;
    N.y = cos(theta);
    N.x = cos(phi) * sin(theta);
    N.z = sin(phi) * sin(theta);
    N = normalize(N);

    // The view vector V is the same as the normal N because we are assuming
    // the viewer is at the center of the environment sphere looking outwards.
    vec3 V = N;

    if (pControl.u_mode == 0) {
        // --- Mode 0: Pre-filtered Radiance Map (for Specular IBL) ---
        
        vec3 prefilteredColor = vec3(0.0);
        float totalWeight = 0.0;
        
        const uint SAMPLE_COUNT = 1024u;
        for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
            // Generate a quasi-random sequence for sampling.
            vec2 Xi = hammersley(i, SAMPLE_COUNT);
            
            // Generate a sample direction (half-vector H) using GGX importance sampling.
            vec3 H = importanceSampleGGX(Xi, N, pControl.u_roughness);
            
            // Calculate the reflection vector L.
            vec3 L = normalize(2.0 * dot(V, H) * H - V);

            float NdotL = max(dot(N, L), 0.0);
            if (NdotL > 0.0) {
                // Sample the environment map using the reflection vector.
                vec2 envUV = SampleSphericalMap(L);
                prefilteredColor += texture(inEnvironmentMap, envUV).rgb * NdotL;
                totalWeight += NdotL;
            }
        }
        
        prefilteredColor = prefilteredColor / totalWeight;
        imageStore(outputImage, storePos, vec4(prefilteredColor, 1.0));

    } else {
        // --- Mode 1: Irradiance Map Convolution (for Diffuse IBL) ---
        
        vec3 irradiance = vec3(0.0);
        
        // We sample the hemisphere around the normal N.
        const uint SAMPLE_COUNT = 2048u;
        for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
            vec2 Xi = hammersley(i, SAMPLE_COUNT);
            vec3 sampleVec = importanceSampleHemisphere(Xi, N);
            
            // Sample the environment map.
            vec2 envUV = SampleSphericalMap(sampleVec);
            
            // Accumulate color, weighted by cos(theta) which is implicitly
            // part of the solid angle differential.
            irradiance += texture(inEnvironmentMap, envUV).rgb;
        }
        
        // The result of the Monte Carlo integration is the average of the samples
        // divided by the PDF. For uniform hemisphere sampling, the PDF is 1/(2*PI).
        // So we multiply by PI. The 1/N from averaging and 1/PDF cancel out.
        irradiance = irradiance * PI / float(SAMPLE_COUNT);
        
        imageStore(outputImage, storePos, vec4(1.0, 0.0, 1.0, 1.0));
    }
}
)";

            return shader_string;
        }
    };
}