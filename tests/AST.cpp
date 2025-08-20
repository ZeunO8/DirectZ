#include <dz/Compiler.hpp>

#include <fstream>

static std::string npathstr = "print_argvc.cpp";
int main(int argc, char** argv) {
    bool newdargv = false;
    if (argc == 1) {
        char** nargv = (char**)malloc(2 * sizeof(char*));
        nargv[0] = argv[0];
        nargv[1] = npathstr.data();
        newdargv = true;
        argv = nargv;
    }
    std::filesystem::path path(argv[1]);

    {
        std::ofstream o(path, std::ios::out | std::ios::binary);
        o << std::string(R"(
// #include <dz/Compiler.hpp>            
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "argc: " << argc << std::endl;
    for (auto j = 0; j < argc; j++) {
        std::cout << "argv[" << j << "]: " <<argv[j] << std::endl;
    }
    return 0;
}
)");
    }

    auto result = dz::compiler::GenerateAST(path);

    if (newdargv) {
        free(argv);
    }

    dz::compiler::PrintAST(result);

    return 0;
}