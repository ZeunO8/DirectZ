#include <dz/ECS/Light.hpp>
using namespace dz::ecs;

LightMetaReflectable::LightMetaReflectable(
    const std::function<Light*()>& get_light_function,
    const std::function<void()>& reset_reflectables_function
):
    get_light_function(get_light_function),
    reset_reflectables_function(reset_reflectables_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Light Type")
{}

int LightMetaReflectable::GetID() {
    return uid;
}

std::string& LightMetaReflectable::GetName() {
    return name;
}

void* LightMetaReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto light_ptr = get_light_function();
    if (!light_ptr)
        return nullptr;
    auto& light = *light_ptr;
    switch (prop_index) {
    case 0: return &light.type;
    case 1: return &light.intensity;
    case 2: return &light.range;
    case 3: return &light.innerCone;
    case 4: return &light.position;
    case 5: return &light.direction;
    case 6: return &light.color;
    case 7: return &light.outerCone;
    default: return nullptr;
    }
}

void LightMetaReflectable::NotifyChange(int prop_index) {
    auto light_ptr = get_light_function();
    if (!light_ptr)
        return;
    auto& light = *light_ptr;
    switch (prop_index) {
    default:
        // LightInit(light);
        // reset_reflectables_function();
        break;
    }
}