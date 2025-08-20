#include <dz/Compiler.hpp>
#include "Clang.hpp"
#include <iostream>

static llvm::cl::OptionCategory ToolCategory("directz-clang options");
dz::compiler::AST dz::compiler::GenerateASTFromFile(const std::filesystem::path &filePath)
{
    AST ast;
    ASTResult result;

    std::string fileStr = filePath.string();
    std::vector<const char *> argv{"directz-clang", fileStr.c_str()};
    int argc = static_cast<int>(argv.size());

    auto ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv.data(), ToolCategory);
    if (!ExpectedParser)
        throw std::runtime_error("Failed to parse command line options");

    auto &ParserRef = *ExpectedParser;
    clang::tooling::ClangTool Tool(ParserRef.getCompilations(), ParserRef.getSourcePathList());

    CaptureASTFactory factory(result, ast);
    if (Tool.run(&factory) != 0)
        throw std::runtime_error("Failed to run ClangTool");

    return ast;
}

dz::compiler::AST dz::compiler::GenerateAST(const std::filesystem::path &filePath)
{
    return dz::compiler::GenerateASTFromFile(filePath);
}

void dz::compiler::PrintAST(const dz::compiler::AST &ast, int indent)
{
    std::function<void(const dz::compiler::ASTNode *, int)> recurse =
        [&](const dz::compiler::ASTNode *node, int level)
    {
        if (!node)
            return;
        for (int i = 0; i < level; ++i)
            std::cout << "  ";
        std::cout << node->kind;
        if (!node->name.empty())
            std::cout << " name=" << node->name;
        if (!node->type.empty())
            std::cout << " type=" << node->type;
        if (!node->value.empty())
            std::cout << " value=" << node->value;
        std::cout << "\n";
        for (auto &child : node->children)
            recurse(child.get(), level + 1);
    };

    recurse(ast.root.get(), indent);
}

static std::vector<std::string> getPlatformCompileFlags(const std::string &triple, const std::string &outputLib)
{
    std::vector<std::string> flags;
    flags.push_back("-cc1");
    flags.push_back("-emit-obj");

    if (triple.find("windows-msvc") != std::string::npos)
    {
        flags.push_back("-D_WIN32");
        flags.push_back("-fms-compatibility");
        flags.push_back("-fms-extensions");
        flags.push_back("-o");
        flags.push_back(outputLib);
    }
    else if (triple.find("linux") != std::string::npos || triple.find("darwin") != std::string::npos)
    {
        flags.push_back("-fPIC");
        flags.push_back("-o");
        flags.push_back(outputLib);
    }
    else
    {
        llvm::errs() << "Unknown triple, defaulting to object output only.\n";
        flags.push_back("-o");
        flags.push_back(outputLib);
    }

    return flags;
}

bool dz::compiler::CompileSharedLibrary(const std::string &inputFile, const std::string &outputLib, const std::vector<std::string> &extraFlags)
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    clang::CompilerInstance compiler;
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS = llvm::vfs::getRealFileSystem();
    compiler.createDiagnostics(*VFS.get(), nullptr, false);

    std::shared_ptr<clang::TargetOptions> targetOpts = std::make_shared<clang::TargetOptions>();
    targetOpts->Triple = triple;
    compiler.setTarget(clang::TargetInfo::CreateTargetInfo(compiler.getDiagnostics(), *targetOpts.get()));

    std::vector<std::string> args = {inputFile};
    std::vector<std::string> platformFlags = getPlatformCompileFlags(targetOpts->Triple, outputLib);
    args.insert(args.end(), platformFlags.begin(), platformFlags.end());
    args.insert(args.end(), extraFlags.begin(), extraFlags.end());

    std::vector<const char *> cargs;
    for (auto &arg : args)
        cargs.push_back(arg.c_str());

    clang::CompilerInvocation &invocation = compiler.getInvocation();
    if (!clang::CompilerInvocation::CreateFromArgs(invocation, llvm::ArrayRef<const char *>(cargs.data(), cargs.size()), compiler.getDiagnostics()))
    {
        llvm::errs() << "Failed to initialize compiler invocation\n";
        return false;
    }

    clang::EmitObjAction action;
    if (!compiler.ExecuteAction(action))
    {
        llvm::errs() << "Failed to generate object file for shared library\n";
        return false;
    }

    llvm::outs() << "Generated object file for shared library: " << outputLib << "\n";
    return true;
}