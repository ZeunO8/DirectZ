

set(DIRECTZ_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/DirectZ")

set(DirectZ_TGTS 
    DirectZ
    shaderc shaderc_util spirv-reflect-static
    glslang GenericCodeGen OSDependent MachineIndependent
    SPIRV SPIRV-Tools-static SPIRV-Tools-opt
    imgui)

foreach(TGT ${DirectZ_TGTS})
    set_target_properties(${TGT} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"      # Static libs (.a, .lib)
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"      # Shared libs (.so, .dylib)
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"      # Executables (.exe) or Windows DLLs

        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"

        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    )
endforeach()

# Export targets
install(TARGETS ${DirectZ_TGTS}
    EXPORT DirectZTargets
    # LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    # ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Core
)

foreach(TGT ${DirectZ_TGTS})
    if("${TGT}" STREQUAL "SPIRV-Tools-static")
        set(TGT "SPIRV-Tools")
    endif()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(OUT_DIR "${CMAKE_BINARY_DIR}/lib/Debug")
        set(POSTFIX "d")
    elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(OUT_DIR "${CMAKE_BINARY_DIR}/lib/Release")
    else()
        set(OUT_DIR "${CMAKE_BINARY_DIR}/lib")
    endif()
    set(LOCATION "${OUT_DIR}/${DZ_LIB_PREFIX}${TGT}${POSTFIX}${STATIC_DZ_LIB_SUFFIX}")
    list(APPEND TGT_LOCATIONS ${LOCATION})
endforeach()

install(FILES ${TGT_LOCATIONS}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Core
)

install(TARGETS dzp
    EXPORT DZPTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Core
)

# Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Core)
install(FILES
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imconfig.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Core)
install(FILES
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/backends
    COMPONENT Core)

# Configure Config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DirectZConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DirectZConfig.cmake"
    INSTALL_DESTINATION ${DIRECTZ_INSTALL_CMAKEDIR}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

# Optional version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DirectZConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

# Install config + version + targets
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DirectZConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DirectZConfigVersion.cmake"
    DESTINATION ${DIRECTZ_INSTALL_CMAKEDIR}
    COMPONENT Core
)

install(EXPORT DirectZTargets
    DESTINATION ${DIRECTZ_INSTALL_CMAKEDIR}
    COMPONENT Core
)

install(EXPORT DZPTargets
    DESTINATION ${DIRECTZ_INSTALL_CMAKEDIR}
    COMPONENT Core
)