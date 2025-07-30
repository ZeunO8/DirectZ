set(SPIRV_TOOLS_BUILD_STATIC ON)
set(SKIP_SPIRV_TOOLS_INSTALL ON)

message(STATUS "FetchContent: spirv_headers")
FetchContent_Declare(spirv_headers
	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
	GIT_TAG main)
FetchContent_MakeAvailable(spirv_headers)

message(STATUS "FetchContent: spirv_tools")
FetchContent_Declare(spirv_tools
	GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
	GIT_TAG main)
FetchContent_MakeAvailable(spirv_tools)

set_target_properties(SPIRV-Tools-static PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(SPIRV-Tools-opt PROPERTIES DEBUG_POSTFIX "d")

set(GLSLANG_TESTS OFF)
set(GLSLANG_ENABLE_INSTALL OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
message(STATUS "FetchContent: glslang")
FetchContent_Declare(glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG main)
FetchContent_MakeAvailable(glslang)
 
set_target_properties(glslang PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(OSDependent PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(MachineIndependent PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(SPIRV PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(GenericCodeGen PROPERTIES DEBUG_POSTFIX "d")