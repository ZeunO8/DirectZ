#include <dz/CMake.hpp>
#include <dz/Util.hpp>
#include <filesystem>
#include <munit.h>

MunitResult test_parse_arguments(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
    function(SET_COMPONENT COMPONENT)
        cmake_parse_arguments(PARSE_ARGV 1 ""
                "BOOTS;CATS;OMEGA;TEA"
                "HUGE"
                "DEPENDENT_COMPONENTS")
        set(MY_OMEGA_DESCRIPTION "${COMPONENT}-BOOTS-${BOOTS}-CATS-${CATS}-${OMEGA}-${TEA}/${HUGE}|._.|${DEPENDENT_COMPONENTS}" PARENT_SCOPE)
    endfunction()
    SET_COMPONENT(my-component BOOTS TEA DEPENDENT_COMPONENTS CXX OBJCXX ASM HUGE "Is It Huge?" "Arg.:.")
    message(STATUS "${MY_OMEGA_DESCRIPTION}")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_function_nested_foreach(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
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

    message("foreach(X IN LISTS A B) ")
    foreach(X IN LISTS A B)
        message(STATUS "X=${X}")
    endforeach()
endfunction()

runforeach(4)
runforeach(3)
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_function_scope(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
function(_get_component_with_prefix COMPONENT PREFIX)
    set(OUT_COMPONENT "${COMPONENT}${PREFIX}" PARENT_SCOPE)
endfunction()
function(_set_component COMPONENT)
    message(STATUS "SETTING COMPONENT: ${COMPONENT}")
    set(${COMPONENT}_SET TRUE PARENT_SCOPE)
    message(STATUS "DID SET COMPONENT: ${COMPONENT}")
    _get_component_with_prefix(${COMPONENT} .lib)
    message(STATUS "OUT_COMPONENT: ${OUT_COMPONENT}")
endfunction()
project(sup-te)
_set_component(dz::function)
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_recursive_function_foreach(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
function(te_fun O S)
    message("Executing te_fun(${O} ${S}) ")
    function(recurse_fun_te W Z)
        message(STATUS "        W: ${W}, Z: ${Z}")
    endfunction()
    foreach(J RANGE ${O})
        message(STATUS "J == ${J}")
        foreach(I RANGE 2)
            if(I LESS 1)
                message(STATUS "    I == ${I}")
                foreach(K RANGE 1)
                    recurse_fun_te(${I} ${K})
                endforeach()
            else()
                message(STATUS "    I >= 1 (${I}) ")
            endif()
        endforeach()
    endforeach()
endfunction()
te_fun(0 "ZN")
te_fun(1 "RF")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_function_nested_ifs_foreachs(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
function(te_ret_function OKAY)
    if("${OKAY}" STREQUAL "Pizza and Peace")
        message(STATUS "Okay to print: ${OKAY}")
    else()
        message(WARNING "Not okay to print OKAY, returning early.")
        foreach(K RANGE 2)
            message(STATUS "K is ${K}")
            if (${K} EQUAL 13)
                message(STATUS "breaking K at 13")
                set(K ${K} PARENT_SCOPE)
                break()
            endif()
            foreach(J RANGE ${K})
                message(STATUS "J is ${J}")
                if (${J} EQUAL 2)
                    message(STATUS "Owning J at 2")
                    set(JK "${J}${K}" PARENT_SCOPE)
                    break()
                endif()
            endforeach()
            if (${JK} EQUAL 22)
                message(STATUS "breowning JK at 22")
                break()
            endif()
        endforeach()
        set(HOORAY FALSE PARENT_SCOPE)
        return()
    endif()
    message(STATUS "The non-final mesage")
    set(HOORAY TRUE PARENT_SCOPE)
endfunction()
te_ret_function("Pizza and Peace")
message(STATUS "And HOORAY is: ${HOORAY}")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_file_write_read(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists =
std::string("file(WRITE test_file_write_read.txt \"One Line\\nTwo Line\\n\" \"Three Line\\n\")") + R"(
file(STRINGS test_file_write_read.txt test_strings)
foreach(LINE IN LISTS test_strings)
    message(STATUS "${LINE}")
endforeach()

)" + std::string("file(APPEND test_file_write_read.txt \"Four Line\\n\" \"Five Line\\n\")") + R"(
file(STRINGS test_file_write_read.txt test_strings REGEX "Four")
foreach(LINE IN LISTS test_strings)
    message(STATUS "${LINE}")
endforeach()

file(READ test_file_write_read.txt test_data)
string(REGEX MATCH "Four" four_match ${test_data})
message(STATUS "four_match: ${four_match}")

string(REGEX MATCHALL "[A-Za-z]* Li" all_match ${test_data})
foreach(M IN LISTS all_match)
    message(STATUS "M: ${M}")
endforeach()
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_find_package_DirectZ(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/modules")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    message(STATUS "Determined Host Windows")
    set(Yo TRUE)
    if(DEFINED Yo)
        message(STATUS "Yo DEFINED")
    else()
        message(STATUS "Yo not DEFINED")
    endif()
    message(STATUS "End of Windows block")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    message(STATUS "Determined Host Linux")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    message(STATUS "Determined Host Darwin")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    message(STATUS "Determined Host iOS")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    message(STATUS "Determined Host Android")
endif()

project(test-project VERSION 3.2.1.0 COMPAT_VERSION 3.2 DESCRIPTION "This is a test project for DirectZ cmake" HOMEPAGE_URL "https://github.com/ZeunO8/DirectZ" LANGUAGES C CXX OBJC OBJCXX ASM)

message(STATUS "WIN32: ${WIN32}")

if(WIN32)
    message(STATUS "Inside WIN32 if")
    find_package(DZ_Vulkan REQUIRED COMPONENTS glslc)
    message(STATUS "After WIN32 find_package(DZ_VULKAN) ")
endif()

add_library(test STATIC test.cpp)

target_include_directories(test PRIVATE ${Vulkan_INCLUDE_DIR})

target_link_libraries(test PRIVATE DirectZ "${Vulkan_LIBRARY}")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_part_vulkan(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
set(Vulkan_INCLUDE_DIR "C:/VulkanSDK/1.4.313.2/Include")
if(Vulkan_INCLUDE_DIR)
    message(STATUS "Vulkan_INCLUDE_DIR DEFINED")
    set(VULKAN_CORE_H ${Vulkan_INCLUDE_DIR}/vulkan/vulkan_core.h)
    if(EXISTS ${VULKAN_CORE_H})
        message(STATUS "VULKAN_CORE_H: ${VULKAN_CORE_H} EXISTS")
        file(STRINGS  ${VULKAN_CORE_H} VulkanHeaderVersionLine REGEX "^#define VK_HEADER_VERSION ")
        string(REGEX MATCHALL "[0-9]+" VulkanHeaderVersion "${VulkanHeaderVersionLine}")
        message(STATUS "VulkanHeaderVersion: ${VulkanHeaderVersion}")
        file(STRINGS  ${VULKAN_CORE_H} VulkanHeaderVersionLine2 REGEX "^#define VK_HEADER_VERSION_COMPLETE ")
        string(REGEX MATCHALL "[0-9]+" VulkanHeaderVersion2 "${VulkanHeaderVersionLine2}")
        message(STATUS "VulkanHeaderVersion2: ${VulkanHeaderVersion2}")
        list(LENGTH VulkanHeaderVersion2 _len)
        message(STATUS "_len: ${_len}")
        #  versions >= 1.2.175 have an additional numbers in front of e.g. '0, 1, 2' instead of '1, 2'
        if(_len EQUAL 3)
            list(REMOVE_AT VulkanHeaderVersion2 0)
        endif()
        list(APPEND VulkanHeaderVersion2 ${VulkanHeaderVersion})
        message(STATUS "VulkanHeaderVersion2: ${VulkanHeaderVersion2}")
        list(JOIN VulkanHeaderVersion2 "." Vulkan_VERSION)
        message(STATUS "Vulkan_VERSION: ${Vulkan_VERSION}")
    endif()
endif()
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_macro(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
macro(_FPHSA_FAILURE_MESSAGE _msg)
  set(__msg "${_msg}")
  if(FPHSA_REASON_FAILURE_MESSAGE)
    string(APPEND __msg "\n    Reason given by package: ${FPHSA_REASON_FAILURE_MESSAGE}\n")
  elseif(NOT DEFINED PROJECT_NAME AND NOT CMAKE_SCRIPT_MODE_FILE)
    string(APPEND __msg "\n"
      "Hint: The project() command has not yet been called.  It sets up system-specific search paths.")
  else()
    string(APPEND __msg "\n" "I'm a message")
  endif()
  if(${_NAME}_FIND_REQUIRED)
    message(FATAL_ERROR "${__msg}")
  else()
    if(NOT ${_NAME}_FIND_QUIETLY)
      message(STATUS "${__msg}")
    endif()
  endif()
endmacro()
_FPHSA_FAILURE_MESSAGE("I'm a Failure?")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_foreach_zip_lists(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
set(A "1;3;5;7")
set(B "2;4")
foreach(x y IN ZIP_LISTS A B)
    message(STATUS "x: ${x}, y: ${y}")
endforeach()
set(A "1;3")
set(B "2;4;5")
foreach(z IN ZIP_LISTS A B)
    message(STATUS "z_0: ${z_0}, z_1: ${z_1}")
endforeach()
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_list_append_prepend(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
set(A "1;3;5;7")
message(STATUS "A Before APPEND: ${A}")
list(APPEND A "9")
message(STATUS "A Before PREPEND: ${A}")
list(PREPEND A "-1")
message(STATUS "A After PREPEND: ${A}")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_include(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
include(FindPackageHandleStandardArgs)
set(Vulkan_LIBRARY "vulkan-1.lib")
set(Vulkan_INCLUDE_DIR "C:/VulkanSDK/1.4.313.2/Include")
set(Vulkan_VERSION "1.4.313")
set(VERSION_VAR ${Vulkan_VERSION})
set(HANDLE_COMPONENTS FALSE)
set(CMAKE_FIND_PACKAGE_NAME Vulkan)
find_package_handle_standard_args(Vulkan
        REQUIRED_VARS
        Vulkan_LIBRARY
        Vulkan_INCLUDE_DIR
        VERSION_VAR
        Vulkan_VERSION
        HANDLE_COMPONENTS
)
message(STATUS "Vulkan_FOUND: ${Vulkan_FOUND}")
message(STATUS "VULKAN_FOUND: ${VULKAN_FOUND}")
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

MunitResult test_function_argx(const MunitParameter[], void *user_data)
{
    std::cout << std::endl;
    static std::string cmakelists = R"(
function(my-function X Y)
    message(STATUS "X: ${X}, Y: ${Y}")
    message(STATUS "ARGC: ${ARGC}")
    message(STATUS "ARGV: ${ARGV}")
    message(STATUS "ARGN: ${ARGN}")
    foreach(I RANGE ${ARGC})
        message(STATUS "ARGV${I}: ${ARGV${I}}")
    endforeach()
endfunction()
my-function(1 2 3)
)";
    try {
        auto project = dz::cmake::CommandParser::parseContent(cmakelists);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}


static MunitTest test_suite_tests[] = {
    // { (char*)"/cmake_parse_arguments", test_parse_arguments, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    // { (char*)"/function_nested_foreach", test_function_nested_foreach, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    // { (char*)"/function_scope", test_function_scope, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    // { (char*)"/recursive_function", test_recursive_function_foreach, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    // { (char*)"/function_nested_logic", test_function_nested_ifs_foreachs, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
    // { (char*)"/file_write_read", test_file_write_read, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // { (char*)"/part_vulkan", test_part_vulkan, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // { (char*)"/macro", test_macro, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // { (char*)"/foreach_zip_lists", test_foreach_zip_lists, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // { (char*)"/list_append_prepend", test_list_append_prepend, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    { (char*)"/include", test_include, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // { (char*)"/function_argx", test_function_argx, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // { (char*)"/find_package_DirectZ", test_find_package_DirectZ, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },

    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {
    (char *)"/CMake.cpp",
    test_suite_tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&test_suite, (void *)"dz", argc, argv);
}