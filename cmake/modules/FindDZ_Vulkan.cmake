cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)

if("FATAL_ERROR" IN_LIST Vulkan_FIND_COMPONENTS)
    message(AUTHOR_WARNING
            "Ignoring unknown component 'FATAL_ERROR'.\n"
            "The find_package() command documents no such argument."
    )
    list(REMOVE_ITEM Vulkan_FIND_COMPONENTS "FATAL_ERROR")
endif()

message(STATUS "About to check 'DEFINED CMAKE_FIND_FRAMEWORK'")
if(DEFINED CMAKE_FIND_FRAMEWORK)
    set(_Vulkan_saved_cmake_find_framework ${CMAKE_FIND_FRAMEWORK})
    set(CMAKE_FIND_FRAMEWORK FIRST)
endif()

message(STATUS "About to check IOS")
if(IOS)
    message(STATUS "An iOS host")
    get_filename_component(Vulkan_Target_SDK "$ENV{VULKAN_SDK}/.." REALPATH)
    list(APPEND CMAKE_FRAMEWORK_PATH "${Vulkan_Target_SDK}/iOS/lib")
    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
else()
    message(STATUS "Not an iOS host")
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

find_path(Vulkan_INCLUDE_DIR
        NAMES vulkan/vulkan.h
        HINTS
        ${_Vulkan_hint_include_search_paths}
)
mark_as_advanced(Vulkan_INCLUDE_DIR)

message(STATUS "Vulkan_INCLUDE_DIR: ${Vulkan_INCLUDE_DIR}")


find_library(Vulkan_LIBRARY
        NAMES ${_Vulkan_library_name}
        HINTS
        ${_Vulkan_hint_library_search_paths}
)
message(STATUS "${_Vulkan_library_name} ${Vulkan_LIBRARY} search paths ${_Vulkan_hint_library_search_paths}")
mark_as_advanced(Vulkan_LIBRARY)

find_library(Vulkan_Layer_API_DUMP
        NAMES VkLayer_api_dump
        HINTS
        ${_Vulkan_hint_library_search_paths}
)
message(STATUS "VkLayer_api_dump ${Vulkan_Layer_API_DUMP}")
mark_as_advanced(Vulkan_Layer_API_DUMP)

find_library(Vulkan_Layer_SHADER_OBJECT
        NAMES VkLayer_khronos_shader_object
        HINTS
        ${_Vulkan_hint_library_search_paths}
)
mark_as_advanced(VkLayer_khronos_shader_object)

find_library(Vulkan_Layer_SYNC2
        NAMES VkLayer_khronos_synchronization2
        HINTS
        ${_Vulkan_hint_library_search_paths}
)
mark_as_advanced(Vulkan_Layer_SYNC2)

find_library(Vulkan_Layer_VALIDATION
        NAMES VkLayer_khronos_validation
        HINTS
        ${_Vulkan_hint_library_search_paths}
)
mark_as_advanced(Vulkan_Layer_VALIDATION)


if(glslc IN_LIST Vulkan_FIND_COMPONENTS)
    find_program(Vulkan_GLSLC_EXECUTABLE
            NAMES glslc
            HINTS
            ${_Vulkan_hint_executable_search_paths}
    )
    mark_as_advanced(Vulkan_GLSLC_EXECUTABLE)
