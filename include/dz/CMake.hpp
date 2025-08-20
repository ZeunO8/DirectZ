#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <sstream>
#include <stack>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <filesystem>
#include <functional>

namespace dz::cmake
{
    struct Project;

    struct Command
    {
        std::string name;
        std::vector<std::string> arguments;
    };

    struct ICMakeEntity
    {
        virtual ~ICMakeEntity() = default;
        virtual std::string getName() const = 0;
        virtual void addArgument(const std::string &arg) = 0;
        virtual const std::vector<std::string> &getArguments() const = 0;
    };

    enum class CMakeMessageType
    {
        UNSET,
        STATUS,
        WARNING,
        FATAL_ERROR
    };

    struct Target : public ICMakeEntity
    {
        enum class Type
        {
            Library,
            Executable
        };
        enum class LinkType
        {
            Shared,
            Static,
        };
        Target(Type t, const std::string &n) : targetType(t), name(n) {}
        std::string getName() const override { return name; }
        void addArgument(const std::string &arg) override { sources.push_back(arg); }
        const std::vector<std::string> &getArguments() const override { return sources; }
        void addIncludeDir(const std::string &dir) { includeDirs.push_back(dir); }
        void addLinkLib(const std::string &lib) { linkLibs.push_back(lib); }
        void setShared() { linkType = LinkType::Shared; }
        void setStatic() { linkType = LinkType::Static; }

        Type targetType;
        LinkType linkType;
        std::string name;
        std::vector<std::string> sources;
        std::vector<std::string> includeDirs;
        std::vector<std::string> linkLibs;
    };

    struct Macro
    {
        std::string name;
        std::vector<std::string> params;
        std::vector<dz::cmake::Command> body;

        void execute(Project &project, const Command &cmd, std::unordered_map<std::string, std::string> &vars) const;
    };

    struct Project
    {
        std::string name;
        std::unordered_map<std::string, std::shared_ptr<Target>> targets;
        std::unordered_map<std::string, Macro> macros;
        
        Project();

        void addCommand(const Command &cmd, std::unordered_map<std::string, std::string> &vars);
        
    private:

        using DSL_Fn = std::function<void(size_t, const Command&, std::unordered_map<std::string, std::string>&)>;
        using DSL_Map = std::unordered_map<std::string, DSL_Fn>;

        DSL_Map dsl_map;

        DSL_Map generate_dsl_map();

    private:

        

        std::string determineFindPackageDir(const std::string &pkg, const std::unordered_map<std::string, std::string> &vars);

        void add_lib_or_exe(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);

        void target_include_directories(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);

        void target_link_libraries(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);

        void find_package(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);

        void message(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);

        void get_filename_component(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);

        void project(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars);
    };

    enum class ConditionOp
    {
        And,
        Or,
        Not,
        Identifier,
        Literal,
        Group
    };

    struct ConditionNode
    {
        ConditionOp op;
        std::string value;
        std::vector<std::shared_ptr<ConditionNode>> children;

        bool BoolVar(const std::string &var) const;

        bool Evaluate(std::unordered_map<std::string, std::string> &vars) const;
    };

    struct ConditionalBlock
    {
        std::vector<std::pair<std::shared_ptr<ConditionNode>, std::vector<Command>>> branches;
        std::vector<Command> elseBranch;

        void EvaluateAndAdd(Project &proj, std::unordered_map<std::string, std::string> &vars);
    };

    struct CommandParser
    {
        static Project parseFile(const std::string &path, std::unordered_map<std::string, std::string> &env);

        static void parseContentWithProject(Project &project, const std::string &content, std::unordered_map<std::string, std::string> &env);

        static Project parseContent(const std::string &content, std::unordered_map<std::string, std::string> &env);

    private:
        static std::shared_ptr<ConditionNode> parseCondition(const std::string &expr);

        static void skipWhitespaceAndComments(const std::string &s, size_t &pos);

        static void trim(std::string &s);

        static void tokenize(const std::string &s, std::vector<std::string> &out);

        static void varize(Command &cmd, const std::unordered_map<std::string, std::string> &env);

        static size_t findMatchingParen(const std::string &s, size_t open);
    };

}