cmake_minimum_required(VERSION 3.2.1...4.8.8)

if(POLICY CMP0169)
	cmake_policy(SET CMP0169 OLD)
endif()

if(POLICY CMP0148)
	cmake_policy(SET CMP0148 NEW)
endif()

include(cmake/Version.cmake)

set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 0)

project(DirectZ VERSION ${DZ_V_MAJ}.${DZ_V_MIN}.${DZ_V_PAT}.${DZ_V_TWE})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(cmake/PlatformDefinitions.cmake)
include(cmake/InstallPaths.cmake)
include(cmake/Dependencies.cmake)

set(DZ_SOURCES src/DirectZ.cpp)

if(MACOS OR IOS)
	list(APPEND DZ_SOURCES src/Window.mm)
endif()

add_library(DirectZ STATIC ${DZ_SOURCES})

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang") AND NOT WIN32)	
	target_compile_options(DirectZ PRIVATE -fPIC)
endif()

add_executable(dzp src/DZP.cpp)

list(APPEND DirectZ_EXES dzp)

set_target_properties(DirectZ PROPERTIES DEBUG_POSTFIX "d")

target_compile_features(DirectZ PRIVATE cxx_std_20)
target_compile_features(dzp PRIVATE cxx_std_20)

include(cmake/dz-targets.cmake)

set(DIRECTZ_INCLUDE_DIRS
	include
	${Vulkan_INCLUDE_DIRS}
	${spirv_reflect_SOURCE_DIR}
	${imgui_SOURCE_DIR}
	${imguizmo_SOURCE_DIR}
	${iostreams_SOURCE_DIR}/include
	${rectpack_SOURCE_DIR}/src
	${stb_SOURCE_DIR}
	${assimp_SOURCE_DIR}/include
)

target_include_directories(DirectZ PRIVATE ${DIRECTZ_INCLUDE_DIRS})

target_include_directories(dzp PRIVATE ${DIRECTZ_INCLUDE_DIRS})

target_link_libraries(DirectZ PRIVATE
	Vulkan::Vulkan
	shaderc
	shaderc_util
	spirv-reflect-static
	imgui
	imguizmo
	SPIRV
	SPIRV-Tools
	SPIRV-Tools-opt
	iostreams
	archive_static
	assimp
	zlibstatic
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
elseif(IOS)
	target_link_libraries(DirectZ PRIVATE
		"-framework UIKit"
		"-framework Foundation"
		"-framework QuartzCore"
		"-framework Metal"
	)
elseif(MACOS)
	target_link_libraries(DirectZ PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal")
endif()

include(./Tests.cmake)

include(./cmake/Install.cmake)

include(./cmake/Package.cmake)

set(DZ_COMPONENTS Core)

crossplatform_cpack_helper(
    ${CMAKE_SOURCE_DIR}/releases
    DirectZ
    DirectZ
    ${DZ_V_MAJ}
    ${DZ_V_MIN}
    ${DZ_V_PAT}
    ${DZ_V_TWE}
    ${CMAKE_SOURCE_DIR}/LICENSE
    "DirectZ: GPU driven renders"
    "An abstraction over the GPU/CPU memory+render layers to provide high performance reflected shader contexts. Has various included dependencies including an ImGui layer for rendering UI"
    ""
    ""
    "Zeucor"
    "Zeun"
    "Zeun"
    "libgl1-mesa-dri,libx11-6,libssl-dev,libvulkan1,libxrandr2,libxfixes3,libxkbcommon-x11-0,libsecret-1-0,libglib2.0-dev,libdrm-dev"
    ""
    "Development/Tools"
    "Program Files\\\\DirectZ"
    "C:"
    "DirectZ-uninstaller"
    "dev.zeucor.DirectZ"
    "${DZ_COMPONENTS}"
)