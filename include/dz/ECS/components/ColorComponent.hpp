#pragma once
#include "../Component.hpp"
#include "../Provider.hpp"
namespace dz::ecs {
    struct ColorComponent;
}
DEF_COMPONENT_ID(dz::ecs::ColorComponent, 2);
DEF_COMPONENT_COMPONENT_NAME(dz::ecs::ColorComponent, "Color");
DEF_COMPONENT_STRUCT_NAME(dz::ecs::ColorComponent, "ColorComponent");

namespace dz::ecs {
    struct ColorComponent : Component, Provider<ColorComponent> {
        using DataT = vec<float, 4>;
        inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
            {"r", {0, 0}},
            {"g", {1, 4}},
            {"b", {2, 8}},
            {"a", {3, 12}}
        };
        inline static std::unordered_map<int, std::string> prop_index_names = {
            {0, "r"},
            {1, "g"},
            {2, "b"},
            {3, "a"}
        };
        inline static std::vector<std::string> prop_names = {
            "r",
            "g",
            "b",
            "a"
        };
        inline static const std::vector<const std::type_info*> typeinfos = {
            &typeid(float),
            &typeid(float),
            &typeid(float),
            &typeid(float)
        };
        DEF_GET_ID;
        DEF_GET_NAME(ColorComponent);
        ReflectableTypeHint GetTypeHint() override {
            return ReflectableTypeHint::VEC4_RGBA;
        }
        DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
        DEF_GET_PROPERTY_NAMES(prop_names);
        DEF_GET_VOID_PROPERTY_BY_INDEX(prop_name_indexes, prop_index_names);
        DEF_GET_VOID_PROPERTY_BY_NAME;
        DEF_GET_PROPERTY_TYPEINFOS(typeinfos);

        inline static float Priority = 2.5f;
        inline static std::string ProviderName = "Color";
        inline static std::string StructName = "ColorComponent";
        inline static std::string GLSLStruct = "#define ColorComponent vec4\n";

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {1.5f, "    final_color = colorcomponent;\n", ShaderModuleType::Vertex}
        };
    };
}