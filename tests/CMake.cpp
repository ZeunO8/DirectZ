#include <dz/CMake.hpp>
#include <dz/Util.hpp>
#include <filesystem>

// #[=[
// #]=]
// #[=[
// #]=]

static std::filesystem::path path("CMakeLists.test.txt");
int main() {
    {
        std::ofstream o(path, std::ios::out | std::ios::binary);
        o << R"(
function(runforeach STEP)
    message("Running runforeach with STEP=${STEP}")

    set(Mega "4;8;12;18")
    foreach(u Mega)
        IF(NOT WIN32)
            message(STATUS "u&i&u: ${u}")
        else()
            message(STATUS "i: ${u}")
        endif()
    endforeach()

    message("foreach(I RANGE ${STEP}) ")
    foreach(I RANGE ${STEP})
        IF(WIN32)
            message(STATUS "IW: ${I}")
        ELSE()
            message(STATUS "IL: ${I}")
        Endif()
    endforeach()

    message("foreach(I RANGE 1 11 ${STEP}) ")
    foreach(I RANGE 1 11 ${STEP})
        message(STATUS "I: ${I}")
    endforeach()

    set(A 0;1)
    set(B 2 ${STEP})
#[===[
    set(D "9):")
    message(WARNING Same Thing)]====]

    message("foreach(X IN LISTS A B) ")
    foreach(X IN LISTS A B)
        message(STATUS "X=${X}")
    endforeach()

endfunction()

runforeach(4)
runforeach(3)
)";
//         o << std::string(R"(
// function(_get_component_with_prefix COMPONENT PREFIX)
//     set(OUT_COMPONENT "${COMPONENT}${PREFIX}" PARENT_SCOPE)
// endfunction()
// function(_set_component COMPONENT)
//     message(STATUS "SETTING COMPONENT: ${COMPONENT}")
//     set(${COMPONENT}_SET TRUE PARENT_SCOPE)
//     message(STATUS "DID SET COMPONENT: ${COMPONENT}")
//     _get_component_with_prefix(${COMPONENT} .lib)
//     message(STATUS "OUT_COMPONENT: ${OUT_COMPONENT}")
// endfunction()
// project(sup-te)
// _set_component(dz::function)
// )");
//         o << std::string(R"(
// list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/modules")

// if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
//     message(STATUS "Determined Host Windows")
//     set(Yo TRUE)
//     if(DEFINED Yo)
//         message(STATUS "Yo DEFINED")
//     else()
//         message(STATUS "Yo not DEFINED")
//     endif()
//     message(STATUS "End of Windows block")
// elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
//     message(STATUS "Determined Host Linux")
// elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
//     message(STATUS "Determined Host Darwin")
// elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
//     message(STATUS "Determined Host iOS")
// elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
//     message(STATUS "Determined Host Android")
// endif()

// project(test-project VERSION 3.2.1.0 COMPAT_VERSION 3.2 DESCRIPTION "This is a test project for DirectZ cmake" HOMEPAGE_URL "https://github.com/ZeunO8/DirectZ" LANGUAGES C CXX OBJC OBJCXX ASM)

// message(STATUS "WIN32: ${WIN32}")

// if(WIN32)
//     message(STATUS "Inside WIN32 if")
//     find_package(DZ_Vulkan REQUIRED COMPONENTS glslc)
//     message(STATUS "After WIN32 find_package(DZ_VULKAN) ")
// endif()

// add_library(test STATIC test.cpp)

// target_include_directories(test PRIVATE ${Vulkan_INCLUDE_DIR})

// target_link_libraries(test PRIVATE DirectZ "${Vulkan_LIBRARY}")
// )");
    }
    try {
        auto project = dz::cmake::CommandParser::parseFile(path.string());
        project->print();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}