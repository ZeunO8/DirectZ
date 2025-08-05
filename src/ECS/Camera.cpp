#include <dz/ECS/Camera.hpp>
#include <dz/GlobalUID.hpp>

dz::ecs::Camera dz::ecs::Camera::DefaultPerspective = {
    .nearPlane = 0.25f,
    .farPlane = 1000.f,
    .type = dz::ecs::Camera::Perspective,
    .position = {0, 0, 10},
    .center = {0, 0, 0},
    .up = {0, 1, 0}
};
dz::ecs::Camera dz::ecs::Camera::DefaultOrthographic = {
    .nearPlane = 0.25f,
    .farPlane = 1000.f,
    .type = dz::ecs::Camera::Orthographic,
    .position = {0, 0, 10},
    .center = {0, 0, 0},
    .up = {0, 1, 0}
};

dz::ecs::Camera::CameraMetaReflectable::CameraMetaReflectable(
    const std::function<Camera*()>& get_camera_function,
    const std::function<void()>& reset_reflectables_function
):
    get_camera_function(get_camera_function),
    reset_reflectables_function(reset_reflectables_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Camera Meta")
{}

int dz::ecs::Camera::CameraMetaReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Camera::CameraMetaReflectable::GetName() {
    return name;
}

void* dz::ecs::Camera::CameraMetaReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0: return &camera.type;
    case 1: return &camera.is_active;
    default: return nullptr;
    }
}

void dz::ecs::Camera::CameraMetaReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        camera.Initialize();
        reset_reflectables_function();
        break;
    }
}

dz::ecs::Camera::CameraViewReflectable::CameraViewReflectable(const std::function<Camera*()>& get_camera_function):
    get_camera_function(get_camera_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("View Transform")
{}

int dz::ecs::Camera::CameraViewReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Camera::CameraViewReflectable::GetName() {
    return name;
}

void* dz::ecs::Camera::CameraViewReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0:
        return &camera.position;
    case 1:
        return &camera.center;
    case 2:
        return &camera.up;
    case 3:
        return &camera.nearPlane;
    case 4:
        return &camera.farPlane;
    default: return nullptr;
    }
}

void dz::ecs::Camera::CameraViewReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        camera.Initialize();
        camera.transform_dirty = 1;
        break;
    }
}

dz::ecs::Camera::CameraPerspectiveReflectable::CameraPerspectiveReflectable(const std::function<Camera*()>& get_camera_function):
    get_camera_function(get_camera_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Perspective")
{}

int dz::ecs::Camera::CameraPerspectiveReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Camera::CameraPerspectiveReflectable::GetName() {
    return name;
}

void* dz::ecs::Camera::CameraPerspectiveReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0: return &camera.fov;
    default: return nullptr;
    }
}

void dz::ecs::Camera::CameraPerspectiveReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        camera.Initialize();
        break;
    }
}

dz::ecs::Camera::CameraOrthographicReflectable::CameraOrthographicReflectable(const std::function<Camera*()>& get_camera_function):
    get_camera_function(get_camera_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Orthographic")
{}

int dz::ecs::Camera::CameraOrthographicReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Camera::CameraOrthographicReflectable::GetName() {
    return name;
}

void* dz::ecs::Camera::CameraOrthographicReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0:
        return &camera.width;
    case 1:
        return &camera.height;
    default: return nullptr;
    }
}

void dz::ecs::Camera::CameraOrthographicReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        camera.Initialize();
        break;
    }
}