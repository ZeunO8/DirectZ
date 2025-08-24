#include <dz/ECS/Light.hpp>
#include <dz/GlobalUID.hpp>

dz::ecs::Light::LightMetaReflectable::LightMetaReflectable(
    const dz::function<Light*()>& get_light_function
):
    get_light_function(get_light_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Light Meta")
{}

int dz::ecs::Light::LightMetaReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Light::LightMetaReflectable::GetName() {
    return name;
}

void* dz::ecs::Light::LightMetaReflectable::GetVoidPropertyByIndex(int prop_index) {
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

void dz::ecs::Light::LightMetaReflectable::NotifyChange(int prop_index) {
    auto light_ptr = get_light_function();
    if (!light_ptr)
        return;
    auto& light = *light_ptr;
    switch (prop_index) {
    default:
        // LightInit(light);
        break;
    }
}