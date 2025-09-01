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
#include <deque>
#include <dz/function.hpp>
#include <functional>

#define dsl_some_abstract_arguments_real size_t, const dz::cmake::Command &
#define dsl_some_abstract_arguments size_t cmd_arguments_size, const Command &cmd
#define DSL_Fn dz::function<void(dsl_some_abstract_arguments_real)>
#define DSL_Map std::unordered_map<std::string, DSL_Fn>
#define dsl_fn_def(NAME) void ___##NAME(dsl_some_abstract_arguments)
#define dsl_fn_project_def(NAME) void dz::cmake::Project::___##NAME(dsl_some_abstract_arguments)
#define dsl_entry(NAME) { #NAME, DSL_Fn(this, &Project::___##NAME) }
#define dsL_entry_key(NAME, FUNC) { #NAME, DSL_Fn(this, &Project::___##FUNC) }
#define dsL_entry_str(STR, FUNC) { STR, DSL_Fn(this, &Project::___##FUNC) }
#define dsl_all_abstract_arguments dsl_some_abstract_arguments, const dz::cmake::ValueVector &options_set, const dz::cmake::ValueVector &one_value_keywords_set, const dz::cmake::ValueVector &multi_value_keywords_set
#define dsl_all_abstract_arguments_real dsl_some_abstract_arguments_real, const dz::cmake::ValueVector &, const dz::cmake::ValueVector &, const dz::cmake::ValueVector &

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
        const std::string* content_ptr = nullptr;
        size_t pos = -1;
        size_t end_pos = -1;
        std::string name;
        ValueVector arguments;
        Command() = default;
        Command(const std::string* content_ptr, size_t pos, size_t end_pos, const std::string& name, const std::string& args);
        Command(const Command& other);
        Command& operator=(const Command& other);
        bool operator==(const Command& other);
        void Evaluate(Project& project) override;
        void Varize(Project& project) override;
    };

    struct Block
    {
        enum Type
        {
            function,
            macro,
            foreach,
            block,
        };
        Type type;
        ValueVector arguments;
        std::vector<Command> body;
        Block(Type type);
        virtual ~Block() = default;
        void DefineArguments(size_t offset, size_t cmd_arguments_size, const Command& cmd);
        virtual void Evaluate(Project& project, size_t cmd_arguments_size, const Command& cmd) = 0;
    };

    struct foreach_Block : Block
    {
        Command foreach_cmd;
        std::shared_ptr<long long> i_ptr;
        std::shared_ptr<long long> range_i_ptr = std::make_shared<long long>(0);
        std::function<void(foreach_Block&)> loop_block_function;
        std::function<void()> clear_loop_block_function;
        std::function<bool(foreach_Block&)> loop_test_function;
        std::function<void(foreach_Block&)> loop_increment_function;

        foreach_Block(
            const std::shared_ptr<long long>& i_ptr,
            const std::function<void(foreach_Block&)>& loop_block_function,
            const std::function<void()>& clear_loop_block_function,
            const std::function<bool(foreach_Block&)>& loop_test_function,
            const std::function<void(foreach_Block&)>& loop_increment_function
        );
        void Evaluate(Project& project, size_t cmd_arguments_size, const Command& cmd) override;
    };

    struct function_Block : Block
    {
        std::string name;

        function_Block();
        void Evaluate(Project& project, size_t cmd_arguments_size, const Command& cmd) override;
    };

    struct macro_Block : Block
    {
        std::string name;

        macro_Block();
        void Evaluate(Project& project, size_t cmd_arguments_size, const Command& cmd) override;
    };

    enum class ConditionOp
    {
        Group,
        Identifier,
        Literal,
    
        // Binary Ops
        EQUAL,
        LESS,
        LESS_EQUAL,
        GREATER,
        GREATER_EQUAL,
        STREQUAL,
        STRLESS,
        STRLESS_EQUAL,
        STRGREATER,
        STRGREATER_EQUAL,
        VERSION_EQUAL,
        VERSION_LESS,
        VERSION_LESS_EQUAL,
        VERSION_GREATER,
        VERSION_GREATER_EQUAL,
        PATH_EQUAL,
        IN_LIST,
        IS_NEWER_THAN,
        MATCHES,
        AND,
        OR,
        
        // Unary Ops
        COMMAND,
        POLICY,
        TARGET,
        TEST,
        EXISTS,
        IS_READABLE,
        IS_WRITABLE,
        IS_EXECUTABLE,
        IS_DIRECTORY,
        IS_SYMLINK,
        IS_ABSOLUTE,
        DEFINED,
        NOT,
    };

    struct ConditionNode
    {
        ConditionOp op;
        std::string value;
        std::vector<std::shared_ptr<ConditionNode>> children;

        bool BoolVar(const std::string &var) const;

        bool Evaluate(Project& project) const;
        void ParseConditions(Project& project, size_t cmd_arguments_size, const Command& cmd);
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
            Unknown,
            Module,
        };
        Target(Type t, const std::string &n) : targetType(t), name(n) {}
        std::string getName() const override { return name; }
        void addArgument(const std::string &arg) override { sources.push_back(arg); }
        const ValueVector &getArguments() const override { return sources; }
        void addIncludeDir(const std::string &dir) { includeDirs.push_back(dir); }
        void addLinkLib(const std::string &lib) { linkLibs.push_back(lib); }
        void setLinkType(LinkType _linkType) { linkType = _linkType; }

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

        Type targetType = (Type)0;
        LinkType linkType = (LinkType)0;
        std::string name;
        ValueVector sources;
        ValueVector includeDirs;
        ValueVector linkLibs;
        bool isInterface = false;
        bool isImported = false;
    };

    struct Policy
    {
        std::string policy_name;
        std::string policy_value;
    };

    struct ParseContext
    {
        const std::string* content_ptr = nullptr;
        size_t pos = 0;
        size_t parse_to_pos = -1;

        std::shared_ptr<Project> root_project;

        std::unordered_map<std::string, std::shared_ptr<Policy>> policy_set_map;
        std::deque<std::shared_ptr<Policy>> policy_stack;
        bool policy_push_just_called = false;

        VariableMap vars;
        VariableMap env;
        std::unordered_map<std::string, bool> marked_vars;

        std::shared_ptr<ConditionNode> current_condition_sh_ptr;

        int if_depth = 0;
        int valid_if_depth = 0;
        int macro_depth = 0;
        int function_depth = 0;
        int block_depth = 0;

        bool recording_cmds_to_block_top = false;
        std::unordered_map<std::string, std::shared_ptr<Block>> block_map;
        std::stack<std::shared_ptr<Block>> block_stack;

        std::deque<Block*> evaluating_block_deque;
        std::deque<const Command*> evaluating_block_cmd_deque;

        bool just_triggered_break = false;
        bool just_triggered_return = false;
        bool just_triggered_continue = false;

        std::unordered_map<std::string, bool> loaded_modules;

        // Block* returning_from_block = nullptr;
        // Block* continuing_in_block = nullptr;
        // Block* breaking_out_of_block = nullptr;

        ParseContext();

        void restore_marked_vars(const VariableMap& old_vars);

        void mark_var(const std::string& var, bool mark_bool = true);

        static VariableMap generate_default_system_vars_map();

        static VariableMap get_env_map();
    };

    struct Project
    {
        
        using DSL_Fn_With_Context = std::function<void(size_t, const Command &, ParseContext&)>;
        using DSL_Map_With_Context = std::unordered_map<std::string, DSL_Fn_With_Context>;

        std::string name;
        std::string version;
        std::string compat_version;
        std::string description;
        std::string homepage_url;
        std::vector<std::string> languages;
        std::unordered_map<std::string, std::shared_ptr<Target>> targets;

        std::shared_ptr<ParseContext> context_sh_ptr;

        DSL_Map dsl_map;

        Project(const std::shared_ptr<ParseContext> &context_sh_ptr);

        void addCommand(const Command &cmd);

        void print();

    private:

        DSL_Map generate_dsl_map();

    private:
        ValueVector determine_find_package_dirs(const std::string &pkg);

        dsl_fn_def(macro);
        dsl_fn_def(endmacro);
        dsl_fn_def(function);
        dsl_fn_def(endfunction);
        dsl_fn_def(_return);
        dsl_fn_def(foreach);
        dsl_fn_def(endforeach);
        dsl_fn_def(_break);
        dsl_fn_def(_continue);
        dsl_fn_def(_if);
        dsl_fn_def(_else);
        dsl_fn_def(endif);
        dsl_fn_def(add_library);
        dsl_fn_def(add_executable);
        dsl_fn_def(target_include_directories);
        dsl_fn_def(target_link_libraries);
        dsl_fn_def(find_package);
        dsl_fn_def(message);
        dsl_fn_def(get_filename_component);
        dsl_fn_def(project);
        dsl_fn_def(list);
        dsl_fn_def(cmake_policy);
        dsl_fn_def(set);
        dsl_fn_def(unset);
        dsl_fn_def(find_path);
        dsl_fn_def(find_library);
        dsl_fn_def(find_program);
        dsl_fn_def(mark_as_advanced);
        dsl_fn_def(cmake_parse_arguments);
        dsl_fn_def(file);
        dsl_fn_def(string);
        dsl_fn_def(include);
    };

    struct CommandParser
    {
        static std::shared_ptr<Project> parseFile(const std::string &path);

        static std::shared_ptr<Project> parseContent(const std::string &content);

        static void parseContentWithProject(Project &project, const std::string &content);

        static std::shared_ptr<Project> parseContent(const std::string &content, const std::shared_ptr<ParseContext> &context_sh_ptr);

        static void execute_context_til_break(ParseContext& context, Project& project);

        static bool get_next_command(ParseContext& context, Project& project, Command& out_cmd);

        static void process_cmd(ParseContext& context, Project& project, Command& cmd);

        static void varize(Command &cmd, ParseContext &parse_context);

        static void varize_str(std::string &str, ParseContext &parse_context, bool dequite = false);

        static void envize_str(std::string &str, ParseContext &parse_context);

        static void tokenize(const std::string &s, ValueVector &out);

        static void skipWhitespaceAndComments(const std::string &s, size_t &pos);

        static void trim(std::string &s);

        static size_t findMatchingParen(const std::string &s, size_t open);
    };

}