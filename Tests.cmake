include(CTest)
function(add_dz_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_include_directories(${TEST_NAME} PRIVATE ${DIRECTZ_INCLUDE_DIRS})
    target_link_libraries(${TEST_NAME} PRIVATE DirectZ zlibstatic)
    if(ANDROID)
        target_link_libraries(${TEST_NAME} PRIVATE android log)
    elseif(IOS)
        target_link_libraries(${TEST_NAME} PRIVATE
            "-framework UIKit"
            "-framework Foundation"
            "-framework QuartzCore"
            "-framework Metal"
        )
    elseif(MACOS)
        target_link_libraries(${TEST_NAME} PRIVATE "-framework Cocoa" "-framework QuartzCore" "-framework Metal")
    endif()
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TEST_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"      # Static libs (.a, .lib)
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"      # Shared libs (.so, .dylib)
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"      # Executables (.exe) or Windows DLLs

        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"

        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    )
    add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE_DIR:${TEST_NAME}>/${TEST_NAME}${TEST_EXT})
endfunction()
add_dz_test(DZ_ShaderReflect tests/ShaderReflect.cpp)
add_dz_test(DZ_Particle2D tests/Particle2D.cpp)
add_dz_test(DZ_AssetStream tests/AssetStream.cpp)
add_dz_test(DZ_LineGrid tests/LineGrid.cpp)
add_dz_test(DZ_D7Stream tests/D7Stream.cpp)
add_dz_test(DZ_ImGuiTest tests/ImGui.cpp)
add_dz_test(DZ_ECSTest tests/ECS.cpp)
file(COPY images/Suzuho-Ueda.bmp DESTINATION ${CMAKE_BINARY_DIR}/images)
file(COPY images/hi.bmp DESTINATION ${CMAKE_BINARY_DIR}/images)
file(COPY models/SaiyanOne.glb DESTINATION ${CMAKE_BINARY_DIR}/models)
file(COPY models/GoldenSportsCar.glb DESTINATION ${CMAKE_BINARY_DIR}/models)
file(COPY hdri/autumn_field_puresky_4k.hdr DESTINATION ${CMAKE_BINARY_DIR}/hdri)
file(COPY hdri/zwartkops_straight_afternoon_4k.hdr DESTINATION ${CMAKE_BINARY_DIR}/hdri)