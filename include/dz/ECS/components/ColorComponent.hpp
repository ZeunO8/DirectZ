#pragma once
#include "../Provider.hpp"
namespace dz::ecs {
    struct ColorComponent;
}

namespace dz::ecs {
    struct ColorComponent : Provider<ColorComponent> {
        color_vec<float, 4> color;

        inline static constexpr bool IsComponent = true;
        inline static constexpr size_t PID = 1000;
        inline static constexpr char ComponentID = 1;
        inline static float Priority = 2.5f;
        inline static std::string ProviderName = "Color";
        inline static std::string StructName = "ColorComponent";
        inline static std::string GLSLStruct = R"(
#define ColorComponent vec4
)";

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {1.5f, "    final_color = colorcomponent;\n", ShaderModuleType::Vertex}
        };
        
        struct ColorComponentReflectable : Reflectable {

        private:
            std::function<ColorComponent*()> get_color_component_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"color", {0, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "color"}
            };
            inline static std::vector<std::string> prop_names = {
                "color"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(color_vec<float, 4>)
            };

        public:
            ColorComponentReflectable(const std::function<ColorComponent*()>& get_color_component_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct ColorComponentReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = 0;
            std::string name;
            std::vector<Reflectable*> reflectables;
            ColorComponentReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("ColorComponent")
            {}
            ColorComponentReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            ~ColorComponentReflectableGroup() {
                ClearReflectables();
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
                    reflectables.push_back(new ColorComponentReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "ColorComponents");
                        return ((struct ColorComponent*)(buffer.get())) + index;
                    }));
                }
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

        using ReflectableGroup = ColorComponentReflectableGroup;
    };
}