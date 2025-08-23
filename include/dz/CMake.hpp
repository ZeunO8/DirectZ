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
#include <deque>

namespace dz::cmake
{
    struct Project;

    struct Evaluable
    {
        virtual ~Evaluable() = default;
        virtual void Evaluate(Project& project) = 0;
        virtual void Varize(Project& project) = 0;
    };

    struct Command : Evaluable
    {
        std::string name;
        std::vector<std::string> arguments;
        void Evaluate(Project& project) override;
        void Varize(Project& project) override;
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
        FATAL_ERROR,
        AUTHOR_WARNING,
    };

    struct Target : public ICMakeEntity
    {
        enum class Type
        {
            Library,
            Executable,
            Imported
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

        std::string GetTypeStr()
        {
            switch (targetType)
            {
            case Type::Library:
                return "Library";
            case Type::Executable:
                return "Executable";
            case Type::Imported:
                return "Imported";
            }
            return "<unknown>";
        }

        std::string GetLinkTypeStr()
        {
            switch (linkType)
            {
            case LinkType::Shared:
                return "Shared";
            case LinkType::Static:
                return "Static";
            }
            return "<unknown>";
        }

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

        void execute(Project &project, const Command &cmd) const;
    };

    struct Policy
    {
        std::string policy_name;
        std::string policy_value;
    };

    using CMakeVariableMap = std::unordered_map<std::string, std::string>;

    struct ParseContext
    {
        std::shared_ptr<Project> root_project;
        std::unordered_map<std::string, std::shared_ptr<Policy>> policy_set_map;
        std::deque<std::shared_ptr<Policy>> policy_stack;
        bool policy_push_just_called = false;

        CMakeVariableMap vars;
        CMakeVariableMap env;

        ParseContext();

        static CMakeVariableMap generate_default_system_vars_map();
        static CMakeVariableMap get_env_map();
    };

    struct Project
    {
        using DSL_Fn = std::function<void(size_t, const Command &)>;
        using DSL_Map = std::unordered_map<std::string, DSL_Fn>;

        std::string name;
        std::unordered_map<std::string, std::shared_ptr<Target>> targets;
        std::unordered_map<std::string, Macro> macros;

        std::shared_ptr<ParseContext> context_sh_ptr;

        DSL_Map dsl_map;

        Project(const std::shared_ptr<ParseContext> &context_sh_ptr);

        void addCommand(const Command &cmd);

        void print();

    private:

        DSL_Map generate_dsl_map();

    private:
        std::vector<std::string> determine_find_package_dirs(const std::string &pkg);

        void add_lib_or_exe(size_t cmd_arguments_size, const Command &cmd);

        void target_include_directories(size_t cmd_arguments_size, const Command &cmd);

        void target_link_libraries(size_t cmd_arguments_size, const Command &cmd);

        void find_package(size_t cmd_arguments_size, const Command &cmd);

        void message(size_t cmd_arguments_size, const Command &cmd);

        void get_filename_component(size_t cmd_arguments_size, const Command &cmd);

        void project(size_t cmd_arguments_size, const Command &cmd);

        void list(size_t cmd_arguments_size, const Command &cmd);

        void cmake_policy(size_t cmd_arguments_size, const Command &cmd);

        void set(size_t cmd_arguments_size, const Command &cmd);
    };

    enum class ConditionOp
    {
        And,
        Or,
        Not,
        Identifier,
        Literal,
        Group,
        Strequal,
        Equal,
        IdentifierOrLiteral,
        InList,
        Defined
    };

    struct ConditionNode
    {
        ConditionOp op;
        std::string value;
        std::vector<std::shared_ptr<ConditionNode>> children;

        bool BoolVar(const std::string &var) const;

        bool Evaluate(Project& project) const;
    };

    struct ConditionalBlock : Evaluable
    {
        using BranchEvaluableVector = std::vector<std::shared_ptr<Evaluable>>;
        std::vector<std::pair<std::shared_ptr<ConditionNode>, BranchEvaluableVector>> branches;
        BranchEvaluableVector elseBranch;

        BranchEvaluableVector* current_branch;
        ConditionalBlock* parent_block = nullptr;

        void Evaluate(Project &project) override;
        void Varize(Project &project) override;
    };

    struct CommandParser
    {
        static std::shared_ptr<Project> parseFile(const std::string &path);

        static void parseContentWithProject(Project &project, const std::string &content);

        static std::shared_ptr<Project> parseContent(const std::string &content);

        static std::shared_ptr<Project> parseContent(const std::string &content, const std::shared_ptr<ParseContext> &context_sh_ptr);

        static void varize(Command &cmd, ParseContext &parse_context);

        static void varize_str(std::string &str, ParseContext &parse_context);

        static void envize_str(std::string &str, ParseContext &parse_context);

    private:
        static std::shared_ptr<ConditionNode> parseCondition(const std::string &expr);

        static void skipWhitespaceAndComments(const std::string &s, size_t &pos);

        static void trim(std::string &s);

        static void tokenize(const std::string &s, std::vector<std::string> &out);

        static size_t findMatchingParen(const std::string &s, size_t open);
    };

}