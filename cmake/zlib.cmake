FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG develop
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(zlib)
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})