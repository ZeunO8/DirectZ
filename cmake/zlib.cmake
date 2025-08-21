set(ZLIB_INSTALL OFF)
set(ZLIB_BUILD_SHARED OFF)
set(ZLIB_BUILD_TESTING OFF)
FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG develop
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(zlib)
set_target_properties(zlibstatic PROPERTIES OUTPUT_NAME zlibstatic)
set_target_properties(zlibstatic PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(zlibstatic PROPERTIES RELEASE_POSTFIX "")

set(ZLIB_FOUND TRUE)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})
set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})

add_library(z INTERFACE)
target_link_libraries(z INTERFACE zlibstatic)
set_target_properties(z PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(z PROPERTIES RELEASE_POSTFIX "")