

set(DIRECTZ_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/DirectZ")

# Export targets
install(TARGETS
    DirectZ
    shaderc shaderc_util spirv-reflect-static
    glslang GenericCodeGen OSDependent MachineIndependent
    SPIRV SPIRV-Tools-static SPIRV-Tools-opt
    imgui
    EXPORT DirectZTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Core
)

install(TARGETS dzp
    EXPORT DZPTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Core
)

# Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Core)

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
    COMPONENT CMakeConfig
)

install(EXPORT DirectZTargets
    NAMESPACE DirectZ::
    DESTINATION ${DIRECTZ_INSTALL_CMAKEDIR}
    COMPONENT CMakeConfig
)

install(EXPORT DZPTargets
    DESTINATION ${DIRECTZ_INSTALL_CMAKEDIR}
    COMPONENT CMakeConfig
)