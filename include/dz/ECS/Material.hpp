#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
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

        inline static constexpr size_t PID = 6;
        inline static float Priority = 2.5f;
        inline static constexpr bool RequiresBuffer = true;
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
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
vec4 GetMaterialBaseColor(in SubMesh submesh) {
    if (Materials.data[submesh.material_index].albedo_atlas_pack.x == -1.0)
        return Materials.data[submesh.material_index].albedo_color;
    return vec4(1.0, 0.0, 1.0, 1.0);
}
vec4 SampleAtlas(in vec2 inUV2, in vec2 image_size, in vec2 packed_rect, in sampler2D atlas) {
    vec2 resolution = vec2(textureSize(atlas, 0));
    vec2 offset_uv = packed_rect / resolution;
    vec2 scale_uv = image_size / resolution;
    vec2 packed_uv = offset_uv + inUV2 * scale_uv;
    return texture(atlas, packed_uv);
}
void EnsureMaterialNormal(in SubMesh submesh, in vec2 inUV2, inout vec3 current_normal) {
    vec2 image_size = Materials.data[submesh.material_index].normal_atlas_pack.xy;
    if (image_size.x == -1.0)
        return;
    vec2 packed_rect = Materials.data[submesh.material_index].normal_atlas_pack.zw;
    current_normal = SampleAtlas(inUV2, image_size, packed_rect, NormalAtlas).xyz;
    current_normal = normalize(current_normal * 2.0 - 1.0);
}
)" },
            { ShaderModuleType::Fragment, R"(
vec4 SampleAtlas(in vec2 image_size, in vec2 packed_rect, in sampler2D atlas) {
    vec2 resolution = vec2(textureSize(atlas, 0));
    vec2 offset_uv = packed_rect / resolution;
    vec2 scale_uv = image_size / resolution;
    vec2 packed_uv = offset_uv + inUV2 * scale_uv;
    return texture(atlas, packed_uv);
}
void EnsureMaterialFragColor(in SubMesh submesh, inout vec4 current_color) {
    vec2 image_size = Materials.data[submesh.material_index].albedo_atlas_pack.xy;
    if (image_size.x == -1.0)
        return;
    vec2 packed_rect = Materials.data[submesh.material_index].albedo_atlas_pack.zw;
    current_color = SampleAtlas(image_size, packed_rect, AlbedoAtlas);
}
void EnsureMaterialNormal(in SubMesh submesh, inout vec3 current_normal) {
    vec2 image_size = Materials.data[submesh.material_index].normal_atlas_pack.xy;
    if (image_size.x == -1.0)
        return;
    vec2 packed_rect = Materials.data[submesh.material_index].normal_atlas_pack.zw;
    vec3 tangentNormal = SampleAtlas(image_size, packed_rect, NormalAtlas).xyz;
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);
    mat3 TBN = mat3(inTangent, inBitangent, inNormal);
    current_normal = normalize(TBN * tangentNormal);
}
void EnsureMaterialMetalnessRoughness(in SubMesh submesh, inout float metalness, inout float roughness) {
    vec2 m_r_image_size = Materials.data[submesh.material_index].metalness_roughness_atlas_pack.xy;
    if (m_r_image_size.x != -1.0) {
        vec2 m_r_packed_rect = Materials.data[submesh.material_index].metalness_roughness_atlas_pack.zw;
        vec4 m_r_vec = SampleAtlas(m_r_image_size, m_r_packed_rect, MetalnessRoughnessAtlas);
        metalness = m_r_vec.r;
        roughness = m_r_vec.g;
        return;
    }
    vec2 m_image_size = Materials.data[submesh.material_index].metalness_atlas_pack.xy;
    if (m_image_size.x != -1.0) {
        vec2 m_packed_rect = Materials.data[submesh.material_index].metalness_atlas_pack.zw;
        metalness = SampleAtlas(m_image_size, m_packed_rect, MetalnessAtlas).r;
    }
    vec2 r_image_size = Materials.data[submesh.material_index].roughness_atlas_pack.xy;
    if (r_image_size.x != -1.0) {
        vec2 r_packed_rect = Materials.data[submesh.material_index].roughness_atlas_pack.zw;
        roughness = SampleAtlas(r_image_size, r_packed_rect, RoughnessAtlas).r;
    }
}
)" }
        };

        inline static std::unordered_map<ShaderModuleType, std::vector<std::string>> GLSLLayouts = {
            // { ShaderModuleType::Vertex, {
            //     "layout(location = @OUT@) out int outIsTexture;"
            // } },
            // { ShaderModuleType::Fragment, {
            //     "layout(location = @IN@) in flat int inIsTexture;"
            // } }
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    final_color = GetMaterialBaseColor(submesh);
    // EnsureMaterialNormal(submesh, mesh_uv2, mesh_normal);
)", ShaderModuleType::Vertex},
            {0.5f, R"(
    EnsureMaterialFragColor(submesh, current_color);
    EnsureMaterialNormal(submesh, current_normal);
    float metalness = 0.0;
    float roughness = 0.0;
    EnsureMaterialMetalnessRoughness(submesh, metalness, roughness);
)", ShaderModuleType::Fragment}
        };
        
        struct MaterialReflectable : ::Reflectable {

        private:
            std::function<Material*()> get_material_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"Albedo Color", {0, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "Albedo Color"}
            };
            inline static std::vector<std::string> prop_names = {
                "Albedo Color"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(color_vec<float, 4>)
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

        struct MaterialReflectableGroup : ReflectableGroup {
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