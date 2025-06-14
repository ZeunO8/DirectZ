cmake_minimum_required(VERSION 3.2.1...4.8.8)

cmake_policy(SET CMP0169 OLD)

include(cmake/Version.cmake)

project(DirectZ VERSION ${DZ_V_MAJ}.${DZ_V_MIN}.${DZ_V_PAT}.${DZ_V_TWE})

include(cmake/PlatformDefinitions.cmake)
include(cmake/InstallPaths.cmake)

add_library(DirectZ src/DirectZ.mm)

target_compile_features(DirectZ PRIVATE cxx_std_20) 

find_package(Vulkan REQUIRED)

message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")

target_include_directories(DirectZ PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(DirectZ PRIVATE include)

target_link_libraries(DirectZ PRIVATE ${Vulkan_LIBRARIES})

include(FetchContent)

include(cmake/SwiftShader.cmake)
include(cmake/GLSL.cmake)

target_include_directories(DirectZ PRIVATE ${spirv_reflect_SOURCE_DIR})

target_link_libraries(DirectZ PRIVATE shaderc spirv-reflect-static)

if(LINUX)
	target_link_libraries(DirectZ PRIVATE objc xcb xkbcommon xcb-util xcb-keysyms xcb-xfixes X11 Xrandr Xext Xfixes xkbcommon Xrender)
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

include(./cmake/Install.cmake)
