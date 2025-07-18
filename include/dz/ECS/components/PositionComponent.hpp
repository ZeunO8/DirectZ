#pragma once
#include "../Component.hpp"
#include "../Provider.hpp"
namespace dz::ecs {
    struct PositionComponent;
}
DEF_COMPONENT_ID(dz::ecs::PositionComponent, 1);
DEF_COMPONENT_COMPONENT_NAME(dz::ecs::PositionComponent, "Position");
DEF_COMPONENT_STRUCT_NAME(dz::ecs::PositionComponent, "PositionComponent");

namespace dz::ecs {
    struct PositionComponent : Component, Provider<PositionComponent> {
        using DataT = vec<float, 4>;
        inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
            {"x", {0, 0}},
            {"y", {1, 4}},
            {"z", {2, 8}},
            {"t", {3, 12}}
        };
        inline static std::unordered_map<int, std::string> prop_index_names = {
            {0, "x"},
            {1, "y"},
            {2, "z"},
            {3, "t"}
        };
        inline static std::vector<std::string> prop_names = {
            "x",
            "y",
            "z",
            "t"
        };
        inline static const std::vector<const std::type_info*> typeinfos = {
            &typeid(float),
            &typeid(float),
            &typeid(float),
            &typeid(float)
        };
        DEF_GET_ID;
        DEF_GET_NAME(PositionComponent);
        ReflectableTypeHint GetTypeHint() override {
            return ReflectableTypeHint::VEC3;
        }
        DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
        DEF_GET_PROPERTY_NAMES(prop_names);
        DEF_GET_VOID_PROPERTY_BY_INDEX(prop_name_indexes, prop_index_names);
        DEF_GET_VOID_PROPERTY_BY_NAME;
        DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
        
        inline static float Priority = 1.5f;
        inline static std::string ProviderName = "Position";
        inline static std::string StructName = "PositionComponent";
        inline static std::string GLSLStruct = "#define PositionComponent vec4\n";
        inline static std::string GLSLMethods = "";

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {1.5f, "    final_position += vec4(positioncomponent.x, positioncomponent.y, positioncomponent.z, 0);\n", ShaderModuleType::Vertex}
        };

    };
}