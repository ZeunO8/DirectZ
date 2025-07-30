#pragma once
#include "Provider.hpp"
#include "../Image.hpp"

namespace dz::ecs {
    struct Material : Provider<Material> {
        vec<float, 4> atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> albedo = {1.0f, 1.0f, 1.0f, 1.0f};

        inline static constexpr size_t PID = 5;
        inline static float Priority = 2.5f;
        inline static constexpr bool IsMaterialProvider = true;
        inline static std::string ProviderName = "Material";
        inline static std::string StructName = "Material";
        inline static std::string GLSLStruct = R"(
    struct Material {
        vec4 atlas_pack;
        vec4 albedo;
    };
    )";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
vec4 GetMaterialBaseColor(in Entity entity) {
    bool not_what = true;
    for (int i = 0; i < 4; i++) {
        if (Materials.data[entity.material_index].atlas_pack[i] != -1.0) {
            not_what = false;
            break;
        }
    }
    if (not_what)
        return Materials.data[entity.material_index].albedo;
    outIsTexture = 1;
    return vec4(1.0, 0.0, 1.0, 1.0);
}
)" },
            { ShaderModuleType::Fragment, R"(
void EnsureMaterialFragColor(in Entity entity, inout vec4 current_color) {
    if (inIsTexture != 1)
        return;
    vec2 uv = inUV2;
    Material material = Materials.data[entity.material_index];
    vec2 atlas_image_size = material.atlas_pack.xy;
    vec2 atlas_packed_rect = material.atlas_pack.zw;

    vec2 atlas_resolution = vec2(textureSize(Atlas, 0));
    vec2 offset_uv = atlas_packed_rect / atlas_resolution;
    vec2 scale_uv = atlas_image_size / atlas_resolution;
    vec2 packed_uv = offset_uv + uv * scale_uv;

    vec4 tex_color = texture(Atlas, packed_uv);
    current_color = tex_color;
}
)" }
        };

        inline static std::unordered_map<ShaderModuleType, std::vector<std::string>> GLSLLayouts = {
            { ShaderModuleType::Vertex, {
                "layout(location = @OUT@) out int outIsTexture;"
            } },
            { ShaderModuleType::Fragment, {
                "layout(location = @IN@) in flat int inIsTexture;"
            } }
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    final_color = GetMaterialBaseColor(entity);
)", ShaderModuleType::Vertex},
            {0.5f, R"(
    EnsureMaterialFragColor(entity, current_color);
)", ShaderModuleType::Fragment}
        };
        
        struct MaterialReflectable : Reflectable {

        private:
            std::function<Material*()> get_material_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"atlasImageSize", {0, 0}},
                {"atlasPackedRect", {1, 0}},
                {"albedo", {2, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "atlasImageSize"},
                {1, "atlasPackedRect"},
                {2, "albedo"}
            };
            inline static std::vector<std::string> prop_names = {
                "atlasImageSize",
                "atlasPackedRect",
                "albedo"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(vec<float, 2>),
                &typeid(vec<float, 2>),
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
            Image* image = nullptr;
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