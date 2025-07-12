#include <dz/Camera.hpp>
void dz::CameraInit(
    Camera& camera,
    vec<float, 3> position,
    vec<float, 3> center,
    vec<float, 3> up,
    float nearPlane,
    float farPlane,
    float aspect,
    float fov,
    Camera::ProjectionType projectionType
) {
    assert(projectionType == Camera::Perspective);
    camera.nearPlane = nearPlane;
    camera.farPlane = farPlane;
    camera.projection = perspective(fov, aspect, camera.nearPlane, camera.farPlane);
    camera.position = position;
    camera.center = center;
    camera.up = up;
    camera.view = lookAt(camera.position, camera.center, camera.up);
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
    camera.projection = orthographic(-viewport[3] / 2.f, viewport[3] / 2.f, -viewport[4] / 2.f, viewport[4] / 2.f, camera.nearPlane, camera.farPlane);
    camera.position = position;
    camera.center = center;
    camera.up = up;
    camera.view = lookAt(camera.position, camera.center, camera.up);
}