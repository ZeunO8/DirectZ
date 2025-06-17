### DirectZ

DirectZ `namespace dz` is a rewrite of **[Zeungine](https://github.com/Zeucor/Zeungine)** and aims to improve upon its core logic while adding *GPU driven rendering*.

DirectZ uses `spirv_reflect` to reflect developer written `.glsl` shaders. Because of this, DirectZ allows developers to simply write shader code and have their render resources created automagically for you.

Memory is mapped from the GPU to CPU, and, using compute shaders, most of the apps logic can be offloaded to the GPU to achieve highly parallel gameloops.

DirectZ provides:

- Easy API for creating windows
- Quick access to a fully reflected Shader pipeline
- GPU driven rendering

##### Getting hands on DirectZ

You can download a Release installer from the releases page or, if building from source, I recommend a clone and install

```bash
git clone https://github.com/Zeucor/DirectZ.git
cd DirectZ
cmake -B build -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++
cmake --build build

# Unix
sudo cmake --install build

# Windows (as admin)
cmake --install build
```

##### Building apps with DirectZ

To get setup with a cross platform compatible render context, create the following files: (note, this example is available at [/tests/x-platform](/tests/x-platform))

**CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.2.1...4.8.8) # use a version range

project(dz-x-platform VERSION 1.0.0)

add_library(lib-${PROJECT_NAME} SHARED src/app-lib.cpp)

find_package(DirectZ REQUIRED)

target_link_libraries(lib-${PROJECT_NAME} PRIVATE ${DirectZ_LIBRARIES})
target_include_directories(lib-${PROJECT_NAME} PRIVATE ${DirectZ_INCLUDE_DIRS})

if((WIN32 OR UNIX) AND NOT IOS AND NOT ANDROID)
    add_executable(${PROJECT_NAME} src/runtime.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE lib-${PROJECT_NAME})
elseif(ANDROID OR IOS)
    # we can link lib-${PROJECT_NAME} in an app loader and call native code
endif()
```

**src/app-lib.cpp**

```cpp
#include <DirectZ.hpp>

WINDOW* cached_window = 0;
int dz_init(WINDOW* window)
{
    cached_window = window;
    Shader* compute_shader = shader_create();
    Shader* raster_shader = shader_create();

    // See "Shaders" section for information on creating shaders
}

void dz_update()
{
    // Do any CPU side logic
}
```

**src/runtime.cpp**
```cpp
#include <DirectZ.hpp>
int main()
{
    set_direct_registry(make_direct_registry());
    auto window = window_create({
        .title = "Example Window",
        .x = 128,
        .y = 128,
        .width = 640,
        .height = 480
    });

    int ret = 0;

    // call app-lib implementation of dz_init
    if ((ret = dz_init(window)))
        return ret;
    while (window_poll_events(window))
    {
        // update and render
        dz_update();
        window_render(window);
    }
    return ret;
}
```

##### Buffer Groups

Before we get into Shaders, we must touch on Buffer Groups.

As we are now GPU driven, the core location of data for logic and rendering exists on the GPU.

Buffer Groups allow us to group a set of buffers (known as SSBOs in glsl) and images such that their data can be shared between Shaders. 

Before creating any shaders in `dz_init` *add*:

```cpp
auto main_buffer_group = buffer_group_create("main_buffer_group");
```

If using multiple buffer groups (you would when sharing specific portions of data with specific shaders) you should resetrict to specific keys in the shader.

```cpp
auto main_buffer_group = buffer_group_create("main_buffer_group");
auto windows_buffer_group = buffer_group_create("windows_buffer_group");

buffer_group_restrict_to_keys(main_buffer_group, {"Quads"});
buffer_group_restrict_to_keys(windows_buffer_group, {"WindowStates"});
```

##### Shaders

Shaders *drive* DirectZ. You write your shader using `.glsl`, DirectZ uses `spirv_reflect` to reflect the inputs/outputs, SSBOs, images back to DirectZ such that you can interface with the Shader as if it were mapped to your program.

###### Writing a raster pipeline

For our `raster_shader` we need two modules, Vertex and Fragment. Below is an implementation of a raster_shader that declares a Quad struct and the "Quads" Buffer

```cpp
shader_add_module(render_shader, ShaderModuleType::Vertex,
DZ_GLSL_VERSION + R"(
struct Quad {
    vec4 viewport;
};

layout(std430, binding = 0) buffer QuadsBuffer {
    Quad quads[];
} Quads;

vec3 get_quad_position()
{
    vec3 pos;
    switch (gl_VertexIndex)
    {
    case 0: pos = vec3(1.0, 1.0, 0);    break;
    case 1: pos = vec3(-1.0, 1.0, 0);   break;
    case 2: pos = vec3(-1.0, -1.0, 0);  break;
    case 3: pos = vec3(-1.0, -1.0, 0);  break;
    case 4: pos = vec3(1.0, -1.0, 0);   break;
    case 5: pos = vec3(1.0, 1.0, 0);    break;
    }
    // vec4 viewport = Quads.quads[gl_InstanceIndex].viewport;
    return pos;
}

vec2 get_quad_uv()
{
    switch (gl_VertexIndex)
    {
    case 0: return vec2(1, 1);
    case 1: return vec2(0, 1);
    case 2: return vec2(0, 0);
    case 3: return vec2(0, 0);
    case 4: return vec2(1, 0);
    case 5: return vec2(1, 1);
    }
    return vec2(0);
}

void main() {
    gl_Position = vec4(get_quad_position(), 1);
}
)");

shader_add_module(render_shader, ShaderModuleType::Fragment,
version + R"(
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(1, 0, 0, 1);
}
)");
```