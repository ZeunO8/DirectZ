#pragma once
#ifdef THIS
#undef THIS
#endif
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/CommandLine.h>

#include <clang/Frontend/Utils.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>

#include <llvm/Config/llvm-config.h>
std::string triple = LLVM_DEFAULT_TARGET_TRIPLE;

#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <dz/Compiler.hpp>

namespace dz::compiler
{

    struct ASTResult
    {
        clang::ASTContext *context = nullptr;
    };

    class ASTBuilderVisitor : public clang::RecursiveASTVisitor<ASTBuilderVisitor>
    {
    public:
        ASTBuilderVisitor(clang::ASTContext &context, dz::compiler::AST &ast)
            : context(context), publicAST(ast)
        {
            publicAST.root = std::make_unique<dz::compiler::ASTNode>();
            publicAST.root->kind = "TranslationUnit";
        }

        bool VisitFunctionDecl(clang::FunctionDecl *decl)
        {
            auto node = std::make_unique<dz::compiler::ASTNode>();
            node->kind = "FunctionDecl";
            node->name = decl->getNameAsString();
            if (decl->getReturnType().getTypePtrOrNull())
                node->type = decl->getReturnType().getAsString();

            publicAST.root->children.push_back(std::move(node));
            return true;
        }

        bool VisitVarDecl(clang::VarDecl *decl)
        {
            auto node = std::make_unique<dz::compiler::ASTNode>();
            node->kind = "VarDecl";
            node->name = decl->getNameAsString();
            if (decl->getType().getTypePtrOrNull())
                node->type = decl->getType().getAsString();

            if (decl->hasInit())
            {
                clang::Expr *init = decl->getInit();
                llvm::StringRef initStr = clang::Lexer::getSourceText(
                    clang::CharSourceRange::getTokenRange(init->getSourceRange()),
                    context.getSourceManager(),
                    context.getLangOpts());
                node->value = initStr.str();
            }

            publicAST.root->children.push_back(std::move(node));
            return true;
        }

    private:
        clang::ASTContext &context;
        dz::compiler::AST &publicAST;
    };

    class CaptureASTConsumer : public clang::ASTConsumer
    {
    public:
        CaptureASTConsumer(ASTResult &result, AST &ast) : result(result), ast(ast) {}

        void HandleTranslationUnit(clang::ASTContext &Context) override
        {
            result.context = &Context;
            ASTBuilderVisitor visitor(Context, ast);
            visitor.TraverseDecl(Context.getTranslationUnitDecl());
        }

    private:
        ASTResult &result;
        AST &ast;
    };

    class CaptureASTAction : public clang::ASTFrontendAction
    {
    public:
        CaptureASTAction(ASTResult &result, AST &ast) : result(result), ast(ast) {}

        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef InFile) override
        {
            return std::make_unique<CaptureASTConsumer>(result, ast);
        }

    private:
        ASTResult &result;
        AST &ast;
    };

    class CaptureASTFactory : public clang::tooling::FrontendActionFactory
    {
    public:
        CaptureASTFactory(ASTResult &result, AST &ast) : result(result), ast(ast) {}

        std::unique_ptr<clang::FrontendAction> create() override
        {
            return std::make_unique<CaptureASTAction>(result, ast);
        }

    private:
        ASTResult &result;
        AST &ast;
    };

    dz::compiler::AST GenerateASTFromFile(const std::filesystem::path &filePath);
}