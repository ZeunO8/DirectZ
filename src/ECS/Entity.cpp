#include <dz/ECS/Entity.hpp>
#include <dz/GlobalUID.hpp>
#include <cassert>

dz::ecs::Entity::EntityTransformReflectable::EntityTransformReflectable(const std::function<Entity*()>& get_entity_function):
    get_entity_function(get_entity_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Transform")
{}

int dz::ecs::Entity::EntityTransformReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Entity::EntityTransformReflectable::GetName() {
    return name;
}

void* dz::ecs::Entity::EntityTransformReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto entity_ptr = get_entity_function();
    assert(entity_ptr);
    auto& entity = *entity_ptr;
    switch (prop_index) {
    case 0: return &entity.position;
    case 1: return &entity.rotation;
    case 2: return &entity.scale;
    default: return nullptr;
    }
}

void dz::ecs::Entity::EntityTransformReflectable::NotifyChange(int prop_index) {
    auto entity_ptr = get_entity_function();
    if (!entity_ptr)
        return;
    auto& entity = *entity_ptr;
    switch (prop_index) {
    default:
        entity.transform_dirty = 1;
        break;
    }
}