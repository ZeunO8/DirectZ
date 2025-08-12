#pragma once
#ifdef THIS
#undef THIS
#endif
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/CommandLine.h>
namespace dz::compiler::clang {
    bool DoAClangThing(int argc, const char** argv);
}