#include <dz/ECS/Scene.hpp>
#include <dz/GlobalUID.hpp>
#include <cassert>

dz::ecs::Scene::SceneTransformReflectable::SceneTransformReflectable(const dz::function<Scene*()>& get_scene_function):
    get_scene_function(get_scene_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Transform")
{}

int dz::ecs::Scene::SceneTransformReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Scene::SceneTransformReflectable::GetName() {
    return name;
}

void* dz::ecs::Scene::SceneTransformReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto scene_ptr = get_scene_function();
    assert(scene_ptr);
    auto& scene = *scene_ptr;
    switch (prop_index) {
    case 0: return &scene.position;
    case 1: return &scene.rotation;
    case 2: return &scene.scale;
    default: return nullptr;
    }
}

void dz::ecs::Scene::SceneTransformReflectable::NotifyChange(int prop_index) {
    auto scene_ptr = get_scene_function();
    if (!scene_ptr)
        return;
    auto& scene = *scene_ptr;
    switch (prop_index) {
    default:
        scene.transform_dirty = 1;
        break;
    }
}