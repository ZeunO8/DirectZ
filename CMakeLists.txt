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

set(DZ_SOURCES src/DirectZ.cpp)

if(MACOS OR IOS)
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
    set(VULKAN_LIB_PATH "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/${ANDROID_TRIPLET}/${ANDROID_PLATFORM_LEVEL}/libvulkan.so")
    set(VULKAN_INCLUDE_PATH "${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include")
    add_library(Vulkan::Vulkan SHARED IMPORTED GLOBAL)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${VULKAN_LIB_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_PATH}"
    )
    set(Vulkan_FOUND TRUE)
elseif(IOS)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
	find_package(Vulkan REQUIRED COMPONENTS MoltenVK)
else()
	find_package(Vulkan REQUIRED)
endif()

message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

include(cmake/SwiftShader.cmake)
include(cmake/GLSL.cmake)
include(cmake/ImGui.cmake)
include(cmake/iostreams.cmake)

include(cmake/dz-targets.cmake)

target_include_directories(DirectZ PRIVATE
	include
	${Vulkan_INCLUDE_DIRS}
	${spirv_reflect_SOURCE_DIR}
	${imgui_SOURCE_DIR}
	${imguizmo_SOURCE_DIR}
	${iostreams_SOURCE_DIR}/include
)

target_include_directories(dzp PRIVATE
	include
	${Vulkan_INCLUDE_DIRS}
	${imgui_SOURCE_DIR}
	${imguizmo_SOURCE_DIR}
	${iostreams_SOURCE_DIR}/include
)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-macro-redefined")

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