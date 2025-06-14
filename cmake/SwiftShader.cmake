
set(SWIFTSHADER_BUILD_PVR OFF)
set(SWIFTSHADER_BUILD_CPPDAP OFF)
set(SWIFTSHADER_BUILD_TESTS OFF)
set(SWIFTSHADER_BUILD_BENCHMARKS OFF)
set(SWIFTSHADER_WARNINGS_AS_ERRORS OFF)
FetchContent_Declare(swiftshader
	GIT_REPOSITORY https://github.com/ZeunO8/swiftshader.git
	GIT_TAG PIC)
FetchContent_GetProperties(swiftshader)
if(NOT swiftshader_POPULATED)
    FetchContent_Populate(swiftshader)
endif()

add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/SPIRV-Headers EXCLUDE_FROM_ALL)
add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/SPIRV-Tools EXCLUDE_FROM_ALL)

set(GLSLANG_TESTS OFF)
set(GLSLANG_ENABLE_INSTALL OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
FetchContent_Declare(glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG vulkan-sdk-1.3.283)
FetchContent_MakeAvailable(glslang)
 
add_subdirectory(${swiftshader_SOURCE_DIR} EXCLUDE_FROM_ALL)

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

add_custom_target(copy_vk_swiftshader ALL
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vk_swiftshader> "${CMAKE_BINARY_DIR}/"
    DEPENDS vk_swiftshader
    COMMENT "Copying vk_swiftshader to binary root"
) 

configure_file(
    "${swiftshader_SOURCE_DIR}/src/Vulkan/vk_swiftshader_icd.json.tmpl"
    "${CMAKE_BINARY_DIR}/vk_swiftshader_icd.json"
)
