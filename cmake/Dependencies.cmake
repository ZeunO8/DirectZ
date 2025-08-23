
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

include(cmake/Vulkan.cmake)
include(cmake/SPIRV.cmake)
include(cmake/GLSL.cmake)
include(cmake/ImGui.cmake)
include(cmake/rectpack2D.cmake)
include(cmake/zlib.cmake)
include(cmake/iostreams.cmake)
include(cmake/stb.cmake)
include(cmake/Assimp.cmake)
include(cmake/LLVM.cmake)