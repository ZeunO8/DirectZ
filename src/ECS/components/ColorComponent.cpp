#include <dz/ECS/components/ColorComponent.hpp>
using namespace dz::ecs;

ColorComponent::ColorComponentReflectable::ColorComponentReflectable(const std::function<ColorComponent*()>& get_color_component_function):
    get_color_component_function(get_color_component_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Color")
{}

int ColorComponent::ColorComponentReflectable::GetID() {
    return uid;
}

std::string& ColorComponent::ColorComponentReflectable::GetName() {
    return name;
}

void* ColorComponent::ColorComponentReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto color_component_ptr = get_color_component_function();
    assert(color_component_ptr);
    auto& color_component = *color_component_ptr;
    switch (prop_index) {
    case 0: return &color_component.color;
    default: return nullptr;
    }
}

void ColorComponent::ColorComponentReflectable::NotifyChange(int prop_index) {}