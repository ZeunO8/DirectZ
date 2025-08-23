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

if(WIN32)
    set(_Vulkan_library_name vulkan-1)
    set(_Vulkan_hint_include_search_paths
            "$ENV{VULKAN_SDK}/include"
    )
    message(STATUS "About to check CMAKE_SIZEOF_VOID_P: ${CMAKE_SIZEOF_VOID_P}")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "CMAKE_SIZEOF_VOID_P: ${CMAKE_SIZEOF_VOID_P}")
        set(_Vulkan_hint_executable_search_paths
                "$ENV{VULKAN_SDK}/bin"
        )
        set(_Vulkan_hint_library_search_paths
                "$ENV{VULKAN_SDK}/lib"
                "$ENV{VULKAN_SDK}/bin"
        )
        message(STATUS "_Vulkan_hint_library_search_paths: ${_Vulkan_hint_library_search_paths}")
    else()
        set(_Vulkan_hint_executable_search_paths
                "$ENV{VULKAN_SDK}/bin32"
                "$ENV{VULKAN_SDK}/bin"
        )
        set(_Vulkan_hint_library_search_paths
                "$ENV{VULKAN_SDK}/lib32"
                "$ENV{VULKAN_SDK}/bin32"
                "$ENV{VULKAN_SDK}/lib"
                "$ENV{VULKAN_SDK}/bin"
        )
    endif()
else()
    set(_Vulkan_library_name vulkan)
    set(_Vulkan_hint_include_search_paths
            "$ENV{VULKAN_SDK}/include"
    )
    set(_Vulkan_hint_executable_search_paths
            "$ENV{VULKAN_SDK}/bin"
    )
    set(_Vulkan_hint_library_search_paths
            "$ENV{VULKAN_SDK}/lib"
    )
endif()

if(APPLE AND DEFINED Vulkan_Target_SDK)
    message(STATUS "is \"APPLE AND DEFINED Vulkan_Target_SDK\"")
    list(APPEND _Vulkan_hint_include_search_paths
            "${Vulkan_Target_SDK}/macOS/include"
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        list(APPEND _Vulkan_hint_library_search_paths
                "${Vulkan_Target_SDK}/iOS/lib"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        list(APPEND _Vulkan_hint_library_search_paths
                "${Vulkan_Target_SDK}/tvOS/lib"
        )
    else()
        list(APPEND _Vulkan_hint_library_search_paths
                "${Vulkan_Target_SDK}/lib"
        )
    endif()
endif()

cmake_policy(POP)