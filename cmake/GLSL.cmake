


# set(SPIRV_SKIP_EXECUTABLES ON)
# set(SPIRV_SKIP_TESTS ON)
# set(SKIP_SPIRV_TOOLS_INSTALL ON)
# FetchContent_Declare(spirv_tools
#     GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
#     GIT_TAG main)
# FetchContent_MakeAvailable(spirv_tools)


set(SHADERC_SKIP_INSTALL ON)
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON)
set(SHADERC_ENABLE_WERROR_COMPILE OFF)
FetchContent_Declare(shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG v2024.2)
FetchContent_MakeAvailable(shaderc)

set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
FetchContent_Declare(spirv_reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG main)
FetchContent_MakeAvailable(spirv_reflect)