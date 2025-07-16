#pragma once
#include "../Reflectable.hpp"

namespace dz::ecs {
    struct Component : Reflectable {
        struct ComponentData {
            int index;
            int type;
            int type_index;
            int data_size;
        };
        using DataT = ComponentData;
        int id = 0;
        int index = -1;
        IGetComponentDataVoid* i = 0;

        virtual ~Component() = default;
        
        inline static std::string ComponentGLSLStruct = R"(
    struct Component {
        int index;
        int type;
        int type_index;
        int data_size;
    };
    )";

        inline static std::string ComponentGLSLMethods = R"(
    Component GetComponentByType(in Entity entity, int type) {
        for (int i = 0; i < entity.componentsCount; i++) {
            int component_index = entity.components[i];
            if (Components.data[component_index].type == type) {
                return Components.data[component_index];
            }
        }
        Component DefaultComponent = Component(-1, -1, -1, -1);
        return DefaultComponent;
    }
    bool HasComponentWithType(in Entity entity, int type, out int t_component_index) {
        for (int i = 0; i < entity.componentsCount; i++) {
            int component_index = entity.components[i];
            if (Components.data[component_index].type == type) {
                t_component_index = Components.data[component_index].type_index;
                return true;
            }
        }
        t_component_index = -1;
        return false;
    }
    )";
    };
}