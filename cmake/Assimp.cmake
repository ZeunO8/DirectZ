message(STATUS "FetchContent: assimp")
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_BUILD_ZLIB OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/ZeunO8/assimp.git
    GIT_TAG zlib-cmake-fix-v6.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(assimp)
SET_TARGET_PROPERTIES(assimp PROPERTIES OUTPUT_NAME assimp)
set_target_properties(assimp PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(assimp PROPERTIES RELEASE_POSTFIX "")