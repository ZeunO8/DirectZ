#include "Clang.hpp"

static llvm::cl::OptionCategory ToolCategory("directz-clang options");


std::unique_ptr<dz::compiler::clang::ASTResult> dz::compiler::clang::GenerateASTFromFile(const std::filesystem::path &filePath) {
    auto result = std::make_unique<ASTResult>();

    // std::string fileStr = filePath.string();
    // std::vector<const char*> argv;
    // argv.push_back("directz-clang");
    // argv.push_back(fileStr.c_str());

    // int argc = static_cast<int>(argv.size());

    // auto ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv.data(), ToolCategory);
    // if (!ExpectedParser) {
    //     llvm::errs() << llvm::toString(ExpectedParser.takeError()) << "\n";
    //     return nullptr;
    // }

    // clang::tooling::CommonOptionsParser &OptionsParser = *ExpectedParser;
    // clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    // auto factory = clang::tooling::newFrontendActionFactory<CaptureASTAction>(*result);
    // if (Tool.run(factory.get()) != 0) {
    //     return nullptr;
    // }

    return result;
}