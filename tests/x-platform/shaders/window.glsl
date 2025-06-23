struct Window {
   float deltaTime;
   float time;
   float width;
   float height;
   int buttons[8];
   float cursor[2];
};

layout(std430, binding = 1) buffer WindowsBuffer {
    Window windows[];
} Windows;