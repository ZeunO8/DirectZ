### DirectZ

DirectZ `namespace dz` is a rewrite of **Zeungine** and aims to improve upon its core logic while adding *GPU driven rendering*.

DirectZ uses `spirv_reflect` to reflect developer written `.glsl` shaders. Because of this, DirectZ allows developers to simply write shader code and have their render resources created automagically for you.

Memory is mapped from the GPU to CPU, and, using compute shaders, most of the apps logic can be offloaded to the GPU to achieve highly parallel gameloops.

DirectZ provides:

- Easy API for creating windows
- Quick access to a Shader pipeline
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

Create the following files:

**CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.2.1...4.8.8) # use a version range

project(your-app VERSION 1.0.0)

add_library(lib-${PROJECT_NAME} src/app-lib.cpp)

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
```

**src/runtime.cpp**
```cpp
int main()
{

}
```

