cmake_minimum_required(VERSION 3.2.1...4.8.8)

if(POLICY CMP0169)
	cmake_policy(SET CMP0169 OLD)
endif()

if(POLICY CMP0148)
	cmake_policy(SET CMP0148 NEW)
endif()

include(cmake/Version.cmake)

project(DirectZ VERSION ${DZ_V_MAJ}.${DZ_V_MIN}.${DZ_V_PAT}.${DZ_V_TWE})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(cmake/PlatformDefinitions.cmake)
include(cmake/InstallPaths.cmake)

set(DZ_SOURCES src/DirectZ.cpp)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
	list(APPEND DZ_SOURCES src/Window.mm)
endif()

add_library(DirectZ STATIC ${DZ_SOURCES})

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") AND NOT WIN32)	
	target_compile_options(DirectZ PRIVATE -fPIC)
endif()

add_executable(dzp src/DZP.cpp)

set_target_properties(DirectZ PROPERTIES DEBUG_POSTFIX "d")

target_compile_features(DirectZ PRIVATE cxx_std_20)
target_compile_features(dzp PRIVATE cxx_std_20)

if(ANDROID)
	set(Vulkan_LIBRARIES "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/${ANDROID_TRIPLET}/${ANDROID_PLATFORM_LEVEL}/libvulkan.so")
	set(Vulkan_INCLUDE_DIRS "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include")
	set(Vulkan_FOUND TRUE)
else()
	find_package(Vulkan REQUIRED)
endif()

message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

include(cmake/SwiftShader.cmake)
include(cmake/GLSL.cmake)
include(cmake/ImGui.cmake)

include(cmake/dz-targets.cmake)


target_include_directories(DirectZ PRIVATE
	include
	${Vulkan_INCLUDE_DIRS}
	${spirv_reflect_SOURCE_DIR}
	${imgui_SOURCE_DIR}
)

target_include_directories(dzp PRIVATE ${Vulkan_INCLUDE_DIRS} include)

target_link_libraries(DirectZ PRIVATE
	${Vulkan_LIBRARIES}
	shaderc
	shaderc_util
	spirv-reflect-static
	imgui
)

target_link_libraries(dzp PRIVATE DirectZ)
if(ANDROID)
	target_link_libraries(dzp PRIVATE android log)
endif()

target_link_libraries(DirectZ PRIVATE)

if(WIN32)
	target_link_libraries(DirectZ PRIVATE gdi32 ole32 advapi32)
elseif(LINUX)
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

include(./cmake/Install.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-macro-redefined")