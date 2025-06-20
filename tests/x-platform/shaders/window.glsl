struct Window {
   float deltaTime;
   float time;
};

layout(std430, binding = 1) buffer WindowsBuffer {
    Window windows[];
} Windows;