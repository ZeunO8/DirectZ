struct Camera {
   mat4 view;
   mat4 proj;
};

layout(std430, binding = 2) buffer CamerasBuffer {
    Camera cameras[];
} Cameras;