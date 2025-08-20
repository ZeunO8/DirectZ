#pragma once

#include <memory>
#include <filesystem>

namespace dz::compiler {
    struct ASTNode {
        std::string kind;
        std::string name;
        std::string type;
        std::string value;
        std::vector<std::unique_ptr<ASTNode>> children;
    };

    struct AST {
        std::unique_ptr<ASTNode> root;
    };

    AST GenerateAST(const std::filesystem::path &filePath);

    void PrintAST(const AST& ast, int indent = 0);

    bool CompileSharedLibrary(const std::string& inputFile, const std::string& outputLib, const std::vector<std::string>& extraFlags = {});
}