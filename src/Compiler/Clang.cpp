#include "Clang.hpp"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory ToolCategory("directz-clang options");

bool dz::compiler::clang::DoAClangThing(int argc, const char** argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!ExpectedParser)
    {
        // Print error if parsing failed
        llvm::errs() << toString(ExpectedParser.takeError()) << "\n";
        return false;
    }

    CommonOptionsParser &OptionsParser = *ExpectedParser;
    ClangTool Tool(OptionsParser.getCompilations(),
                   OptionsParser.getSourcePathList());
    return true;
};