endif()
if(glslangValidator IN_LIST Vulkan_FIND_COMPONENTS)
    find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE
            NAMES glslangValidator
            HINTS
            ${_Vulkan_hint_executable_search_paths}
    )
    mark_as_advanced(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
endif()
if(glslang IN_LIST Vulkan_FIND_COMPONENTS)
    find_library(Vulkan_glslang-spirv_LIBRARY
            NAMES SPIRV
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-spirv_LIBRARY)

    find_library(Vulkan_glslang-spirv_DEBUG_LIBRARY
            NAMES SPIRVd
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-spirv_DEBUG_LIBRARY)

    find_library(Vulkan_glslang-oglcompiler_LIBRARY
            NAMES OGLCompiler
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-oglcompiler_LIBRARY)

    find_library(Vulkan_glslang-oglcompiler_DEBUG_LIBRARY
            NAMES OGLCompilerd
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-oglcompiler_DEBUG_LIBRARY)

    find_library(Vulkan_glslang-osdependent_LIBRARY
            NAMES OSDependent
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-osdependent_LIBRARY)

    find_library(Vulkan_glslang-osdependent_DEBUG_LIBRARY
            NAMES OSDependentd
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-osdependent_DEBUG_LIBRARY)

    find_library(Vulkan_glslang-machineindependent_LIBRARY
            NAMES MachineIndependent
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-machineindependent_LIBRARY)

    find_library(Vulkan_glslang-machineindependent_DEBUG_LIBRARY
            NAMES MachineIndependentd
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-machineindependent_DEBUG_LIBRARY)

    find_library(Vulkan_glslang-genericcodegen_LIBRARY
            NAMES GenericCodeGen
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-genericcodegen_LIBRARY)

    find_library(Vulkan_glslang-genericcodegen_DEBUG_LIBRARY
            NAMES GenericCodeGend
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang-genericcodegen_DEBUG_LIBRARY)

    find_library(Vulkan_glslang_LIBRARY
            NAMES glslang
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang_LIBRARY)

    find_library(Vulkan_glslang_DEBUG_LIBRARY
            NAMES glslangd
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_glslang_DEBUG_LIBRARY)
endif()
if(shaderc_combined IN_LIST Vulkan_FIND_COMPONENTS)
    find_library(Vulkan_shaderc_combined_LIBRARY
            NAMES shaderc_combined
            HINTS
            ${_Vulkan_hint_library_search_paths})
    mark_as_advanced(Vulkan_shaderc_combined_LIBRARY)

    find_library(Vulkan_shaderc_combined_DEBUG_LIBRARY
            NAMES shaderc_combinedd
            HINTS
            ${_Vulkan_hint_library_search_paths})
    mark_as_advanced(Vulkan_shaderc_combined_DEBUG_LIBRARY)
endif()
if(SPIRV-Tools IN_LIST Vulkan_FIND_COMPONENTS)
    find_library(Vulkan_SPIRV-Tools_LIBRARY
            NAMES SPIRV-Tools
            HINTS
            ${_Vulkan_hint_library_search_paths})
    mark_as_advanced(Vulkan_SPIRV-Tools_LIBRARY)

    find_library(Vulkan_SPIRV-Tools_DEBUG_LIBRARY
            NAMES SPIRV-Toolsd
            HINTS
            ${_Vulkan_hint_library_search_paths})
    mark_as_advanced(Vulkan_SPIRV-Tools_DEBUG_LIBRARY)
endif()
if(MoltenVK IN_LIST Vulkan_FIND_COMPONENTS)
    # CMake has a bug in 3.28 that doesn't handle xcframeworks.  Do it by hand for now.
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        if(CMAKE_VERSION VERSION_LESS 3.29)
            set( _Vulkan_hint_library_search_paths ${Vulkan_Target_SDK}/ios/lib/MoltenVK.xcframework/ios-arm64)
        else ()
            set( _Vulkan_hint_library_search_paths ${Vulkan_Target_SDK}/ios/lib/)
        endif ()
    endif ()
    find_library(Vulkan_MoltenVK_LIBRARY
            NAMES MoltenVK
            NO_DEFAULT_PATH
            HINTS
            ${_Vulkan_hint_library_search_paths}
    )
    mark_as_advanced(Vulkan_MoltenVK_LIBRARY)

    find_path(Vulkan_MoltenVK_INCLUDE_DIR
            NAMES MoltenVK/mvk_vulkan.h
            HINTS
            ${_Vulkan_hint_include_search_paths}
    )
    mark_as_advanced(Vulkan_MoltenVK_INCLUDE_DIR)
endif()
if(volk IN_LIST Vulkan_FIND_COMPONENTS)
    find_library(Vulkan_volk_LIBRARY
            NAMES volk
            HINTS
            ${_Vulkan_hint_library_search_paths})
    mark_as_advanced(Vulkan_Volk_LIBRARY)
endif()

if (dxc IN_LIST Vulkan_FIND_COMPONENTS)
    find_library(Vulkan_dxc_LIBRARY
            NAMES dxcompiler
            HINTS
            ${_Vulkan_hint_library_search_paths})
    mark_as_advanced(Vulkan_dxc_LIBRARY)

    find_program(Vulkan_dxc_EXECUTABLE
            NAMES dxc
            HINTS
            ${_Vulkan_hint_executable_search_paths})
    mark_as_advanced(Vulkan_dxc_EXECUTABLE)
endif()

if(DEFINED _Vulkan_saved_cmake_find_framework)
    set(CMAKE_FIND_FRAMEWORK ${_Vulkan_saved_cmake_find_framework})
    unset(_Vulkan_saved_cmake_find_framework)
endif()

if(Vulkan_GLSLC_EXECUTABLE)
    set(Vulkan_glslc_FOUND TRUE)
else()
    set(Vulkan_glslc_FOUND FALSE)
endif()

if(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    set(Vulkan_glslangValidator_FOUND TRUE)
else()
    set(Vulkan_glslangValidator_FOUND FALSE)
endif()

if (Vulkan_dxc_EXECUTABLE)
    set(Vulkan_dxc_exe_FOUND TRUE)
else()
    set(Vulkan_dxc_exe_FOUND FALSE)
endif()

function(_Vulkan_set_library_component_found component)
    cmake_parse_arguments(PARSE_ARGV 1 _ARG
            "NO_WARNING"
            ""
            "DEPENDENT_COMPONENTS")

    set(all_dependent_component_found TRUE)
    foreach(dependent_component IN LISTS _ARG_DEPENDENT_COMPONENTS)
        if(NOT Vulkan_${dependent_component}_FOUND)
            set(all_dependent_component_found FALSE)
            break()
        endif()
    endforeach()

    if(all_dependent_component_found AND (Vulkan_${component}_LIBRARY OR Vulkan_${component}_DEBUG_LIBRARY))
        set(Vulkan_${component}_FOUND TRUE PARENT_SCOPE)

        # For Windows Vulkan SDK, third party tools binaries are provided with different MSVC ABI:
        #   - Release binaries uses a runtime library
        #   - Debug binaries uses a debug runtime library
        # This lead to incompatibilities in linking for some configuration types due to CMake-default or project-configured selected MSVC ABI.
        if(WIN32 AND NOT _ARG_NO_WARNING)
            if(NOT Vulkan_${component}_LIBRARY)
                message(WARNING
                        "Library ${component} for Release configuration is missing, imported target Vulkan::${component} may not be able to link when targeting this build configuration due to incompatible MSVC ABI.")
            endif()
            if(NOT Vulkan_${component}_DEBUG_LIBRARY)
                message(WARNING
                        "Library ${component} for Debug configuration is missing, imported target Vulkan::${component} may not be able to link when targeting this build configuration due to incompatible MSVC ABI. Consider re-installing the Vulkan SDK and request debug libraries to fix this warning.")
            endif()
        endif()
    else()
        set(Vulkan_${component}_FOUND FALSE PARENT_SCOPE)
    endif()
endfunction()

_Vulkan_set_library_component_found(glslang-spirv NO_WARNING)
_Vulkan_set_library_component_found(glslang-oglcompiler NO_WARNING)
_Vulkan_set_library_component_found(glslang-osdependent NO_WARNING)
_Vulkan_set_library_component_found(glslang-machineindependent NO_WARNING)
_Vulkan_set_library_component_found(glslang-genericcodegen NO_WARNING)
_Vulkan_set_library_component_found(glslang
        DEPENDENT_COMPONENTS
        glslang-spirv
        glslang-oglcompiler
        glslang-osdependent
        glslang-machineindependent
        glslang-genericcodegen)
_Vulkan_set_library_component_found(shaderc_combined)
_Vulkan_set_library_component_found(SPIRV-Tools)
_Vulkan_set_library_component_found(volk)
_Vulkan_set_library_component_found(dxc)

if(Vulkan_MoltenVK_INCLUDE_DIR AND Vulkan_MoltenVK_LIBRARY)
    set(Vulkan_MoltenVK_FOUND TRUE)
else()
    set(Vulkan_MoltenVK_FOUND FALSE)
endif()

set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})
set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})

cmake_policy(POP)