set(SHADERC_SKIP_INSTALL ON)
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON)
set(SHADERC_ENABLE_WERROR_COMPILE OFF)
message(STATUS "FetchContent: shaderc")
FetchContent_Declare(shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG main)
FetchContent_MakeAvailable(shaderc)

set_target_properties(shaderc PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(shaderc_util PROPERTIES DEBUG_POSTFIX "d")

set_target_properties(shaderc PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${shaderc_SOURCE_DIR}/libshaderc/include>;$<INSTALL_INTERFACE:include>"
)

set_target_properties(shaderc_util PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${shaderc_SOURCE_DIR}/libshaderc_util/include>;$<INSTALL_INTERFACE:include>"
)

message(STATUS "FetchContent: spirv_reflect")
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
FetchContent_Declare(spirv_reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG main)
FetchContent_MakeAvailable(spirv_reflect)

set_target_properties(spirv-reflect-static PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(spirv-reflect-static PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${spirv_reflect_SOURCE_DIR}/include>;$<INSTALL_INTERFACE:include/>"
)