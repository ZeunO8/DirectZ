#pragma once
#include "Provider.hpp"
#include "../Shader.hpp"
#include "../Reflectable.hpp"
#include "../BufferGroup.hpp"
#include "../Image.hpp"

namespace dz::ecs {

    void set_radiance_control_block(Shader*, void*);
    struct RadianceControlBlock {
        float roughness;
        int mip;
    };

    struct HDRIIndexReflectable {
        int hdri_index = 0;
    };

    struct HDRI : Provider<HDRI> {
        vec<float, 4> hdri_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> irradiance_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> radiance_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};

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
    vec4 radiance_atlas_pack;
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
vec4 SampleRadiance(in int hdri_index, in vec3 v, float lod) {
    vec2 image_size = HDRIs.data[hdri_index].radiance_atlas_pack.xy;
    if (image_size.x == -1.0)
        return vec4(0.0);
    vec2 packed_rect = HDRIs.data[hdri_index].radiance_atlas_pack.zw;
    return SampleAtlasLOD(SampleSphericalMap(v), image_size, packed_rect, RadianceAtlas, lod);
}
)" }
        };

        inline static std::vector<std::string> GLSLBindings = {
                R"(
layout(binding = @BINDING@) uniform sampler2D HDRIAtlas;
layout(binding = @BINDING@) uniform sampler2D IrradianceAtlas;
layout(binding = @BINDING@) uniform sampler2D RadianceAtlas;
)"
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
)", ShaderModuleType::Vertex},
            {0.5f, R"(
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

        struct HDRIReflectableGroup : ::ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;

            Image* hdri_image = nullptr;
            VkDescriptorSet hdri_frame_image_ds = VK_NULL_HANDLE;

            Image* irradiance_image = nullptr;
            VkDescriptorSet irradiance_frame_image_ds = VK_NULL_HANDLE;

            Image* radiance_image = nullptr;
            VkDescriptorSet radiance_frame_image_ds = VK_NULL_HANDLE;

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
            if (hdri_group.radiance_image)
                return;

            auto hdri_width = image_get_width(hdri_group.hdri_image);
            auto hdri_height = image_get_height(hdri_group.hdri_image);

            // Setup irradiance image
            hdri_group.irradiance_image = image_create({
                .width = hdri_width / 32,
                .height = hdri_height / 32,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            });
            // ds and atlas
            hdri_group.irradiance_frame_image_ds = image_create_descriptor_set(hdri_group.irradiance_image).second;
            ecs.irradiance_atlas_pack.addImage(hdri_group.irradiance_image);

            // generate irradiance
            auto irradianceGenerationShader = GetIrradianceGenerationShader(ecs, hdri_group);
            transition_image_layout(hdri_group.irradiance_image, VK_IMAGE_LAYOUT_GENERAL);
            shader_dispatch(
                irradianceGenerationShader,
                ceil(image_get_width(hdri_group.irradiance_image) / 8),
                ceil(image_get_height(hdri_group.irradiance_image) / 8),
                1
            );
            transition_image_layout(hdri_group.irradiance_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


            // Setup radiance image
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(hdri_width, hdri_height)))) + 1;
            hdri_group.radiance_image = image_create({
                .width = hdri_width,
                .height = hdri_height,
                .format = VK_FORMAT_R16G16B16A16_SFLOAT,
                .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .mip_levels = mipLevels
            });
            // ds and atlas
            hdri_group.radiance_frame_image_ds = image_create_descriptor_set(hdri_group.radiance_image).second;
            ecs.radiance_atlas_pack.addImage(hdri_group.radiance_image);

            static RadianceControlBlock controlBlock;
            
            // generate radiance
            auto radianceGenerationShader = GetRadianceGenerationShader(ecs, hdri_group);
            for (uint32_t mip = 0; mip < mipLevels; ++mip)
            {
                transition_image_layout(hdri_group.radiance_image, VK_IMAGE_LAYOUT_GENERAL, mip);
                uint32_t mipWidth = (std::max)(1u, hdri_width >> mip);
                uint32_t mipHeight = (std::max)(1u, hdri_height >> mip);

                controlBlock.roughness = float(mip) / float(mipLevels - 1);
                controlBlock.mip = mip;

                uint32_t groupsX = (mipWidth + 7) / 8;
                uint32_t groupsY = (mipHeight + 7) / 8;
                shader_dispatch(
                    radianceGenerationShader,
                    ceil(groupsX),
                    ceil(groupsY),
                    1,
                    set_radiance_control_block,
                    0,
                    &controlBlock
                );
                transition_image_layout(hdri_group.radiance_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mip);
            }

            //
        }

        static BufferGroup* GetIrradianceBufferGroup() {
            static BufferGroup* buffer_group = nullptr;

            if (buffer_group)
                return buffer_group;

            buffer_group = buffer_group_create("IrradianceGroup");

            buffer_group_restrict_to_keys(buffer_group, {
                "inHDRI",
                "outIrradiance"
            });

            return buffer_group;
        }

        static BufferGroup* GetRadianceBufferGroup() {
            static BufferGroup* buffer_group = nullptr;

            if (buffer_group)
                return buffer_group;

            buffer_group = buffer_group_create("RadianceGroup");

            buffer_group_restrict_to_keys(buffer_group, {
                "inHDRI",
                "outRadiance"
            });

            return buffer_group;
        }

        template<typename TECS>
        static Shader* GetIrradianceGenerationShader(TECS& ecs, HDRIReflectableGroup& hdri_group) {
            static Shader* shader = nullptr;

            if (shader) {
                shader_use_image(shader, "inHDRI", hdri_group.hdri_image);
                shader_use_image(shader, "outIrradiance", hdri_group.irradiance_image);

                shader_update_descriptor_sets(shader);
                return shader;
            }

            auto buffer_group = GetIrradianceBufferGroup();

            shader = shader_create();

            shader_add_buffer_group(shader, buffer_group);

            shader_use_image(shader, "inHDRI", hdri_group.hdri_image);
            shader_use_image(shader, "outIrradiance", hdri_group.irradiance_image);

            shader_add_module(shader, ShaderModuleType::Compute, GenerateIrradianceGenerationShader(ecs));

            shader_initialize(shader);

            shader_update_descriptor_sets(shader);

            return shader;
        }

        template<typename TECS>
        static Shader* GetRadianceGenerationShader(TECS& ecs, HDRIReflectableGroup& hdri_group) {
            static Shader* shader = nullptr;

            if (shader) {
                shader_use_image(shader, "inHDRI", hdri_group.hdri_image);
                shader_use_image(shader, "outRadiance", hdri_group.radiance_image);

                shader_update_descriptor_sets(shader);
                return shader;
            }

            auto buffer_group = GetRadianceBufferGroup();

            shader = shader_create();

            shader_add_buffer_group(shader, buffer_group);

            shader_use_image(shader, "inHDRI", hdri_group.hdri_image);
            shader_use_image(shader, "outRadiance", hdri_group.radiance_image);

            shader_add_module(shader, ShaderModuleType::Compute, GenerateRadianceGenerationShader(ecs));

            shader_initialize(shader);

            shader_update_descriptor_sets(shader);

            return shader;
        }

        template<typename TECS>
        static std::string GenerateIrradianceGenerationShader(TECS& ecs) {
            std::string shader_string;

            shader_string += R"(
#version 450 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

const float PI = 3.14159265359;

layout(binding = 0) uniform sampler2D inHDRI;
layout(binding = 1) uniform writeonly image2D outIrradiance;

// Convert UV to direction (spherical)
vec3 DirectionFromEquirect(vec2 uv)
{
    float phi = uv.x * 2.0 * PI;
    float theta = (1.0 - uv.y) * PI;
    float x = sin(theta) * cos(phi);
    float y = cos(theta);
    float z = sin(theta) * sin(phi);
    return normalize(vec3(x, y, z));
}

// Convert direction to UV (used if needed)
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    uv.y = clamp(uv.y, 0.0, 1.0 - 0.0);
    return uv;
}

