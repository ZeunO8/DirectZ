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
#include <dz/function.hpp>

namespace dz::cmake
{
    struct Project;

    using VariableMap = std::unordered_map<std::string, std::string>;
    using ValueVector = std::vector<std::string>;

    struct Evaluable
    {
        virtual ~Evaluable() = default;
        virtual void Evaluate(Project& project) = 0;
        virtual void Varize(Project& project) = 0;
    };

    struct Command : Evaluable
    {
        std::string name;
        ValueVector arguments;
        void Evaluate(Project& project) override;
        void Varize(Project& project) override;
    };

    struct ICMakeEntity
    {
        virtual ~ICMakeEntity() = default;
        virtual std::string getName() const = 0;
        virtual void addArgument(const std::string &arg) = 0;
        virtual const ValueVector &getArguments() const = 0;
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
        const ValueVector &getArguments() const override { return sources; }
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
        ValueVector sources;
        ValueVector includeDirs;
        ValueVector linkLibs;
    };

    struct Macro
    {
        std::string name;
        ValueVector params;
        std::vector<dz::cmake::Command> body;

        void execute(Project &project, const Command &cmd) const;
    };

    struct Policy
    {
        std::string policy_name;
        std::string policy_value;
    };

    struct ParseContext
    {
        std::shared_ptr<Project> root_project;
        std::unordered_map<std::string, std::shared_ptr<Policy>> policy_set_map;
        std::deque<std::shared_ptr<Policy>> policy_stack;
        bool policy_push_just_called = false;

        VariableMap vars;
        VariableMap env;
        std::unordered_map<std::string, bool> marked_vars;

        ParseContext();

        void restore_marked_vars(const VariableMap& old_vars);

        void mark_var(const std::string& var, bool mark_bool = true);

        static VariableMap generate_default_system_vars_map();

        static VariableMap get_env_map();
    };

    struct Project
    {
        using DSL_Fn = dz::function<void(size_t, const Command &)>;
        using DSL_Map = std::unordered_map<std::string, DSL_Fn>;
        
        using DSL_Fn_With_Context = dz::function<void(size_t, const Command &, ParseContext&)>;
        using DSL_Map_With_Context = std::unordered_map<std::string, DSL_Fn_With_Context>;

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
        ValueVector determine_find_package_dirs(const std::string &pkg);

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

        void find_path(size_t cmd_arguments_size, const Command &cmd);

        void mark_as_advanced(size_t cmd_arguments_size, const Command &cmd);
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

        ConditionalBlock* parent_block = nullptr;

        bool owned_by_sh_ptr = false;

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

        static void tokenize(const std::string &s, ValueVector &out);

        static size_t findMatchingParen(const std::string &s, size_t open);
    };

}