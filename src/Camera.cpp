#include <dz/Camera.hpp>
void dz::CameraInit(
    Camera& camera,
    vec<float, 3> position,
    vec<float, 3> center,
    vec<float, 3> up,
    float nearPlane,
    float farPlane,
    float width,
    float height,
    float fov,
    Camera::ProjectionType projectionType
) {
    assert(projectionType == Camera::Perspective);
    camera.nearPlane = nearPlane;
    camera.farPlane = farPlane;
    camera.position = position;
    camera.center = center;
    camera.up = up;
    camera.type = int(projectionType);
    camera.aspect = width / height;
    camera.fov = fov;
    camera.orthoWidth = width;
    camera.orthoHeight = height;
    CameraInit(camera);
}
void dz::CameraInit(
    Camera& camera,
    vec<float, 3> position,
    vec<float, 3> center,
    vec<float, 3> up,
    float nearPlane,
    float farPlane,
    vec<float, 4> viewport,
    Camera::ProjectionType projectionType
) {
    assert(projectionType == Camera::Orthographic);
    camera.nearPlane = nearPlane;
    camera.farPlane = farPlane;
    camera.position = position;
    camera.center = center;
    camera.up = up;
    camera.type = int(projectionType);
    camera.orthoWidth = viewport[3];
    camera.orthoHeight = viewport[4];
    camera.aspect = camera.orthoWidth / camera.orthoHeight;
    CameraInit(camera);
}

void dz::CameraInit(Camera& camera) {
    switch(Camera::ProjectionType(camera.type))
    {
    case Camera::Perspective:
        camera.projection = perspective(camera.fov, camera.aspect, camera.nearPlane, camera.farPlane);
        camera.view = lookAt(camera.position, camera.center, camera.up);
        break;
    case Camera::Orthographic:
        camera.projection = orthographic(-camera.orthoWidth / 2.f, camera.orthoWidth / 2.f, -camera.orthoHeight / 2.f, camera.orthoHeight / 2.f, camera.nearPlane, camera.farPlane);
        camera.view = lookAt(camera.position, camera.center, camera.up);
        break;
    default: break;
    }
}

Camera::CameraTypeReflectable::CameraTypeReflectable(
    const std::function<Camera*()>& get_camera_function,
    const std::function<void()>& reset_reflectables_function
):
    get_camera_function(get_camera_function),
    reset_reflectables_function(reset_reflectables_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Camera Type")
{}

int Camera::CameraTypeReflectable::GetID() {
    return uid;
}

std::string& Camera::CameraTypeReflectable::GetName() {
    return name;
}

void* Camera::CameraTypeReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0:
        return &camera.type;
    default: return nullptr;
    }
}

void Camera::CameraTypeReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        CameraInit(camera);
        reset_reflectables_function();
        break;
    }
}

Camera::CameraViewReflectable::CameraViewReflectable(const std::function<Camera*()>& get_camera_function):
    get_camera_function(get_camera_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("View Transform")
{}

int Camera::CameraViewReflectable::GetID() {
    return uid;
}

std::string& Camera::CameraViewReflectable::GetName() {
    return name;
}

void* Camera::CameraViewReflectable::GetVoidPropertyByIndex(int prop_index) {
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

void Camera::CameraViewReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        CameraInit(camera);
        break;
    }
}

Camera::CameraPerspectiveReflectable::CameraPerspectiveReflectable(const std::function<Camera*()>& get_camera_function):
    get_camera_function(get_camera_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Perspective")
{}

int Camera::CameraPerspectiveReflectable::GetID() {
    return uid;
}

std::string& Camera::CameraPerspectiveReflectable::GetName() {
    return name;
}

void* Camera::CameraPerspectiveReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0:
        return &camera.aspect;
    case 1:
        return &camera.fov;
    default: return nullptr;
    }
}

void Camera::CameraPerspectiveReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        CameraInit(camera);
        break;
    }
}

Camera::CameraOrthographicReflectable::CameraOrthographicReflectable(const std::function<Camera*()>& get_camera_function):
    get_camera_function(get_camera_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Orthographic")
{}

int Camera::CameraOrthographicReflectable::GetID() {
    return uid;
}

std::string& Camera::CameraOrthographicReflectable::GetName() {
    return name;
}

void* Camera::CameraOrthographicReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return nullptr;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    case 0:
        return &camera.orthoWidth;
    case 1:
        return &camera.orthoHeight;
    default: return nullptr;
    }
}

void Camera::CameraOrthographicReflectable::NotifyChange(int prop_index) {
    auto camera_ptr = get_camera_function();
    if (!camera_ptr)
        return;
    auto& camera = *camera_ptr;
    switch (prop_index) {
    default:
        CameraInit(camera);
        break;
    }
}