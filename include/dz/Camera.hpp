#pragma once

#include "math.hpp"

namespace dz {
    struct Camera {
        enum ProjectionType {
            Perspective,
            Orthographic
        };
        mat<float, 4, 4> view;
        mat<float, 4, 4> projection;
        float nearPlane;
        float farPlane;
        float padding1 = 0;
        float padding2 = 0;
        vec<float, 3> position;
        float padding3;
        vec<float, 3> center;
        float padding4;
        vec<float, 3> up;
        float padding5;
        inline static std::string GetGLSLStruct() {
            return R"(
struct Camera {
    mat4 view;
    mat4 projection;
    float nearPlane;
    float farPlane;
    float padding1;
    float padding2;
    vec3 position;
    float padding3;
    vec3 center;
    float padding4;
    vec3 up;
    float padding5;
};
)";
        }
    };

    void CameraInit(
        Camera& camera, 
        vec<float, 3> position,
        vec<float, 3> center,
        vec<float, 3> up,
        float nearPlane,
        float farPlane,
        float aspect,
        float fov,
        Camera::ProjectionType projectionType = Camera::Perspective
    );
    void CameraInit(
        Camera& camera, 
        vec<float, 3> position,
        vec<float, 3> center,
        vec<float, 3> up,
        float nearPlane,
        float farPlane,
        vec<float, 4> viewport,
        Camera::ProjectionType projectionType = Camera::Orthographic
    );
        
}