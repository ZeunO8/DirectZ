include(CTest)
function(add_dz_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_include_directories(${TEST_NAME} PRIVATE
        include
        ${Vulkan_INCLUDE_DIRS}
        ${imgui_SOURCE_DIR}
	    ${imguizmo_SOURCE_DIR}
	    ${iostreams_SOURCE_DIR}/include
    )
    target_link_libraries(${TEST_NAME} PRIVATE DirectZ)
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
    add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE_DIR:${TEST_NAME}>/${TEST_NAME}${TEST_EXT})
endfunction()
add_dz_test(DZ_ShaderReflect tests/ShaderReflect.cpp)
add_dz_test(DZ_Particle2D tests/Particle2D.cpp)
add_dz_test(DZ_AssetStream tests/AssetStream.cpp)
add_dz_test(DZ_LineGrid tests/LineGrid.cpp)
add_dz_test(DZ_D7Stream tests/D7Stream.cpp)
add_dz_test(DZ_ImGuiTest tests/ImGui.cpp)
add_dz_test(DZ_ECSTest tests/ECS.cpp)