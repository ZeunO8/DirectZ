#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
#include "../Shader.hpp"
#include "../Image.hpp"

namespace dz::ecs {

    struct MaterialIndexReflectable {
        int material_index = 0;
    };

    struct Material : Provider<Material> {
        vec<float, 4> albedo_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> normal_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> metalness_roughness_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> roughness_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> metalness_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> shininess_atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> albedo_color = {1.0f, 1.0f, 1.0f, 1.0f};
        float metalness = 0;
        float roughness = 0;
        int padding1 = 0;
        int padding2 = 0;

        inline static constexpr size_t PID = 6;
        inline static float Priority = 2.5f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsMaterialProvider = true;
        inline static std::string ProviderName = "Material";
        inline static std::string StructName = "Material";
        inline static std::string GLSLStruct = R"(
struct Material {
    vec4 albedo_atlas_pack;
    vec4 normal_atlas_pack;
    vec4 metalness_roughness_atlas_pack;
    vec4 roughness_atlas_pack;
    vec4 metalness_atlas_pack;
    vec4 shininess_atlas_pack;
    vec4 albedo_color;
    float metalness;
    float roughness;
    int padding1;
    int padding2;
};

struct MaterialParams {
    vec3 albedo;
    vec3 normal;
    float metalness;
    float roughness;
};

MaterialParams mParams;
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
vec4 GetMaterialBaseColor(in SubMesh submesh) {
    if (Materials.data[submesh.material_index].albedo_atlas_pack.x == -1.0)
        return Materials.data[submesh.material_index].albedo_color;
    return vec4(1.0, 0.0, 1.0, 1.0);
}
)" },
            { ShaderModuleType::Fragment, R"(
vec4 SampleAtlas(in vec2 uv, in vec2 image_size, in vec2 packed_rect, in sampler2D atlas) {
    vec2 resolution = vec2(textureSize(atlas, 0));
    vec2 offset_uv = packed_rect / resolution;
    vec2 scale_uv = image_size / resolution;
    vec2 packed_uv = offset_uv + uv * scale_uv;
    return texture(atlas, packed_uv);
}
void EnsureMaterialFragColor(in vec2 uv, in SubMesh submesh, inout vec4 current_color) {
    vec2 image_size = Materials.data[submesh.material_index].albedo_atlas_pack.xy;
    if (image_size.x == -1.0)
        return;
    vec2 packed_rect = Materials.data[submesh.material_index].albedo_atlas_pack.zw;
    current_color = SampleAtlas(uv, image_size, packed_rect, AlbedoAtlas);
}
void EnsureMaterialNormal(in vec2 uv, in SubMesh submesh, inout vec3 current_normal) {
    vec2 image_size = Materials.data[submesh.material_index].normal_atlas_pack.xy;
    if (image_size.x == -1.0)
        return;
    vec2 packed_rect = Materials.data[submesh.material_index].normal_atlas_pack.zw;
    vec3 tangentNormal = SampleAtlas(uv, image_size, packed_rect, NormalAtlas).xyz;
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);
    mat3 TBN = mat3(inTangent, inBitangent, inNormal);
    current_normal = normalize(TBN * tangentNormal);
}
void EnsureMaterialMetalnessRoughness(in vec2 uv, in SubMesh submesh, inout float metalness, inout float roughness) {
    vec2 m_r_image_size = Materials.data[submesh.material_index].metalness_roughness_atlas_pack.xy;
    if (m_r_image_size.x != -1.0) {
        vec2 m_r_packed_rect = Materials.data[submesh.material_index].metalness_roughness_atlas_pack.zw;
        vec4 m_r_vec = SampleAtlas(uv, m_r_image_size, m_r_packed_rect, MetalnessRoughnessAtlas);
        metalness = m_r_vec.r;
        roughness = m_r_vec.g;
        return;
    }
    vec2 m_image_size = Materials.data[submesh.material_index].metalness_atlas_pack.xy;
    if (m_image_size.x != -1.0) {
        vec2 m_packed_rect = Materials.data[submesh.material_index].metalness_atlas_pack.zw;
        metalness = SampleAtlas(uv, m_image_size, m_packed_rect, MetalnessAtlas).r;
    }
    vec2 r_image_size = Materials.data[submesh.material_index].roughness_atlas_pack.xy;
    if (r_image_size.x != -1.0) {
        vec2 r_packed_rect = Materials.data[submesh.material_index].roughness_atlas_pack.zw;
        roughness = SampleAtlas(uv, r_image_size, r_packed_rect, RoughnessAtlas).r;
    }
}
)" }
        };

        inline static std::vector<std::string> GLSLBindings = {
            R"(
layout(binding = @BINDING@) uniform sampler2D AlbedoAtlas;
layout(binding = @BINDING@) uniform sampler2D NormalAtlas;
layout(binding = @BINDING@) uniform sampler2D RoughnessAtlas;
layout(binding = @BINDING@) uniform sampler2D MetalnessAtlas;
layout(binding = @BINDING@) uniform sampler2D MetalnessRoughnessAtlas;
layout(binding = @BINDING@) uniform sampler2D ShininessAtlas;

layout(binding = @BINDING@) uniform sampler2D brdfLUT;
)"
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    final_color = GetMaterialBaseColor(submesh);
)", ShaderModuleType::Vertex},
            {0.5f, R"(
    EnsureMaterialFragColor(inUV2, submesh, current_color);
    EnsureMaterialNormal(inUV2, submesh, current_normal);
    float metalness = Materials.data[submesh.material_index].metalness;
    float roughness = Materials.data[submesh.material_index].roughness;
    EnsureMaterialMetalnessRoughness(inUV2, submesh, metalness, roughness);
    mParams.albedo = vec3(current_color);
    mParams.normal = current_normal;
    mParams.metalness = metalness;
    mParams.roughness = roughness;
)", ShaderModuleType::Fragment}
        };

        static Shader* ensure_brdfLUT_shader(Image* brdfLUT_image);
        
        static void StaticInitialize();

        static void ShaderTweak(Shader*);
        
        struct MaterialReflectable : ::Reflectable {

        private:
            std::function<Material*()> get_material_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"Albedo Color", {0, 0}},
                {"Metalness", {1, 0}},
                {"Roughness", {2, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "Albedo Color"},
                {1, "Metalness"},
                {2, "Roughness"}
            };
            inline static std::vector<std::string> prop_names = {
                "Albedo Color",
                "Metalness",
                "Roughness"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(color_vec<float, 4>),
                &typeid(float),
                &typeid(float)
            };

        public:
            MaterialReflectable(const std::function<Material*()>& get_material_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct MaterialReflectableGroup : ::ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;

            Image* albedo_image = nullptr;
            VkDescriptorSet albedo_frame_image_ds = VK_NULL_HANDLE;

            Image* normal_image = nullptr;
            VkDescriptorSet normal_frame_image_ds = VK_NULL_HANDLE;

            Image* metalness_roughness_image = nullptr;
            VkDescriptorSet metalness_roughness_frame_image_ds = VK_NULL_HANDLE;

            Image* roughness_image = nullptr;
            VkDescriptorSet roughness_frame_image_ds = VK_NULL_HANDLE;

            Image* metalness_image = nullptr;
            VkDescriptorSet metalness_frame_image_ds = VK_NULL_HANDLE;

            Image* shininess_image = nullptr;
            VkDescriptorSet shininess_frame_image_ds = VK_NULL_HANDLE;

            MaterialReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Material")
            {}
            MaterialReflectableGroup(BufferGroup* buffer_group, Serial& serial):
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
                    reflectables.push_back(new MaterialReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "Materials");
                        return ((struct Material*)(buffer.get())) + index;
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

        using ReflectableGroup = MaterialReflectableGroup;
    };
}