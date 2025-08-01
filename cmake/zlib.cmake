FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG develop
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(zlib)
set_target_properties(zlibstatic PROPERTIES OUTPUT_NAME zlibstatic)
set_target_properties(zlibstatic PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(zlibstatic PROPERTIES RELEASE_POSTFIX "")
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})