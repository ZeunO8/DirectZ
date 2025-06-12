cmake_minimum_required(VERSION 3.2.1...4.8.8)

include(cmake/Version.cmake)

project(DirectZ VERSION ${DZ_V_MAJ}.${DZ_V_MIN}.${DZ_V_PAT}.${DZ_V_TWE})

include(cmake/PlatformDefinitions.cmake)
include(cmake/InstallPaths.cmake)

add_library(DirectZ src/DirectZ.cpp)

target_compile_features(DirectZ PRIVATE cxx_std_17)

find_package(Vulkan REQUIRED)

message(STATUS "Vulkan include path: ${Vulkan_INCLUDE_DIRS}")
get_filename_component(VULKAN_SDK_PATH "${Vulkan_INCLUDE_DIRS}" DIRECTORY)
message(STATUS "Derived Vulkan SDK path: ${VULKAN_SDK_PATH}")

target_include_directories(DirectZ PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(DirectZ PRIVATE include)

target_link_libraries(DirectZ PRIVATE ${Vulkan_LIBRARIES})

include(FetchContent)

FetchContent_Declare(spirv_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
    GIT_TAG main)
FetchContent_MakeAvailable(spirv_headers)

set(SPIRV_SKIP_EXECUTABLES ON)
set(SPIRV_SKIP_TESTS ON)
set(SKIP_SPIRV_TOOLS_INSTALL ON)
FetchContent_Declare(spirv_tools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
    GIT_TAG main)
FetchContent_MakeAvailable(spirv_tools)

set(GLSLANG_TESTS OFF)
set(GLSLANG_ENABLE_INSTALL OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
FetchContent_Declare(glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG main)
FetchContent_MakeAvailable(glslang)

set(SHADERC_SKIP_INSTALL ON)
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON)
set(SHADERC_ENABLE_WERROR_COMPILE OFF)
FetchContent_Declare(shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG main)
FetchContent_MakeAvailable(shaderc)

set(SPIRV_CROSS_STATIC ON)
set(SPIRV_CROSS_SHARED OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_SKIP_INSTALL ON)
FetchContent_Declare(spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG main)
FetchContent_MakeAvailable(spirv_cross)

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
FetchContent_Declare(spirv_reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG main)
FetchContent_MakeAvailable(spirv_reflect)

target_include_directories(DirectZ PRIVATE ${spirv_reflect_SOURCE_DIR})

target_link_libraries(DirectZ PRIVATE shaderc spirv-reflect-static spirv-cross-core spirv-cross-cpp spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl spirv-cross-reflect spirv-cross-util)

if(LINUX)
	target_link_libraries(DirectZ PRIVATE xcb xkbcommon xcb-util xcb-keysyms xcb-xfixes X11 Xrandr Xext Xfixes xkbcommon Xrender)
	if(USE_WAYLAND)
		target_link_libraries(DirectZ PRIVATE wayland-client)
	endif()
	target_link_libraries(DirectZ PRIVATE drm)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
	if(GTKMM_FOUND)
		target_link_libraries(DirectZ PRIVATE PkgConfig::GTKMM)
	endif()
elseif(MACOS)
	target_link_libraries(DirectZ PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal")
endif()

include(./Tests.cmake)