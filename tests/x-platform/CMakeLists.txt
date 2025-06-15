cmake_minimum_required(VERSION 3.2.1...4.8.8) # use a version range

project(dz-x-platform VERSION 1.0.0)

add_library(lib-${PROJECT_NAME} SHARED src/app-lib.cpp)

find_package(DirectZ REQUIRED)

message(STATUS "DirectZ_LIBRARIES: ${DirectZ_LIBRARIES}")
message(STATUS "DirectZ_INCLUDE_DIRS: ${DirectZ_INCLUDE_DIRS}")

# include DirectZ globally as we're using in multiple targets, and, this should be the root CMakeLists
include_directories(lib-${PROJECT_NAME} PRIVATE ${DirectZ_INCLUDE_DIRS})

target_link_libraries(lib-${PROJECT_NAME} PRIVATE ${DirectZ_LIBRARIES})

if((WIN32 OR UNIX) AND NOT IOS AND NOT ANDROID)
    add_executable(${PROJECT_NAME} src/runtime.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE lib-${PROJECT_NAME})
elseif(ANDROID OR IOS)
    # we can link lib-${PROJECT_NAME} in an app loader and call native code
endif()