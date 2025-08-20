#include <dz/Compiler.hpp>

#include <fstream>

static std::filesystem::path path("so.cpp");
int main(int argc, char** argv) {
    {
        std::ofstream o(path, std::ios::out | std::ios::binary);
        o << std::string(R"(
bool get_bool(int n) {
    return n % 2 == 0;
}
)");
    }

    std::string out_so =
#if defined(_WIN32)
    "out.dll";
#else
    "out.so";
#endif


    dz::compiler::CompileSharedLibrary(path.string(), out_so);

    return 0;
}