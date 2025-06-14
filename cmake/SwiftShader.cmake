
set(SWIFTSHADER_BUILD_PVR OFF)
set(SWIFTSHADER_BUILD_CPPDAP OFF)
set(SWIFTSHADER_BUILD_TESTS OFF)
set(SWIFTSHADER_BUILD_BENCHMARKS OFF)
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


# file(GLOB SWIFTSHADER_SOURCES
#     # "${swiftshader_SOURCE_DIR}/src/Reactor/*.cpp"
#     "${swiftshader_SOURCE_DIR}/src/System/*.cpp"
#     "${swiftshader_SOURCE_DIR}/src/Pipeline/*.cpp"
#     "${swiftshader_SOURCE_DIR}/src/Device/*.cpp"
#     "${swiftshader_SOURCE_DIR}/src/Vulkan/libVulkan.cpp"
# )

# add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/llvm-16.0)
# add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/marl)
# # add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/subzero)
# # add_subdirectory(${swiftshader_SOURCE_DIR}/third_party/llvm-subzero)

# add_library(swiftshader STATIC ${SWIFTSHADER_SOURCES})
# target_compile_features(swiftshader PRIVATE cxx_std_17)
# target_include_directories(swiftshader PRIVATE ${Vulkan_INCLUDE_DIRS})
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/src)
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/marl/include)
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/SPIRV-Headers/include)
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/llvm-16.0/llvm/include)
# if(WINDOWS)
# set(LLVM_PLATFORM windows)
# elseif(LINUX)
# set(LLVM_PLATFORM linux)
# elseif(MACOS)
# set(LLVM_PLATFORM darwin)
# elseif(ANDROID)
# set(LLVM_PLATFORM android)
# endif()
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/llvm-16.0/configs/${LLVM_PLATFORM}/include)
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/subzero/src)
# # target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/subzero)
# target_include_directories(swiftshader PRIVATE ${swiftshader_SOURCE_DIR}/third_party/llvm-16.0/configs/common/include)


# target_compile_definitions(swiftshader
#     PUBLIC
#         "ALLOW_DUMP=0"
#         "ALLOW_TIMERS=0"
#         "ALLOW_LLVM_CL=0"
#         "ALLOW_LLVM_IR=0"
#         "ALLOW_LLVM_IR_AS_INPUT=0"
#         "ALLOW_MINIMAL_BUILD=0"
#         "ALLOW_WASM=0"
#         "ICE_THREAD_LOCAL_HACK=0"
# )