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

        inline static constexpr size_t PID = 11;
        inline static float Priority = 2.6f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsHDRIProvider = true;
        inline static std::string ProviderName = "HDRI";
        inline static std::string StructName = "HDRI";
        inline static std::string GLSLStruct = R"(
struct HDRI {
    vec4 hdri_atlas_pack;
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
)" }
        };

        inline static std::vector<std::string> GLSLBindings = {
                R"(
layout(binding = @BINDING@) uniform sampler2D HDRIAtlas;
)"
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
)", ShaderModuleType::Vertex},
            {0.5f, R"(
    vec4 hdri_sample = SampleHDRI(0, inLocalPosition);
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
    };
}