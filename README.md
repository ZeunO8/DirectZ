### DirectZ

![GitHub](https://img.shields.io/github/license/ZeunO8/DirectZ)
![GitHub repo size](https://img.shields.io/github/repo-size/ZeunO8/DirectZ)

DirectZ `namespace dz` is a rewrite of **[Zeungine](https://github.com/Zeucor/Zeungine)** and aims to improve upon its core logic while adding *GPU driven rendering*.

DirectZ uses `spirv_reflect` to reflect developer written `.glsl` shaders. Because of this, DirectZ allows developers to simply write shader code and have their render resources created automagically for you.

Memory is mapped from the GPU to CPU, and, using compute shaders, most of the apps logic can be offloaded to the GPU to achieve highly parallel gameloops.

DirectZ provides:

- Easy API for creating windows
- Quick access to a fully reflected Shader pipeline
- GPU driven rendering

##### Build Status
[![MacOS (ARM) Build](https://github.com/ZeunO8/DirectZ/actions/workflows/macos-arm.yml/badge.svg?branch=master&label=MacOS%20ARM%20Build)](https://github.com/ZeunO8/DirectZ/actions/workflows/macos-arm.yml)
[![MacOS (x86_64) Build](https://github.com/ZeunO8/DirectZ/actions/workflows/macos-x64.yml/badge.svg?branch=master&label=MacOS%20x86_64%20Build)](https://github.com/ZeunO8/DirectZ/actions/workflows/macos-x64.yml)
[![Linux (x86_64) Build](https://github.com/ZeunO8/DirectZ/actions/workflows/linux.yml/badge.svg?branch=master&label=Linux%20Build)](https://github.com/ZeunO8/DirectZ/actions/workflows/linux.yml)
[![Windows (AMD64) Build](https://github.com/ZeunO8/DirectZ/actions/workflows/windows.yml/badge.svg?branch=master&label=Windows%20Build)](https://github.com/ZeunO8/DirectZ/actions/workflows/windows.yml)
[![Android (aarch64) Build](https://github.com/ZeunO8/DirectZ/actions/workflows/android-aarch64.yml/badge.svg?branch=master&label=Android%20ARM64%20Build)](https://github.com/ZeunO8/DirectZ/actions/workflows/android-aarch64.yml)
[![iOS (ARM64) Build](https://github.com/ZeunO8/DirectZ/actions/workflows/ios-arm64.yml/badge.svg?branch=master&label=iOS%20ARM64%20Build)](https://github.com/ZeunO8/DirectZ/actions/workflows/ios-arm64.yml)

## Documentation

DirectZ has a GitHub pages site, a static build of the Doxygen documentaion.

It is available at [https://ZeunO8.github.io/DirectZ/](https://ZeunO8.github.io/DirectZ/)

## Getting hands on DirectZ

You can download a Release installer from the releases page or, if building from source, I recommend a clone and install

```bash
git clone https://github.com/Zeucor/DirectZ.git --recurse-submodules
cd DirectZ
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build

# Unix
sudo cmake --install build

# Windows (as admin)
cmake --install build
```

## Building apps with DirectZ

To get setup with a cross platform compatible render context, create the following files.

(**note**: this example is available in full at [/tests/x-platform](https://github.com/ZeunO8/DirectZ/tree/master/tests/x-platform))

**CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.2.1...4.8.8) # use a version range

project(dz-x-platform VERSION 1.0.0)

set(LIB_NAME x-platform)

add_library(${LIB_NAME} SHARED src/app-lib.cpp)
target_compile_features(${LIB_NAME} PRIVATE cxx_std_20)

find_package(DirectZ REQUIRED)

message(STATUS "DirectZ_LIBRARIES: ${DirectZ_LIBRARIES}")
message(STATUS "DirectZ_INCLUDE_DIRS: ${DirectZ_INCLUDE_DIRS}")

target_include_directories(${LIB_NAME} PRIVATE ${DirectZ_INCLUDE_DIRS})
target_link_libraries(${LIB_NAME} PRIVATE ${DirectZ_LIBRARIES})

if((WIN32 OR UNIX) AND NOT IOS AND NOT ANDROID)
    add_executable(${PROJECT_NAME} src/runtime.cpp)
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    target_include_directories(${PROJECT_NAME} PRIVATE ${DirectZ_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME})
elseif(ANDROID OR IOS)
    # we can link ${LIB_NAME} in an app loader and call native code
endif()
```

**src/app-lib.cpp**

```cpp
#include <DirectZ.hpp>

WINDOW* cached_window = 0;

DZ_EXPORT EventInterface* init(const WindowCreateInfo& window_info)
{
    cached_window = window_create(window_info);
    Shader* compute_shader = shader_create();
    Shader* raster_shader = shader_create();

    // See "Shaders" section for information on creating shaders
    return window_get_event_interface(cached_window);
}

DZ_EXPORT bool poll_events()
{
    return window_poll_events(cached_window);
}

DZ_EXPORT void update()
{
    // Do any CPU side logic
}

DZ_EXPORT void render()
{
    window_render(cached_window);
}
```

**src/runtime.cpp**
```cpp
#include <DirectZ.hpp>
int main()
{
    EventInterface* ei = 0;

    // call app-lib implementation of init
    if (!(ei = init({
        .title = "Example Window",
        .x = 128,
        .y = 128,
        .width = 640,
        .height = 480
    })))
        return 1;

    //
    while (poll_events())
    {
        update();
        render();
    }

    return 0;
}

```

### Buffer Groups

Before we get into Shaders, we must touch on Buffer Groups.

As we are now GPU driven, the core location of data for logic and rendering exists on the GPU.

Buffer Groups allow us to group a set of buffers (known as SSBOs in glsl) and images such that their data can be shared between Shaders. 

Before creating any shaders in `init` *add*:

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

### Shaders

Shaders *drive* DirectZ. You write your shader using `.glsl`, DirectZ uses `spirv_reflect` to reflect the inputs/outputs, SSBOs, images back to DirectZ such that you can interface with the Shader as if it were mapped to your program.

##### Writing a raster pipeline

For our `raster_shader` we need two modules, Vertex and Fragment. Below is an implementation of a raster_shader that declares a Quad struct and the "Quads" Buffer

```cpp
shader_add_module(raster_shader, ShaderModuleType::Vertex,
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

shader_add_module(raster_shader, ShaderModuleType::Fragment,
DZ_GLSL_VERSION + R"(
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(1, 0, 0, 1);
}
)");
```

## License

Almost all of the code in this repository is licensed under the MIT license.

Dependencies and fonts may have their own license