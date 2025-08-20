#include <dz/CMake.hpp>
#include <filesystem>

static std::filesystem::path path("CMakeLists.test.txt");
int main() {
    {
        std::ofstream o(path, std::ios::out | std::ios::binary);
        o << std::string(R"(
project(test-project)

message(STATUS "WIN32: ${WIN32}")

find_package(DirectZ REQUIRED)

add_library(test STATIC test.cpp)
target_link_libraries(test PRIVATE DirectZ)
if(WIN32)
    target_link_libraries(test PRIVATE DirectZ-win)
elseif(ANDROID)
    target_link_libraries(test PRIVATE DirectZ-android)
endif()
)");
    }
    std::unordered_map<std::string, std::string> env = {
        {"WIN32", "TRUE"}
    };
    try {
        auto project = dz::cmake::CommandParser::parseFile(path.string(), env);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}