// Coordinate frame from normal (Tangent-space)
void CreateTangentSpace(in vec3 N, out vec3 T, out vec3 B)
{
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// Cosine-weighted hemisphere sampling (uniform distribution)
vec3 SampleHemisphereCosine(float u1, float u2)
{
    float r = sqrt(u1);
    float theta = 2.0 * PI * u2;
    float x = r * cos(theta);
    float z = r * sin(theta);
    float y = sqrt(max(0.0, 1.0 - u1));
    return vec3(x, y, z);
}

void main()
{
    ivec2 size = imageSize(outIrradiance);
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);

    if (pix.x >= size.x || pix.y >= size.y)
        return;

    vec2 uv = (vec2(pix) + 0.5) / vec2(size); // Center of texel
    vec3 N = DirectionFromEquirect(uv);       // Surface normal

    vec3 T, B;
    CreateTangentSpace(N, T, B);

    const uint SAMPLE_COUNT = 128u;
    vec3 irradiance = vec3(0.0);
    float weight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float u1 = float(i) / float(SAMPLE_COUNT);
        float u2 = fract(sin(dot(vec2(i, i + 1), vec2(12.9898, 78.233))) * 43758.5453);
        vec3 sampleVec = SampleHemisphereCosine(u1, u2);

        // Transform to world space
        vec3 worldSample = normalize(
            sampleVec.x * T +
            sampleVec.y * N +
            sampleVec.z * B
        );

        vec2 sampleUV = SampleSphericalMap(worldSample);
        sampleUV.y = -sampleUV.y;
        vec3 sampleColor = texture(inHDRI, sampleUV).rgb;

        irradiance += sampleColor * sampleVec.y; // * cosine
        weight += sampleVec.y;
    }

    irradiance = (PI * irradiance) / max(weight, 0.0001);
    imageStore(outIrradiance, pix, vec4(irradiance, 1.0));
}
)";

            return shader_string;
        }

        template<typename TECS>
        static std::string GenerateRadianceGenerationShader(TECS& ecs) {
            std::string shader_string;

            shader_string += R"(
#version 450 core

#extension GL_EXT_nonuniform_qualifier : require

layout (local_size_x = 8, local_size_y = 8) in;

layout (binding = 0) uniform sampler2D inHDRI;
layout (binding = 1) writeonly uniform image2D outRadiance[];
layout (push_constant) uniform PushConstants {
    float roughness;
    int mip;
} push;

const float PI = 3.14159265359;

vec3 DirectionFromEquirect(vec2 uv)
{
    float phi = uv.x * 2.0 * 3.14159265359;
    float theta = (1.0 - uv.y) * 3.14159265359;
    float sinTheta = sin(theta);
    vec3 R;
    R.x = -sinTheta * cos(phi);
    R.y = -cos(theta); 
    R.z = -sinTheta * sin(phi);
    return R;
}

vec2 SampleSphericalMap(vec3 dir)
{
    // Normalize input direction
    dir = normalize(dir);
    
    // Calculate azimuthal angle (phi) from x and z
    float phi = atan(dir.z, dir.x);
    
    // Calculate polar angle (theta) from y
    float theta = acos(dir.y);
    
    // Convert to UV coordinates
    // phi from [-π, π] to [0, 1]
    // theta from [0, π] to [0, 1]
    vec2 uv;
    uv.x = (phi / (2.0 * 3.14159265359)) + 0.5;
    uv.y = theta / 3.14159265359;
    
    return uv;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH = max(dot(N, H), 0.0);
    float NoH2 = NoH * NoH;

    float denom = NoH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 1e-5);
}

