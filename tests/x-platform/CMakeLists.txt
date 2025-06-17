cmake_minimum_required(VERSION 3.2.1...4.8.8) # use a version range

project(dz-x-platform VERSION 1.0.0)

add_library(lib-${PROJECT_NAME} SHARED src/app-lib.cpp)
target_compile_features(lib-${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(DirectZ REQUIRED)

message(STATUS "DirectZ_LIBRARIES: ${DirectZ_LIBRARIES}")
message(STATUS "DirectZ_INCLUDE_DIRS: ${DirectZ_INCLUDE_DIRS}")

target_include_directories(lib-${PROJECT_NAME} PRIVATE ${DirectZ_INCLUDE_DIRS})
target_link_libraries(lib-${PROJECT_NAME} PRIVATE ${DirectZ_LIBRARIES})

if((WIN32 OR UNIX) AND NOT IOS AND NOT ANDROID)
    add_executable(${PROJECT_NAME} src/runtime.cpp)
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    target_include_directories(${PROJECT_NAME} PRIVATE ${DirectZ_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE lib-${PROJECT_NAME} dxgi)
    # note we don't link DirectZ here to avoid copies

    # This eliminates duplicates of the DirectZ registry

    # if you need to interface with DirectZ functions from your runtime, do so via
    #  app-lib interfaces such that app-lib is the one calling DirectZ code

    # that said, a runtime should be very minimal, with the app-lib doing the main app logic
elseif(ANDROID OR IOS)
    # we can link lib-${PROJECT_NAME} in an app loader and call native code
endif()