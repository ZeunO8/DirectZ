#pragma once
#include "Provider.hpp" 

namespace dz::ecs {
    struct Material : Provider<Material> {
        vec<float, 4> atlas_pack = {-1.0f, -1.0f, -1.0f, -1.0f};
        vec<float, 4> albedo = {1.0f, 1.0f, 1.0f, 1.0f};

        inline static constexpr size_t PID = 5;
        inline static float Priority = 2.5f;
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
    return vec4(1.0, 0.0, 1.0, 1.0);
}
)" }
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    final_color = GetMaterialBaseColor(entity);
)", ShaderModuleType::Vertex}
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
            BufferGroup* buffer_group = 0;
            std::string name;
            std::vector<Reflectable*> reflectables;
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