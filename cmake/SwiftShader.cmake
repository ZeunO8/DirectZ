set(SPIRV_TOOLS_BUILD_STATIC ON)
set(SKIP_SPIRV_TOOLS_INSTALL ON)

set(SWIFTSHADER_BUILD_PVR OFF)
set(SWIFTSHADER_BUILD_CPPDAP OFF)
set(SWIFTSHADER_BUILD_TESTS OFF)
set(SWIFTSHADER_BUILD_BENCHMARKS OFF)
set(SWIFTSHADER_WARNINGS_AS_ERRORS OFF)
message(STATUS "FetchContent: swiftshader")
FetchContent_Declare(swiftshader
	GIT_REPOSITORY https://github.com/ZeunO8/swiftshader.git
	GIT_TAG ANDROID)
FetchContent_GetProperties(swiftshader)
if(NOT swiftshader_POPULATED)
    FetchContent_Populate(swiftshader)
endif()

add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/SPIRV-Headers EXCLUDE_FROM_ALL)
add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/SPIRV-Tools EXCLUDE_FROM_ALL)
set_target_properties(SPIRV-Tools-static PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(SPIRV-Tools-opt PROPERTIES DEBUG_POSTFIX "d")

set(GLSLANG_TESTS OFF)
set(GLSLANG_ENABLE_INSTALL OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
message(STATUS "FetchContent: glslang")
FetchContent_Declare(glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG vulkan-sdk-1.3.283)
FetchContent_MakeAvailable(glslang)
 
set_target_properties(glslang PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(OSDependent PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(MachineIndependent PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(SPIRV PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(GenericCodeGen PROPERTIES DEBUG_POSTFIX "d")
add_subdirectory(${swiftshader_SOURCE_DIR} EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)

if(WIN32)
    set_target_properties(llvm PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(vk_device PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(vk_pipeline PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(ReactorSubzero PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(ReactorLLVM PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(ReactorLLVMSubmodule PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(vk_system PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(vk_swiftshader PROPERTIES POSITION_INDEPENDENT_CODE "")
    set_target_properties(vk_wsi PROPERTIES POSITION_INDEPENDENT_CODE "")
    if(SWIFTSHADER_EMIT_COVERAGE)
        set_target_properties(llvm-with-cov PROPERTIES POSITION_INDEPENDENT_CODE "")
    endif()
endif()

if(WIN32)
    set(VULKAN_API_LIBRARY_NAME "vulkan-1.dll")
elseif(LINUX)
    set(VULKAN_API_LIBRARY_NAME "libvulkan.so.1")
elseif(APPLE)
    set(VULKAN_API_LIBRARY_NAME "libvulkan.dylib")
elseif(FUCHSIA OR ANDROID)
    set(VULKAN_API_LIBRARY_NAME "libvulkan.so")
else()
    message(FATAL_ERROR "Platform does not support Vulkan yet")
endif()
add_custom_target(copy_vk_swiftshader ALL
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vk_swiftshader> "${CMAKE_BINARY_DIR}/SwiftShader"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vk_swiftshader> "${CMAKE_BINARY_DIR}/SwiftShader/${VULKAN_API_LIBRARY_NAME}"
    DEPENDS vk_swiftshader
    COMMENT "Copying vk_swiftshader to binary root"
) 

set(ICD_LIBRARY_PATH "${CMAKE_SHARED_LIBRARY_PREFIX}vk_swiftshader${CMAKE_SHARED_LIBRARY_SUFFIX}")
if(WIN32)
    # The path is output to a JSON file, which requires backslashes to be escaped.
    set(ICD_LIBRARY_PATH ".\\\\${ICD_LIBRARY_PATH}")
else()
    set(ICD_LIBRARY_PATH "./${ICD_LIBRARY_PATH}")
endif()
configure_file(
    "${swiftshader_SOURCE_DIR}/src/Vulkan/vk_swiftshader_icd.json.tmpl"
    "${CMAKE_BINARY_DIR}/SwiftShader/vk_swiftshader_icd.json"
)


if(ANDROID)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
	target_include_directories(vk_pipeline PUBLIC ${swiftshader_SOURCE_DIR}/include/Android)
	target_include_directories(vk_pipeline PUBLIC ${CMAKE_SOURCE_DIR}/android)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${swiftshader_SOURCE_DIR}/src/commit_id.py gen ${CMAKE_SOURCE_DIR}/android/commit.h
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Python script failed with error: ${err}")
    endif()

    message(STATUS "Python script output: ${out}")

    # Extract numeric API level (e.g. from "android-30" → "30")
    string(REGEX REPLACE "^android-([0-9]+).*$" "\\1" ANDROID_API_LEVEL "${ANDROID_PLATFORM}")

    # Define __ANDROID_API__ macro
    add_compile_definitions(__ANDROID_API__=${ANDROID_API_LEVEL})

    message(STATUS "Derived __ANDROID_API__=${ANDROID_API_LEVEL} from ANDROID_PLATFORM=${ANDROID_PLATFORM}")
endif()