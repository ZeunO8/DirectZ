cmake_minimum_required(VERSION 3.18.1)
project("mdirecz")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    if(NOT DirectZ_DIR)
        set(DirectZ_DIR "C:/Program Files/DirectZ/lib/android/${CMAKE_SYSTEM_PROCESSOR}/cmake/DirectZ")
    endif()
endif()

add_subdirectory(../../../../../ build_x_platform)

find_package(DirectZ REQUIRED)

add_library(glue SHARED glue.cpp)
target_link_libraries(glue PRIVATE x-platform android log)
target_include_directories(glue PRIVATE ${DirectZ_INCLUDE_DIRS})

add_dependencies(glue pack_assets_pack)
#message(FATAL_ERROR "DirectZ_INCLUDE_DIRS: ${DirectZ_INCLUDE_DIRS}")
