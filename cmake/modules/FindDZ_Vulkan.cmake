message(STATUS "Upon Woke, Touchedj.")

cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)

if("FATAL_ERROR" IN_LIST Vulkan_FIND_COMPONENTS)
    message(AUTHOR_WARNING
            "Ignoring unknown component 'FATAL_ERROR'.\n"
            "The find_package() command documents no such argument."
    )
    list(REMOVE_ITEM Vulkan_FIND_COMPONENTS "FATAL_ERROR")
endif()

if(DEFINED CMAKE_FIND_FRAMEWORK)
    set(_Vulkan_saved_cmake_find_framework ${CMAKE_FIND_FRAMEWORK})
    set(CMAKE_FIND_FRAMEWORK FIRST)
endif()

if(IOS)
    message(STATUS "An iOS host")
    get_filename_component(Vulkan_Target_SDK "$ENV{VULKAN_SDK}/.." REALPATH)
    list(APPEND CMAKE_FRAMEWORK_PATH "${Vulkan_Target_SDK}/iOS/lib")
    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endif()

if(NOT glslc IN_LIST Vulkan_FIND_COMPONENTS)
    list(APPEND Vulkan_FIND_COMPONENTS glslc)
    message(STATUS "Appended glslc to Vulkan_FIND_COMPONENTS")
endif()
if(NOT glslangValidator IN_LIST Vulkan_FIND_COMPONENTS)
    list(APPEND Vulkan_FIND_COMPONENTS glslangValidator)
    message(STATUS "Appended glslangValidator to Vulkan_FIND_COMPONENTS")
endif()

cmake_policy(POP)