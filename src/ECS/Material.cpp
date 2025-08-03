#include <dz/ECS/Material.hpp>
#include <dz/GlobalUID.hpp>

dz::ecs::Material::MaterialReflectable::MaterialReflectable(const std::function<Material*()>& get_material_function):
    get_material_function(get_material_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Material")
{}

int dz::ecs::Material::MaterialReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Material::MaterialReflectable::GetName() {
    return name;
}

void* dz::ecs::Material::MaterialReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto material_ptr = get_material_function();
    assert(material_ptr);
    auto& material = *material_ptr;
    switch (prop_index) {
    case 0: return &material.albedo;
    default: return nullptr;
    }
}

void dz::ecs::Material::MaterialReflectable::NotifyChange(int prop_index) {}