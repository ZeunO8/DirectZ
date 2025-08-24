#include <dz/ECS/SubMesh.hpp>
#include <dz/GlobalUID.hpp>

dz::ecs::SubMesh::SubMeshReflectable::SubMeshReflectable(const dz::function<SubMesh*()>& get_submesh_function):
    get_submesh_function(get_submesh_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("SubMesh")
{}

int dz::ecs::SubMesh::SubMeshReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::SubMesh::SubMeshReflectable::GetName() {
    return name;
}

void* dz::ecs::SubMesh::SubMeshReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto mesh_ptr = get_submesh_function();
    assert(mesh_ptr);
    auto& mesh = *mesh_ptr;
    switch (prop_index) {
    case 0: return &mesh.material_index;
    case 1: return &mesh.material_index;
    default: return nullptr;
    }
}

void dz::ecs::SubMesh::SubMeshReflectable::NotifyChange(int prop_index) {}