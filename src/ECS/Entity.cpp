#include <dz/ECS/Entity.hpp>
using namespace dz::ecs;

Entity::EntityTransformReflectable::EntityTransformReflectable(const std::function<Entity*()>& get_entity_function):
    get_entity_function(get_entity_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Transform")
{}

int Entity::EntityTransformReflectable::GetID() {
    return uid;
}

std::string& Entity::EntityTransformReflectable::GetName() {
    return name;
}

void* Entity::EntityTransformReflectable::GetVoidPropertyByIndex(int prop_index) {
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

void Entity::EntityTransformReflectable::NotifyChange(int prop_index) {}