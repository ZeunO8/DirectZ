#include <dz/ECS/Mesh.hpp>
#include <dz/GlobalUID.hpp>
#include <cassert>

dz::ecs::Mesh::MeshReflectable::MeshReflectable(const dz::function<Mesh*()>& get_mesh_function):
    get_mesh_function(get_mesh_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Mesh")
{}

int dz::ecs::Mesh::MeshReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Mesh::MeshReflectable::GetName() {
    return name;
}

void* dz::ecs::Mesh::MeshReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto mesh_ptr = get_mesh_function();
    assert(mesh_ptr);
    auto& mesh = *mesh_ptr;
    switch (prop_index) {
    case 0: return &mesh.vertex_count;
    case 1: return &mesh.position_offset;
    case 2: return &mesh.uv2_offset;
    case 3: return &mesh.normal_offset;
    default: return nullptr;
    }
}

void dz::ecs::Mesh::MeshReflectable::NotifyChange(int prop_index) {}