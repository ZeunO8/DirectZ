struct Camera {
   mat4 mvp;
};

layout(std430, binding = 2) buffer CamerasBuffer {
    Camera cameras[];
} Cameras;