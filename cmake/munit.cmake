message(STATUS "FetchContent: munit")

FetchContent_Declare(munit
    GIT_REPOSITORY https://github.com/nemequ/munit.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_GetProperties(munit)
if(NOT munit_POPULATED)
  FetchContent_Populate(munit)
endif()

add_library(munit STATIC ${munit_SOURCE_DIR}/munit.c)

target_include_directories(munit PRIVATE ${munit_SOURCE_DIR})