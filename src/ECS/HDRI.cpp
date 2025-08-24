#include <dz/ECS/HDRI.hpp>
#include <dz/GlobalUID.hpp>

void dz::ecs::set_radiance_control_block(Shader* shader, void* user_data) {
    RadianceControlBlock& controlBlock = *(RadianceControlBlock*)user_data;
    shader_update_push_constant(shader, 0, (void*)&controlBlock.roughness, sizeof(float));
    shader_update_push_constant(shader, 1, (void*)&controlBlock.mip, sizeof(int));
    shader_ensure_push_constants(shader);

}

dz::ecs::HDRI::HDRIReflectable::HDRIReflectable(const dz::function<HDRI*()>& get_hdri_function):
    get_hdri_function(get_hdri_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("HDRI")
{}

int dz::ecs::HDRI::HDRIReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::HDRI::HDRIReflectable::GetName() {
    return name;
}

void* dz::ecs::HDRI::HDRIReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto hdri_ptr = get_hdri_function();
    assert(hdri_ptr);
    auto& hdri = *hdri_ptr;
    switch (prop_index) {
    default: return nullptr;
    }
}

void dz::ecs::HDRI::HDRIReflectable::NotifyChange(int prop_index) {}