vec3 ImportanceSampleGGX(float u1, float u2, float roughness, vec3 N)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * u1;
    float cosTheta = sqrt((1.0 - u2) / (1.0 + (a * a - 1.0) * u2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = cosTheta;
    H.z = sin(phi) * sinTheta;

    vec3 up = vec3(0.0, 1.0, 0.0); //abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);

    return normalize(T * H.x + N * H.y + B * H.z);
}

void main()
{
    ivec2 size = imageSize(outRadiance[push.mip]);
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    if (pix.x >= size.x || pix.y >= size.y)
        return;

    vec2 uv = (vec2(pix) + 0.5) / vec2(size);
    vec3 R = DirectionFromEquirect(uv);
    vec3 N = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float u1 = float(i) / float(SAMPLE_COUNT);
        float u2 = fract(sin(float(i) * 12.9898) * 43758.5453);

        vec3 H = ImportanceSampleGGX(u1, u2, push.roughness, N);
        vec3 L = normalize(reflect(-R, H));
        float NoL = max(dot(N, L), 0.0);

        if (NoL > 0.0)
        {
            vec2 sampleUV = SampleSphericalMap(L);
            vec3 sampleColor = texture(inHDRI, sampleUV).rgb;

            float NoH = max(dot(N, H), 0.0);
            float HoL = max(dot(H, L), 0.0);
            float D = DistributionGGX(N, H, push.roughness);
            float pdf = (D * NoH) / (4.0 * HoL + 1e-5);
            float omegaS = 1.0 / (pdf + 1e-5);

            prefilteredColor += sampleColor * NoL * omegaS;
            totalWeight += NoL * omegaS;
        }
    }

    prefilteredColor = prefilteredColor / max(totalWeight, 1e-5);
    imageStore(outRadiance[push.mip], pix, vec4(prefilteredColor, 1.0));
}
)";

            return shader_string;
        }
    };
}