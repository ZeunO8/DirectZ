#include <dz/CMake.hpp>
#include <dz/Util.hpp>
#include <queue>
#include <unordered_set>
#include <cassert>
#include <regex>

namespace dz::cmake
{
    Command::Command(const std::string *content_ptr, size_t pos, size_t end_pos, const std::string &name, const std::string &args) : content_ptr(content_ptr),
                                                                                                                                     pos(pos),
                                                                                                                                     end_pos(end_pos),
                                                                                                                                     name(name)
    {
        CommandParser::tokenize(args, arguments);
    }
    Command::Command(const Command &other) : content_ptr(other.content_ptr),
                                             pos(other.pos),
                                             end_pos(other.end_pos),
                                             name(other.name),
                                             arguments(other.arguments)
    {
    }
    Command &Command::operator=(const Command &other)
    {
        content_ptr = other.content_ptr;
        pos = other.pos;
        end_pos = other.end_pos;
        name = other.name;
        arguments = other.arguments;
        return *this;
    }
    bool Command::operator==(const Command &other)
    {
        return content_ptr == other.content_ptr && pos == other.pos;
    }
    void mark_unset_options(const auto &options, const auto &options_set, auto &all_keys_and_vals, const auto &abs_prefix)
    {
        for (auto &possible_option : options)
        {
            if (!in_vec(options_set, possible_option))
            {
                all_keys_and_vals[abs_prefix + possible_option] = "FALSE";
            }
        }
    }

    std::tuple<ValueVector, VariableMap, ValueVector, ValueVector, ValueVector> parse_all_keys_and_vals(
        ParseContext &context,
        const std::string &prefix,
        const ValueVector &multi_value_keywords,
        const ValueVector &one_value_keywords,
        const ValueVector &options,
        size_t cmd_arguments_size,
        const Command &cmd,
        size_t parse_argv = 0)
    {
        VariableMap all_keys_and_vals;

        insert_to_map_from_vec_with_string_prefix_prepended(all_keys_and_vals, multi_value_keywords, prefix);
        insert_to_map_from_vec_with_string_prefix_prepended(all_keys_and_vals, one_value_keywords, prefix);
        insert_to_map_from_vec_with_string_prefix_prepended(all_keys_and_vals, options, prefix);

        std::string parsing_key;
        std::string *parsing_val = nullptr;
        ValueVector new_arguments;

        ValueVector options_set, one_value_keywords_set, multi_value_keywords_set;

        std::string abs_prefix = prefix.empty() ? "" : (prefix + "_");

        for (size_t i = 0; i < cmd_arguments_size; i++)
        {
            auto &current_arg = cmd.arguments[i];
            if (in_map(all_keys_and_vals, abs_prefix + current_arg))
            {
                parsing_key = current_arg;
                parsing_val = &all_keys_and_vals[abs_prefix + parsing_key];
                if (in_vec(options, parsing_key))
                {
                    (*parsing_val) = "TRUE";
                    options_set.push_back(parsing_key);
                    parsing_key.clear();
                }
                else if (in_vec(one_value_keywords, parsing_key))
                {
                    one_value_keywords_set.push_back(parsing_key);
                }
                else // only possible other vec is multi
                {
                    multi_value_keywords_set.push_back(parsing_key);
                }
            }
            else if (parsing_key.empty())
            {
                if ((parse_argv != 0 && new_arguments.size() < parse_argv) || !parse_argv)
                {
                    new_arguments.push_back(current_arg);
                }
            }
            else if (in_vec(one_value_keywords, parsing_key))
            {
                if (parsing_val->empty())
                {
                    (*parsing_val) = current_arg;
                    parsing_key.clear();
                }
                else
                {
                    throw std::runtime_error("[cmake] '" + parsing_key + "' expects one value)");
                }
            }
            else if (in_vec(multi_value_keywords, parsing_key))
            {
                (*parsing_val) += ((parsing_val->empty() ? "" : ";") + current_arg);
            }
        }

        mark_unset_options(options, options_set, all_keys_and_vals, abs_prefix);

        return {new_arguments, all_keys_and_vals, options_set, one_value_keywords_set, multi_value_keywords_set};
    }

    template <typename F>
    auto abstractify_cmake_function(
        const std::shared_ptr<ParseContext> &context_sh_ptr,
        const std::string &prefix,
        const ValueVector &options,
        const ValueVector &one_value_keywords,
        const ValueVector &multi_value_keywords,
        F run_with_abstract_set,
        size_t parse_argv = 0,
        bool new_scope = true)
    {
        struct AbstractContext
        {
            VariableMap all_keys_and_vals;
            std::unordered_map<std::string, std::shared_ptr<Block>> old_block_map;
            std::unordered_map<std::string, bool> old_marked_vars;
            VariableMap old_context_vars;
            int old_if_depth;
            int old_valid_if_depth;
        };
        auto abs_con_sh_ptr = std::make_shared<AbstractContext>();
        return std::make_pair([=](dsl_some_abstract_arguments) mutable
                              {
            auto &context = *context_sh_ptr;

            auto& abs_con = *abs_con_sh_ptr;
            abs_con.old_block_map = context.block_map;
            abs_con.old_marked_vars = context.marked_vars;
            abs_con.old_context_vars = context.vars;
            abs_con.old_if_depth = context.if_depth;
            abs_con.old_valid_if_depth = context.valid_if_depth;

            if ((cmd.name == "else" || cmd.name == "elseif") && (context.if_depth - 1) == context.valid_if_depth)
                static_assert(true);
            else if ((cmd.name != "endif") && context.if_depth != context.valid_if_depth)
            {
                if (cmd.name == "if") {
                    if (context.valid_if_depth > context.if_depth)
                        context.valid_if_depth++;
                    context.if_depth++;
                }
                return;
            }

            auto [new_arguments, all_keys_and_vals, options_set, one_value_keywords_set, multi_value_keywords_set] = parse_all_keys_and_vals(
                context,
                prefix,
                multi_value_keywords,
                one_value_keywords,
                options,
                cmd_arguments_size,
                cmd,
                parse_argv);

            {
                Command new_cmd = cmd;
                new_cmd.arguments = new_arguments;
                abs_con.all_keys_and_vals = all_keys_and_vals;
                if (new_scope)
                {
                    context.valid_if_depth = context.if_depth = 0;
                }
                insert_to_map_from_map(context.vars, all_keys_and_vals);
                if constexpr (requires { run_with_abstract_set(new_cmd.arguments.size(), new_cmd, options_set, one_value_keywords_set, multi_value_keywords_set); })
                {
                    run_with_abstract_set(new_cmd.arguments.size(), new_cmd, options_set, one_value_keywords_set, multi_value_keywords_set);
                }
                else if constexpr (requires { run_with_abstract_set(new_cmd.arguments.size(), new_cmd); })
                {
                    run_with_abstract_set(new_cmd.arguments.size(), new_cmd);
                }
                else if constexpr (requires { run_with_abstract_set(); })
                {
                    run_with_abstract_set();
                }
            } }, [=]() mutable
                              {
            auto &context = *context_sh_ptr;
            auto& abs_con = *abs_con_sh_ptr;
            if (new_scope)
            {
                auto new_context_vars = context.vars;
                context.vars = abs_con.old_context_vars;
                context.restore_marked_vars(new_context_vars);
                context.marked_vars = abs_con.old_marked_vars;
                context.valid_if_depth = abs_con.old_valid_if_depth;
                context.if_depth = abs_con.old_if_depth;
                context.block_map = abs_con.old_block_map;
            }
            else
            {
                remove_to_map_from_map(abs_con.old_context_vars, context.vars, abs_con.all_keys_and_vals);
            } });
    }

    auto abstractify_scope(
        const std::shared_ptr<ParseContext> &context_sh_ptr,
        const std::string &prefix,
        const ValueVector &options,
        const ValueVector &one_value_keywords,
        const ValueVector &multi_value_keywords,
        size_t parse_argv = 0,
        bool new_scope = true)
    {
        struct AbstractContext
        {
            VariableMap all_keys_and_vals;
            std::unordered_map<std::string, std::shared_ptr<Block>> old_block_map;
            std::unordered_map<std::string, bool> old_marked_vars;
            VariableMap old_context_vars;
            int old_if_depth;
            int old_valid_if_depth;
        };
        auto abs_con_sh_ptr = std::make_shared<AbstractContext>();
        return std::make_pair([=](dsl_some_abstract_arguments) mutable
                              {
            auto &context = *context_sh_ptr;
            auto& abs_con = *abs_con_sh_ptr;
            abs_con.old_block_map = context.block_map;
            abs_con.old_marked_vars = context.marked_vars;
            abs_con.old_context_vars = context.vars;
            abs_con.old_if_depth = context.if_depth;
            abs_con.old_valid_if_depth = context.valid_if_depth;

            auto [new_arguments, all_keys_and_vals, options_set, one_value_keywords_set, multi_value_keywords_set] = parse_all_keys_and_vals(
                context,
                prefix,
                multi_value_keywords,
                one_value_keywords,
                options,
                cmd_arguments_size,
                cmd,
                parse_argv);

            abs_con.all_keys_and_vals = all_keys_and_vals;
            if (new_scope)
            {
                context.valid_if_depth = context.if_depth = 0;
            }
            insert_to_map_from_map(context.vars, all_keys_and_vals); }, [=]() mutable
                              {
            auto &context = *context_sh_ptr;
            auto& abs_con = *abs_con_sh_ptr;
            if (new_scope)
            {
                auto new_context_vars = context.vars;
                context.vars = abs_con.old_context_vars;
                context.restore_marked_vars(new_context_vars);
                context.marked_vars = abs_con.old_marked_vars;
                context.valid_if_depth = abs_con.old_valid_if_depth;
                context.if_depth = abs_con.old_if_depth;
                context.block_map = abs_con.old_block_map;
            }
            else
            {
                remove_to_map_from_map(abs_con.old_context_vars, context.vars, abs_con.all_keys_and_vals);
            } });
    }

    inline static auto identify_var(ParseContext &context, auto &var)
    {
        auto var_it = context.vars.find(var);
        if (var_it == context.vars.end())
            return var;
        return var_it->second;
    }
    inline static auto identify_child(ParseContext &context, auto &child)
    {
        if (child.op == ConditionOp::Identifier)
        {
            auto var_it = context.vars.find(child.value);
            if (var_it == context.vars.end())
                return child.value;
            return var_it->second;
        }
        return dequote(child.value);
    }

    inline static bool toBool(auto &project, auto &context, const auto &condition_node)
    {
        switch (condition_node.op)
        {
        case ConditionOp::Identifier:
        {
            auto it = context.vars.find(condition_node.value);
            return it != context.vars.end() ? truthy(it->second) : false;
        }
        case ConditionOp::Literal:
        {
            return truthy(condition_node.value);
        }
        case ConditionOp::Group:
        {
            return condition_node.Evaluate(project);
        }
        default:
        {
            return truthy(condition_node.value);
        }
        }
    }

    inline static bool isNot(const auto &condition_node)
    {
        if (condition_node.op == ConditionOp::Not)
            return true;
        if (condition_node.op == ConditionOp::Identifier || condition_node.op == ConditionOp::Literal)
        {
            std::string u = to_upper(condition_node.value);
            if (u == "!" || u == "NOT")
                return true;
        }
        return false;
    }
}

void dz::cmake::Command::Evaluate(Project &project)
{
    auto &context = *project.context_sh_ptr;

    auto cmd_arguments_size = arguments.size();

    auto block_it = context.block_map.find(name);
    if (block_it != context.block_map.end())
    {
        auto &block = *block_it->second;
        block.Evaluate(project, cmd_arguments_size, *this);
        return;
    }

    name = to_lower(name);
    auto cmd_it = project.dsl_map.find(name);
    if (cmd_it == project.dsl_map.end())
    {
        throw std::runtime_error(R"(
[cmake] CMake Error:
[cmake]   Unknown CMake command ")" +
                                 name + R"(".
)");
    }

    cmd_it->second(cmd_arguments_size, *this);
}

void dz::cmake::Command::Varize(Project &project)
{
    CommandParser::varize(*this, *project.context_sh_ptr);
}

dz::cmake::Block::Block(Type type) : type(type)
{
}

void dz::cmake::Block::DefineArguments(size_t offset, size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size <= offset)
        return;
    arguments.insert(arguments.end(), cmd.arguments.begin() + offset, cmd.arguments.end());
}

dz::cmake::foreach_Block::foreach_Block(
    const std::shared_ptr<long long> &i_ptr,
    const std::function<void(foreach_Block &)> &loop_block_function,
    const std::function<void()> &clear_loop_block_function,
    const std::function<bool(foreach_Block &)> &loop_test_function,
    const std::function<void(foreach_Block &)> &loop_increment_function) : Block(Block::foreach),
                                                                           i_ptr(i_ptr),
                                                                           loop_block_function(loop_block_function),
                                                                           clear_loop_block_function(clear_loop_block_function),
                                                                           loop_test_function(loop_test_function),
                                                                           loop_increment_function(loop_increment_function)
{
}

void dz::cmake::foreach_Block::Evaluate(Project &project, size_t cmd_arguments_size, const Command &cmd)
{
    // auto &context = *project.context_sh_ptr;
    // auto [context_function, context_clear] = abstractify_cmake_function(project.context_sh_ptr, prefix + "", {"IN"}, {}, {"LISTS", "ITEMS", "ZIP_LISTS", "RANGE"}, [&](dsl_all_abstract_arguments)
    //                                                    {
    //         return; }, 2, false);
    // context_function(cmd_arguments_size, cmd);
    // context_clear();
    return;
}

dz::cmake::function_Block::function_Block() : Block(Block::function)
{
}

void dz::cmake::function_Block::Evaluate(Project &project, size_t cmd_arguments_size, const Command &cmd)
{
    auto &context = *project.context_sh_ptr;
    static std::string prefix = "";
    static ValueVector options = {};
    auto one_value_keywords = arguments;
    static ValueVector multi_value_keywords = {};
    auto ___evaluate_function_impl = [&](dsl_all_abstract_arguments)
    {
        if (body.empty())
            return;
        context.evaluating_block_deque.push_front(this);
        context.evaluating_block_cmd_deque.push_front(&cmd);
        auto old_parse_to_pos = context.parse_to_pos;
        auto old_pos = context.pos;
        context.pos = body.front().pos;
        context.parse_to_pos = body.back().end_pos;
        dz::cmake::CommandParser::execute_context_til_break(context, project);
        context.parse_to_pos = old_parse_to_pos;
        context.pos = old_pos;
        context.evaluating_block_deque.pop_front();
        context.evaluating_block_cmd_deque.pop_front();
    };
    auto [context_function, context_clear] = abstractify_cmake_function(project.context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___evaluate_function_impl, 0, true);
    auto block_cmd = cmd;
    block_cmd.arguments.clear();
    block_cmd.arguments.reserve(cmd_arguments_size + arguments.size());
    auto cmd_arguments_data = cmd.arguments.data();
    auto j = 0;
    for (auto &arg_name : arguments)
    {
        if (j >= cmd_arguments_size)
            break;
        block_cmd.arguments.push_back(arg_name);
        block_cmd.arguments.push_back(cmd_arguments_data[j++]);
    }
    for (; j < cmd_arguments_size; ++j)
    {
        block_cmd.arguments.push_back(cmd_arguments_data[j]);
    }
    context_function(block_cmd.arguments.size(), block_cmd);
    context_clear();
    return;
}

dz::cmake::macro_Block::macro_Block() : Block(Block::macro)
{
}

void dz::cmake::macro_Block::Evaluate(Project &project, size_t cmd_arguments_size, const Command &cmd)
{
    auto &context = *project.context_sh_ptr;
    static std::string prefix = "";
    static ValueVector options = {};
    auto one_value_keywords = arguments;
    static ValueVector multi_value_keywords = {};
    auto ___evaluate_macro_impl = [&](dsl_all_abstract_arguments)
    {
        if (body.empty())
            return;
        context.evaluating_block_deque.push_front(this);
        context.evaluating_block_cmd_deque.push_front(&cmd);
        auto old_parse_to_pos = context.parse_to_pos;
        auto old_pos = context.pos;
        context.pos = body.front().pos;
        context.parse_to_pos = body.back().end_pos;
        dz::cmake::CommandParser::execute_context_til_break(context, project);
        context.parse_to_pos = old_parse_to_pos;
        context.pos = old_pos;
        context.evaluating_block_deque.pop_front();
        context.evaluating_block_cmd_deque.pop_front();
    };
    auto [context_function, context_clear] = abstractify_cmake_function(project.context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___evaluate_macro_impl, 0, true);
    auto block_cmd = cmd;
    block_cmd.arguments.clear();
    block_cmd.arguments.reserve(cmd_arguments_size + arguments.size());
    auto cmd_arguments_data = cmd.arguments.data();
    auto j = 0;
    for (auto &arg_name : arguments)
    {
        if (j >= cmd_arguments_size)
            break;
        block_cmd.arguments.push_back(arg_name);
        block_cmd.arguments.push_back(cmd_arguments_data[j++]);
    }
    for (; j < cmd_arguments_size; ++j)
    {
        block_cmd.arguments.push_back(cmd_arguments_data[j]);
    }
    context_function(block_cmd.arguments.size(), block_cmd);
    context_clear();
    return;
}

void dz::cmake::ConditionNode::ParseConditions(Project &project, size_t cmd_arguments_size, const Command &cmd)
{
    static std::unordered_map<std::string, ConditionOp> condition_op_map = {
        {"AND", ConditionOp::And},
        {"NOT", ConditionOp::Not},
        {"OR", ConditionOp::Or},
        {"STREQUAL", ConditionOp::Strequal},
        {"LESS", ConditionOp::Less},
        {"GREATER", ConditionOp::Greater},
        {"LESS_EQUAL", ConditionOp::LessEqual},
        {"GREATER_EQUAL", ConditionOp::GreaterEqual},
        {"EQUAL", ConditionOp::Equal},
        {"IN_LIST", ConditionOp::InList},
        {"DEFINED", ConditionOp::Defined},
        {"EXISTS", ConditionOp::Exists},
    };

    auto &context = *project.context_sh_ptr;

    op = ConditionOp::Group;
    value = join_string_vec(cmd.arguments, " ");

    children.reserve(cmd.arguments.size());
    for (auto &arg : cmd.arguments)
    {
        auto cond_node = std::make_shared<ConditionNode>();
        children.push_back(cond_node);

        auto &cond = *cond_node;
        cond.value = arg;

        auto cond_it = condition_op_map.find(arg);
        if (cond_it != condition_op_map.end())
        {
            cond.op = cond_it->second;
            continue;
        }

        auto var_it = context.vars.find(arg);
        if (var_it != context.vars.end() || !is_literal(arg))
        {
            cond.op = ConditionOp::Identifier;
        }
        else
        {
            cond.op = ConditionOp::Literal;
        }
    }

    return;
}

void dz::cmake::ParseContext::restore_marked_vars(const VariableMap &old_vars)
{
    for (auto &[marked_var, is_marked] : marked_vars)
    {
        if (!is_marked)
            continue;
        is_marked = false;
        auto o_it = old_vars.find(marked_var);
        if (o_it == old_vars.end())
            continue;
        vars[marked_var] = o_it->second;
    }
    marked_vars.clear();
}

void dz::cmake::ParseContext::mark_var(const std::string &var, bool mark_bool)
{
    marked_vars[var] = mark_bool;
}

dz::cmake::VariableMap dz::cmake::ParseContext::generate_default_system_vars_map()
{
    return {
#if defined(_WIN32)
        {"WIN32", "TRUE"},
#elif defined(__linux__) && !defined(_ANDROID_)
        {"UNIX", "TRUE"},
#elif defined(MACOS)
        {"UNIX", "TRUE"},
#elif defined(IOS)
        {"IOS", "TRUE"},
#elif defined(ANDROID)
        {"ANDROID", "TRUE"},
#endif
        {"CMAKE_CURRENT_LIST_DIR", std::filesystem::absolute(".").string()},
        {"CMAKE_SYSTEM_NAME",
#if defined(_WIN32)
         "Windows"
#elif defined(__linux__) && !defined(_ANDROID_)
         "Linux"
#elif defined(MACOS)
         "Darwin"
#elif defined(IOS)
         "iOS"
#elif defined(ANDROID)
         "Android"
#endif
        },
        {"CMAKE_SIZEOF_VOID_P", std::to_string(sizeof(void *))}};
}

dz::cmake::VariableMap dz::cmake::ParseContext::get_env_map()
{
    return get_all_env_vars();
}

dz::cmake::ParseContext::ParseContext() : vars(generate_default_system_vars_map()),
                                          env(get_env_map())
{
}

dz::cmake::Project::Project(const std::shared_ptr<ParseContext> &context_sh_ptr) : dsl_map(generate_dsl_map()),
                                                                                   context_sh_ptr(context_sh_ptr)
{
}

dsl_fn_project_def(macro)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___MACRO___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___macro_impl = [&](dsl_all_abstract_arguments)
    {
        if (!cmd_arguments_size)
            throw std::runtime_error("[cmake] -- macro requires <name>");
        auto &macro_name = cmd.arguments[0];
        auto macro_block_sh_ptr = std::make_shared<macro_Block>();
        context.block_map[macro_name] = macro_block_sh_ptr;
        context.recording_cmds_to_block_top = true;
        auto &macro_block = *macro_block_sh_ptr;
        macro_block.name = macro_name;
        macro_block.DefineArguments(1, cmd_arguments_size, cmd);
        context.block_stack.push(macro_block_sh_ptr);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___macro_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(endmacro)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ENDMACRO___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___endmacro_impl = [&](dsl_all_abstract_arguments)
    {
        if (context.block_stack.empty())
            throw std::runtime_error("[cmake] -- endmacro() without opening macro()");
        auto macro_block_ptr = context.block_stack.top();
        if (macro_block_ptr->type != Block::macro)
            throw std::runtime_error("[cmake] -- endmacro() without opening macro()");
        context.block_stack.pop();
        if (context.block_stack.empty() ||
            (context.block_stack.top()->type != Block::macro &&
             context.block_stack.top()->type != Block::block))
            context.recording_cmds_to_block_top = false;
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___endmacro_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(function)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___FUNCTION___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___function_impl = [&](dsl_all_abstract_arguments)
    {
        if (!cmd_arguments_size)
            throw std::runtime_error("[cmake] -- function requires <name>");
        auto &function_name = cmd.arguments[0];
        auto function_block_sh_ptr = std::make_shared<function_Block>();
        context.block_map[function_name] = function_block_sh_ptr;
        context.recording_cmds_to_block_top = true;
        auto &function_block = *function_block_sh_ptr;
        function_block.name = function_name;
        function_block.DefineArguments(1, cmd_arguments_size, cmd);
        context.block_stack.push(function_block_sh_ptr);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___function_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(endfunction)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ENDFUNCTION___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___endfunction_impl = [&](dsl_all_abstract_arguments)
    {
        if (context.block_stack.empty())
            throw std::runtime_error("[cmake] -- endfunction() without opening function()");
        auto function_block_ptr = context.block_stack.top();
        if (function_block_ptr->type != Block::function)
            throw std::runtime_error("[cmake] -- endfunction() without opening function()");
        context.block_stack.pop();
        if (context.block_stack.empty() ||
            (context.block_stack.top()->type != Block::function &&
             context.block_stack.top()->type != Block::block))
            context.recording_cmds_to_block_top = false;
        if (context.just_triggered_return)
        {
            context.just_triggered_return = false;
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___endfunction_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(_return)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___RETURN___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___return_impl = [&]()
    {
        if (context.evaluating_block_deque.empty())
            throw std::runtime_error("[cmake] -- return() used and not within evaluable block");
        auto eval_deque_size = context.evaluating_block_deque.size();
        size_t block_i = 0;
        for (; block_i < eval_deque_size; block_i++)
        {
            auto block_ptr = context.evaluating_block_deque[block_i];
            assert(block_ptr);
            auto type = block_ptr->type;
            if (type == Block::function || type == Block::block || type == Block::macro)
            {
                // context.returning_from_block = block_ptr;
                context.just_triggered_return = true;
                break;
            }
        }
        if (block_i == eval_deque_size)
            throw std::runtime_error("[cmake] -- return() used and not within evaluable block");
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___return_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(foreach)
{
    auto &context = *context_sh_ptr;
    std::string prefix;
    if (!context.evaluating_block_deque.empty())
    {
        auto block_ptr = context.evaluating_block_deque.front();
        if (block_ptr->type == Block::foreach)
        {
            auto foreach_block_ptr = dynamic_cast<foreach_Block *>(block_ptr);
            if (foreach_block_ptr->foreach_cmd == cmd)
            {
                prefix = "___evaluate_foreach_impl____" + std::to_string(context.evaluating_block_deque.size());
            }
            else
            {
                goto _default_prefix;
            }
        }
        else
        {
            goto _default_prefix;
        }
    }
    else
    {
    _default_prefix:
        prefix = "___evaluate_foreach_impl____" + std::to_string(context.evaluating_block_deque.size() + 1);
    }
    static ValueVector options = {"IN"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {"LISTS", "ITEMS", "ZIP_LISTS", "RANGE"};
    auto ___evaluate_foreach_impl = [&](dsl_all_abstract_arguments)
    {
        _begin:
            if (!context.evaluating_block_deque.empty())
            {
                auto block_ptr = context.evaluating_block_deque.front();
                if (block_ptr->type == Block::foreach)
                {
                    auto foreach_block_ptr = dynamic_cast<foreach_Block *>(block_ptr);
                    if (foreach_block_ptr->foreach_cmd == cmd)
                    {
                        if (!foreach_block_ptr->loop_test_function(*foreach_block_ptr))
                        {
                            context.just_triggered_break = true;
                            return;
                        }
                        else
                        {
                            foreach_block_ptr->loop_block_function(*foreach_block_ptr);
                            return;
                        }
                    }
                }
            }

            {
                enum class ChosenLoopType
                {
                    In = 1,
                    Range = 2,
                    ZipLists = 3
                };
                auto &context = *context_sh_ptr;

                ChosenLoopType chose_loop = (ChosenLoopType)0;
                if (cmd_arguments_size < 1)
                    return;

                auto in = in_vec(options_set, "IN");

                auto lists_set = in_vec(multi_value_keywords_set, "LISTS");
                auto items_set = in_vec(multi_value_keywords_set, "ITEMS");
                auto zip_lists_set = in_vec(multi_value_keywords_set, "ZIP_LISTS");

                auto range_set = in_vec(multi_value_keywords_set, "RANGE");

                auto lists = split_string(context.vars[prefix + "_LISTS"], ";");
                auto items = split_string(context.vars[prefix + "_ITEMS"], ";");
                auto zip_lists = split_string(context.vars[prefix + "_ZIP_LISTS"], ";");

                auto all_lists = lists;
                all_lists.insert(all_lists.end(), items.begin(), items.end());
                all_lists.insert(all_lists.end(), zip_lists.begin(), zip_lists.end());
                auto all_lists_size = all_lists.size();

                ValueVector loop_vars, loop_ranges;
                loop_vars.push_back(cmd.arguments[0]);

                if (!in && !lists_set && !items_set && !zip_lists_set && !range_set && cmd_arguments_size == 2)
                {
                    loop_ranges.push_back(cmd.arguments[1]);
                    chose_loop = ChosenLoopType::In;
                }
                else
                {
                    if (in && !zip_lists_set && !items_set && !lists_set && !range_set && cmd_arguments_size > 1)
                    {
                        loop_ranges.push_back(cmd.arguments[1]);
                        chose_loop = ChosenLoopType::In;
                    }
                    else if (in)
                    {
                        for (size_t arg_l = 0; arg_l < all_lists_size; arg_l++)
                        {
                            auto &arg = all_lists[arg_l];
                            loop_ranges.push_back(arg);
                            if (arg_l > 0)
                            {
                                if (loop_ranges.size() <= cmd.arguments.size())
                                    loop_vars.push_back(cmd.arguments[arg_l]);
                            }
                        }
                        if (zip_lists_set)
                        {
                            chose_loop = ChosenLoopType::ZipLists;
                        }
                        else if (items_set)
                            chose_loop = ChosenLoopType::In;
                        else if (lists_set)
                            chose_loop = ChosenLoopType::In;
                    }
                    else if (range_set)
                        chose_loop = ChosenLoopType::Range;
                    else
                        throw std::runtime_error("[cmake] -- Unsupported foreach arguments");
                }

                std::shared_ptr<foreach_Block> foreach_block_sh_ptr;

                switch (chose_loop)
                {
                case ChosenLoopType::In:
                {
                    auto &loop_var = loop_vars.front();

                    auto [set_scope_function, clear_scope_function] = abstractify_scope(context_sh_ptr, "", {}, loop_vars, {}, 0, false);

                    auto context_ptr = &context;

                    foreach_block_sh_ptr = std::make_shared<foreach_Block>(
                        std::make_shared<long long>(0),
                        [loop_var, set_scope_function, loop_ranges, context_ptr](foreach_Block &foreach_block) mutable
                        {
                            auto &range_i = *foreach_block.range_i_ptr;
                            auto &i = *foreach_block.i_ptr;
                            auto &loop_range_id = loop_ranges[range_i];
                            auto &context = *context_ptr;
                            CommandParser::varize_str(loop_range_id, context);
                            auto loop_range = identify_var(context, loop_range_id);
                            auto loop_split = split_string(loop_range, ";");
                            auto loop_val = loop_split[i];
                            Command loop_cmd;
                            loop_cmd.arguments.push_back(loop_var);
                            loop_cmd.arguments.push_back(loop_val);
                            set_scope_function(loop_cmd.arguments.size(), loop_cmd);
                            return;
                        },
                        [clear_scope_function]() mutable
                        {
                            clear_scope_function();
                            return;
                        },
                        [loop_ranges, context_ptr](foreach_Block &foreach_block) mutable
                        {
                            auto &range_i = *foreach_block.range_i_ptr;
                            auto &i = *foreach_block.i_ptr;
                            auto range_lt = range_i < loop_ranges.size();
                            if (!range_lt)
                                return false;
                            auto &loop_range_id = loop_ranges[range_i];
                            auto &context = *context_ptr;
                            CommandParser::varize_str(loop_range_id, context);
                            auto loop_range = identify_var(context, loop_range_id);
                            auto loop_split = split_string(loop_range, ";");
                            auto val_lt = i < loop_split.size();
                            return val_lt;
                        },
                        [loop_ranges, context_ptr](foreach_Block &foreach_block) mutable
                        {
                            auto &range_i = *foreach_block.range_i_ptr;
                            auto &i = *foreach_block.i_ptr;

                            auto &loop_range_id = loop_ranges[range_i];
                            auto &context = *context_ptr;
                            CommandParser::varize_str(loop_range_id, context);
                            auto loop_range = identify_var(context, loop_range_id);
                            auto loop_split = split_string(loop_range, ";");

                            i++;
                            if (i >= loop_split.size())
                            {
                                i = 0;
                                range_i++;
                            }
                            return;
                        });
                    break;
                }
                case ChosenLoopType::Range:
                {
                    auto ranges = split_string(context.vars[prefix + "_RANGE"], ";");
                    auto ranges_size = ranges.size();

                    long long start = 0, stop = 0, step = 1;

                    switch (ranges_size)
                    {
                    case 1:
                    {
                        try
                        {
                            stop = std::stold(ranges[0]);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("[cmake] -- Unable to conver range var to Number");
                        }
                        break;
                    }
                    case 2:
                    {
                        try
                        {
                            start = std::stold(ranges[0]);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("[cmake] -- Unable to conver range var to Number");
                        }

                        try
                        {
                            stop = std::stold(ranges[1]);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("[cmake] -- Unable to conver range var to Number");
                        }
                        break;
                    }
                    case 3:
                    {
                        try
                        {
                            start = std::stold(ranges[0]);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("[cmake] -- Unable to conver range var to Number");
                        }

                        try
                        {
                            stop = std::stold(ranges[1]);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("[cmake] -- Unable to conver range var to Number");
                        }

                        try
                        {
                            step = std::stold(ranges[2]);
                        }
                        catch (...)
                        {
                            throw std::runtime_error("[cmake] -- Unable to conver range var to Number");
                        }
                        break;
                    }
                    }

                    auto &loop_var = loop_vars.front();

                    auto [set_scope_function, clear_scope_function] = abstractify_scope(context_sh_ptr, "", {}, loop_vars, {}, 0, false);

                    foreach_block_sh_ptr = std::make_shared<foreach_Block>(
                        std::make_shared<long long>(start),
                        [loop_var, set_scope_function](foreach_Block &foreach_block) mutable
                        {
                            auto &i = *foreach_block.i_ptr;
                            Command loop_cmd;
                            loop_cmd.arguments.push_back(loop_var);
                            loop_cmd.arguments.push_back(std::to_string(i));
                            set_scope_function(loop_cmd.arguments.size(), loop_cmd);
                            return;
                        },
                        [clear_scope_function]() mutable
                        {
                            clear_scope_function();
                            return;
                        },
                        [stop](foreach_Block &foreach_block)
                        {
                            return *foreach_block.i_ptr <= stop;
                        },
                        [step](foreach_Block &foreach_block)
                        {
                            *foreach_block.i_ptr += step;
                            return;
                        });
                    break;
                }
                case ChosenLoopType::ZipLists:
                {
                    auto looking_for_loop_vars = loop_vars;
                    if (loop_vars.empty())
                        throw std::runtime_error("[cmake] -- no <loop_variable> provided to foreach(<loop_variable>... IN ZIP_LISTS <list>...)");
                    else if (loop_vars.size() != 1 && loop_vars.size() != loop_ranges.size())
                        throw std::runtime_error("[cmake] -- incompatible <loop_variable>... range provided to foreach(<loop_variable>... IN ZIP_LISTS <list>...)");
                    else if (loop_vars.size() == 1)
                    {
                        auto loop_var = loop_vars.front();
                        looking_for_loop_vars.clear();
                        for (size_t i = 0; i < loop_ranges.size(); i++)
                            looking_for_loop_vars.push_back(loop_var + "_" + std::to_string(i));
                    }

                    auto [set_scope_function, clear_scope_function] = abstractify_scope(context_sh_ptr, "", {}, looking_for_loop_vars, {}, 0, false);

                    auto context_ptr = &context;

                    foreach_block_sh_ptr = std::make_shared<foreach_Block>(
                        std::make_shared<long long>(0),
                        [loop_vars, set_scope_function, loop_ranges, context_ptr](foreach_Block &foreach_block) mutable
                        {
                            auto &i = *foreach_block.i_ptr;
                            size_t loop_var_i = 0;
                            Command loop_cmd;
                            for (auto& loop_range_id : loop_ranges)
                            {
                                auto &context = *context_ptr;
                                CommandParser::varize_str(loop_range_id, context);
                                auto loop_range = identify_var(context, loop_range_id);
                                auto loop_split = split_string(loop_range, ";");
                                std::string loop_val = i < loop_split.size() ? loop_split[i] : "";
                                std::string loop_var = (loop_vars.size() == loop_ranges.size()) ?
                                    loop_vars[loop_var_i] :
                                    (loop_vars[0] + "_" + std::to_string(loop_var_i));
                                loop_cmd.arguments.push_back(loop_var);
                                loop_cmd.arguments.push_back(loop_val);
                                loop_var_i++;
                            }
                            set_scope_function(loop_cmd.arguments.size(), loop_cmd);
                            return;
                        },
                        [clear_scope_function]() mutable
                        {
                            clear_scope_function();
                            return;
                        },
                        [loop_ranges, context_ptr](foreach_Block &foreach_block) mutable
                        {
                            auto& context = *context_ptr;
                            auto &i = *foreach_block.i_ptr;
                            bool one_lt = false;
                            for (auto& loop_range_id : loop_ranges)
                            {
                                CommandParser::varize_str(loop_range_id, context);
                                auto loop_range = identify_var(context, loop_range_id);
                                auto loop_split = split_string(loop_range, ";");
                                if (i < loop_split.size()) {
                                    one_lt = true;
                                    break;
                                }
                            }
                            return one_lt;
                        },
                        [loop_ranges, context_ptr](foreach_Block &foreach_block) mutable
                        {
                            (*foreach_block.i_ptr)++;
                            return;
                        });
                    break;
                }
                }

                auto &foreach_block = *foreach_block_sh_ptr;
                foreach_block.DefineArguments(0, cmd_arguments_size, cmd);
                foreach_block.foreach_cmd = cmd;
                context.block_stack.push(foreach_block_sh_ptr);
                context.evaluating_block_deque.push_front(&foreach_block);
                context.evaluating_block_cmd_deque.push_front(&foreach_block.foreach_cmd);
            }
            goto _begin;
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___evaluate_foreach_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(endforeach)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ENDFOREACH___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___endforeach_impl = [&](dsl_all_abstract_arguments)
    {
        if (context.block_stack.empty() || context.evaluating_block_deque.empty())
            throw std::runtime_error("[cmake] -- endforeach() without opening foreach()");
        auto block_ptr = context.block_stack.top();
        if (block_ptr->type != Block::foreach)
            throw std::runtime_error("[cmake] -- endforeach() without opening foreach()");
        auto eval_block_ptr = context.evaluating_block_deque.front();
        assert(block_ptr.get() == eval_block_ptr);
        auto foreach_block_ptr = dynamic_cast<foreach_Block *>(eval_block_ptr);
        foreach_block_ptr->clear_loop_block_function();
        foreach_block_ptr->loop_increment_function(*foreach_block_ptr);
        if (context.just_triggered_break || !foreach_block_ptr->loop_test_function(*foreach_block_ptr))
        {
            context.just_triggered_break = false;
            context.block_stack.pop();
            context.evaluating_block_deque.pop_front();
            return;
        }
        else
        {
            if (context.just_triggered_break)
                context.just_triggered_break = false;
            context.pos = foreach_block_ptr->foreach_cmd.pos;
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___endforeach_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(_break)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___BREAK___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___break_impl = [&]()
    {
        if (context.evaluating_block_deque.empty())
            throw std::runtime_error("[cmake] -- break() used and not within evaluable block");
        auto eval_deque_size = context.evaluating_block_deque.size();
        size_t block_i = 0;
        for (; block_i < eval_deque_size; block_i++)
        {
            auto block_ptr = context.evaluating_block_deque[block_i];
            assert(block_ptr);
            auto type = block_ptr->type;
            if (type == Block::foreach)
            {
                // context.breaking_out_of_block = block_ptr;
                context.just_triggered_break = true;
                break;
            }
        }
        if (block_i == eval_deque_size)
            throw std::runtime_error("[cmake] -- break() used and not within evaluable block");
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___break_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(_continue)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___CONTINUE___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___continue_impl = [&]()
    {
        if (context.evaluating_block_deque.empty())
            throw std::runtime_error("[cmake] -- continue() used and not within evaluable block");
        auto eval_deque_size = context.evaluating_block_deque.size();
        size_t block_i = 0;
        for (; block_i < eval_deque_size; block_i++)
        {
            auto block_ptr = context.evaluating_block_deque[block_i];
            assert(block_ptr);
            auto type = block_ptr->type;
            if (type == Block::foreach)
            {
                // context.continuing_in_block = block_ptr;
                context.just_triggered_continue = true;
                break;
            }
        }
        if (block_i == eval_deque_size)
            throw std::runtime_error("[cmake] -- continue() used and not within evaluable block");
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___continue_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(_if)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___IF___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___if_impl = [&](dsl_all_abstract_arguments)
    {
        auto is_elseif = (cmd.name == "elseif");
        if (is_elseif && context.if_depth == context.valid_if_depth)
        {
            context.valid_if_depth++;
            return;
        }
        else if (is_elseif)
        {
            context.valid_if_depth--;
        }
        auto is_if = (cmd.name == "if");
        if (is_if)
            context.if_depth++;
        ConditionNode condition;
        condition.ParseConditions(*this, cmd_arguments_size, cmd);
        if (condition.Evaluate(*this) || is_elseif)
            context.valid_if_depth++;
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___if_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(_else)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ELSE___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___else_impl = [&](dsl_all_abstract_arguments)
    {
        context.valid_if_depth++;
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___else_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(endif)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ENDIF___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto ___endif_impl = [&](dsl_all_abstract_arguments)
    {
        bool if_gt_valid = (context.if_depth > context.valid_if_depth);
        context.if_depth--;
        if (!if_gt_valid)
        {
            context.valid_if_depth--;
            if (context.if_depth == 0)
                context.valid_if_depth = 0;
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___endif_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(add_library)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ADD_LIBRARY___";
    static ValueVector options = {
        "SHARED",
        "STATIC",
        "MODULE",
        "INTERFACE",
        "IMPORTED"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto add_library_impl = [&](dsl_all_abstract_arguments)
    {
        if (!cmd_arguments_size)
            return;
        auto &target_name = cmd.arguments[0];
        auto target = std::make_shared<Target>(Target::Type::Library, target_name);
        for (size_t i = 1; i < cmd_arguments_size; ++i)
        {
            auto &arg = cmd.arguments[i];
            target->addArgument(arg);
        }
        targets[target_name] = target;
        if (in_vec(options_set, "SHARED"))
            target->setShared();
        if (in_vec(options_set, "STATIC"))
            target->setStatic();
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, add_library_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(add_executable)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___ADD_EXECUTABLE___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto add_executable_impl = [&](dsl_all_abstract_arguments)
    {
        if (!cmd_arguments_size)
            return;
        auto &target_name = cmd.arguments[0];
        auto target = std::make_shared<Target>(Target::Type::Executable, target_name);
        for (size_t i = 1; i < cmd_arguments_size; ++i)
        {
            auto &arg = cmd.arguments[i];
            target->addArgument(arg);
        }
        targets[target_name] = target;
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, add_executable_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(target_include_directories)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___TARGET_INCLUDE_DIRECTORIES___";
    static ValueVector options = {
        "PRIVATE",
        "PUBLIC"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto target_include_directories_impl = [&](dsl_some_abstract_arguments)
    {
        if (cmd_arguments_size < 2)
            return;
        auto it = targets.find(cmd.arguments[0]);
        if (it == targets.end())
            return;
        for (size_t i = 1; i < cmd_arguments_size; ++i)
        {
            auto &arg = cmd.arguments[i];
            it->second->addIncludeDir(arg);
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, target_include_directories_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(target_link_libraries)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___TARGET_LINK_LIBRARIES___";
    static ValueVector options = {
        "PRIVATE",
        "PUBLIC"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto target_link_libraries_impl = [&](dsl_some_abstract_arguments)
    {
        if (cmd_arguments_size < 2)
            return;
        auto it = targets.find(cmd.arguments[0]);
        if (it == targets.end())
            return;
        for (size_t i = 1; i < cmd_arguments_size; ++i)
        {
            auto &arg = cmd.arguments[i];
            it->second->addLinkLib(arg);
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, target_link_libraries_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(message)
{
    static std::unordered_map<std::string, CMakeMessageType> msg_type_map = {
        {"", CMakeMessageType::UNSET},
        {"STATUS", CMakeMessageType::STATUS},
        {"WARNING", CMakeMessageType::WARNING},
        {"FATAL_ERROR", CMakeMessageType::FATAL_ERROR},
        {"AUTHOR_WARNING", CMakeMessageType::AUTHOR_WARNING},
    };
    auto &context = *context_sh_ptr;
    static std::string prefix = "___MESSAGE___";
    static ValueVector options = {
        "STATUS",
        "WARNING",
        "FATAL_ERROR",
        "AUTHOR_WARNING",
    };
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto message_impl = [&](dsl_all_abstract_arguments)
    {
        if (cmd_arguments_size < 1)
            return;
        auto cmake_msg_type = msg_type_map[options_set.empty() ? "" : options_set.front()];
        std::string args_concat;
        for (auto i = 0; i < cmd_arguments_size; i++)
        {
            std::string concat = "[cmake] ";
            if (!i && cmake_msg_type != CMakeMessageType::UNSET)
                concat += "-- ";
            auto arg = dequote(cmd.arguments[i]);
            replaceAll(arg, "\n", "\n[cmake]");
            concat += arg;
            if (i < cmd_arguments_size - 1)
            {
                concat += "\n";
            }
            args_concat += concat;
        }
        switch (cmake_msg_type)
        {
        case CMakeMessageType::UNSET:
        case CMakeMessageType::STATUS:
            std::cout << args_concat << std::endl;
            break;
        case CMakeMessageType::WARNING:
            std::cout << "[cmake] CMake Warning:\n"
                      << args_concat << std::endl;
            fflush(stdout);
            break;
        case CMakeMessageType::FATAL_ERROR:
            throw std::runtime_error(R"([cmake] CMake Error:
    )" + args_concat);
        case CMakeMessageType::AUTHOR_WARNING:
            std::cout << "[cmake] CMake Warning (dev)\n"
                      << args_concat << "\n[cmake] This warning is for project developers.  Use -Wno-dev to suppress it." << std::endl;
            fflush(stdout);
            break;
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, message_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(get_filename_component)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___GET_FILE_NAME_COMPONENT___";
    static ValueVector options = {
        "ABSOLUTE",
        "REALPATH",
        "PATH",
        "NAME"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto get_filename_component_impl = [&](dsl_all_abstract_arguments)
    {
        static std::unordered_map<std::string, std::function<void(size_t, const Command &, ParseContext &, const std::string &, const std::filesystem::path &)>> gfc_actions = {
            {"", [](auto cmd_arguments_size, auto &cmd, auto &context, const auto &varName, const auto &p)
             {
                 context.vars[varName] = p.string();
                 return;
             }},
            {"ABSOLUTE", [](auto cmd_arguments_size, auto &cmd, auto &context, const auto &varName, const auto &p)
             {
                 context.vars[varName] = std::filesystem::absolute(p).lexically_normal().string();
                 return;
             }},
            {"REALPATH", [](auto cmd_arguments_size, auto &cmd, auto &context, const auto &varName, const auto &p)
             {
                 try
                 {
                     context.vars[varName] = std::filesystem::weakly_canonical(p).string();
                 }
                 catch (const std::exception &)
                 {
                     // If even weakly_canonical somehow fails, fallback to absolute normalized
                     context.vars[varName] = std::filesystem::absolute(p).lexically_normal().string();
                 }
                 return;
             }},
            {"PATH", [](auto cmd_arguments_size, auto &cmd, auto &context, const auto &varName, const auto &p)
             {
                 context.vars[varName] = p.parent_path().string();
                 return;
             }},
            {"NAME", [](auto cmd_arguments_size, auto &cmd, auto &context, const auto &varName, const auto &p)
             {
                 context.vars[varName] = p.filename().string();
                 return;
             }},
        };

        if (cmd_arguments_size < 2)
            return;

        const std::string &varName = cmd.arguments[0];
        std::string inputPath = cmd.arguments[1];

        std::string mode;
        if (!options_set.empty())
            mode = options_set.front();

        auto p = std::filesystem::path(inputPath);

        gfc_actions[mode](cmd_arguments_size, cmd, context, varName, p);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, get_filename_component_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(project)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___PROJECT___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {
        "VERSION",
        "COMPAT_VERSION",
        "DESCRIPTION",
        "HOMEPAGE_URL"};
    static ValueVector multi_value_keywords = {
        "LANGUAGES"};
    auto project_impl = [&](dsl_all_abstract_arguments)
    {
        if (cmd_arguments_size < 1)
            return;
        name = cmd.arguments[0];

        if (in_vec(one_value_keywords_set, "VERSION"))
            version = context.vars["___PROJECT____VERSION"];
        if (in_vec(one_value_keywords_set, "COMPAT_VERSION"))
            compat_version = context.vars["___PROJECT____COMPAT_VERSION"];
        if (in_vec(one_value_keywords_set, "DESCRIPTION"))
            description = context.vars["___PROJECT____DESCRIPTION"];
        if (in_vec(one_value_keywords_set, "HOMEPAGE_URL"))
            homepage_url = context.vars["___PROJECT____HOMEPAGE_URL"];
        if (in_vec(multi_value_keywords_set, "LANGUAGES"))
            languages = split_string(context.vars["___PROJECT____LANGUAGES"], ";");
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, project_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(list)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___LIST___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto list_impl = [&](dsl_all_abstract_arguments)
    {
        static std::unordered_map<std::string, dz::function<void(ParseContext &, dsl_all_abstract_arguments_real)>> action_fns = {
            {"APPEND", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 1)
                     throw std::runtime_error("[cmake] -- arguments passed to list(APPEND <var_name> <input>...) is less than required: (1)");
                 auto arguments_data = cmd.arguments.data();
                 auto &var_name = arguments_data[0];
                 auto &var = context.vars[var_name];
                 for (size_t i = 1; i < cmd_arguments_size; i++)
                 {
                     auto val = arguments_data[i];
                     val = dequote(val);
                     if (!var.empty())
                         var += ";";
                     var += val;
                 }
                 return;
             }},
            {"REMOVE_ITEM", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 1)
                     throw std::runtime_error("[cmake] -- arguments passed to list(REMOVE_ITEM <var_name> <input>...) is less than required: (1)");
                 auto arguments_data = cmd.arguments.data();
                 auto &var_name = arguments_data[0];
                 auto &var = context.vars[var_name];
                 for (size_t i = 1; i < cmd_arguments_size; i++)
                 {
                     auto val = arguments_data[i];
                     val = dequote(val);
                     auto var_split = split_string(var, ";");
                     auto f_it = std::find(var_split.begin(), var_split.end(), val);
                     if (f_it != var_split.end())
                     {
                         var_split.erase(f_it);
                         var = join_string_vec(var_split, ";");
                     }
                 }
                 return;
             }},
            {"REMOVE_AT", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 2)
                     throw std::runtime_error("[cmake] -- arguments passed to list(REMOVE_AT <var_name> <at>) is less than required: (2)");
                 auto &var_name = cmd.arguments[0];
                 auto &at_str = cmd.arguments[1];
                 auto &var_val = context.vars[var_name];
                 auto var_split = split_string(var_val, ";");
                 auto at = std::stoll(at_str);
                 auto var_split_size = var_split.size();
                 if (at >= var_split_size)
                     throw std::runtime_error("[cmake] -- list(REMOVE_AT <var> <at>), at(" + at_str + ") >= list_size(" + std::to_string(var_split_size) + ").");
                 var_split.erase(var_split.begin() + at);
                 context.vars[var_name] = join_string_vec(var_split, ";");
                 return;
             }},
            {"LENGTH", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 2)
                     throw std::runtime_error("[cmake] -- arguments passed to list(LENGTH <check_variable> <output_variable>) is less than required: (2)");
                 auto &check_variable = cmd.arguments[0];
                 auto output_variable = dequote(cmd.arguments[1]);
                 std::string output_val("0");
                 auto check_it = context.vars.find(check_variable);
                 if (check_it != context.vars.end())
                 {
                     auto &check_val = check_it->second;
                     auto check_split = split_string(check_val, ";");
                     output_val = std::to_string(check_split.size());
                 }
                 context.vars[output_variable] = output_val;
             }},
            {"JOIN", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 3)
                     throw std::runtime_error("[cmake] -- arguments passed to list(JOIN <list> <glue> <output_variable>) is less than required: (3)");
                 auto &list = cmd.arguments[0];
                 auto glue = dequote(cmd.arguments[1]);
                 auto &output_variable = cmd.arguments[2];
                 auto list_it = context.vars.find(list);
                 std::string joined;
                 if (list_it != context.vars.end())
                 {
                     auto &list_val = list_it->second;
                     auto list_split = split_string(list_val, ";");
                     joined = join_string_vec(list_split, glue);
                 }
                 context.vars[output_variable] = joined;
             }}};
        if (cmd_arguments_size < 1)
            throw std::runtime_error("[cmake] -- no arguments passed to list()");
        auto action = cmd.arguments[0];
        auto action_it = action_fns.find(action);
        if (action_it == action_fns.end())
            throw std::runtime_error("[cmake] -- <ACTION> '" + action + "' is not a valid list(<ACTION>)");
        auto action_cmd = cmd;
        action_cmd.arguments.erase(action_cmd.arguments.begin());
        auto action_cmd_arguments_size = cmd_arguments_size;
        action_cmd_arguments_size--;
        action_it->second(context, action_cmd_arguments_size, action_cmd, options_set, one_value_keywords_set, multi_value_keywords_set);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, list_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(cmake_policy)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___CMAKE_POLICY___";
    static ValueVector options = {
        "PUSH",
        "POP",
        "SET",
        "GET"};
    static ValueVector one_value_keywords = {
        "VERSION"};
    static ValueVector multi_value_keywords = {};
    auto policy_impl = [&](dsl_all_abstract_arguments)
    {
        static DSL_Map_With_Context policy_actions = {
            {"PUSH", [](auto cmd_arguments_size, auto &cmd, auto &context)
             {
                 context.policy_stack.emplace_front(); // push empty scope
                 context.policy_push_just_called = true;
                 return;
             }},
            {"POP", [](auto cmd_arguments_size, auto &cmd, auto &context)
             {
                 if (context.policy_stack.empty())
                     return;

                 auto policy_sh_ptr = context.policy_stack.front();
                 context.policy_stack.pop_front(); // discard scope
                 auto &policy = *policy_sh_ptr;
                 context.policy_set_map.erase(policy.policy_name);
                 return;
             }},
            {"SET", [](auto cmd_arguments_size, auto &cmd, auto &context)
             {
                 if (cmd_arguments_size < 2)
                     return;

                 auto &policy_name = cmd.arguments[0];
                 auto &policy_value = cmd.arguments[1];

                 auto policy_sh_ptr = std::make_shared<Policy>();
                 auto &policy = *policy_sh_ptr;
                 policy.policy_name = policy_name;
                 policy.policy_value = policy_value;

                 if (context.policy_push_just_called)
                 {
                     context.policy_stack.front() = policy_sh_ptr;
                     context.policy_push_just_called = false;
                 }
                 context.policy_set_map[policy_name] = policy_sh_ptr;
                 return;
             }},
            {"GET", [](auto cmd_arguments_size, auto &cmd, auto &context)
             {
                 // TODO:
             }},
            {"VERSION", [](auto cmd_arguments_size, auto &cmd, auto &context)
             {
                 // TODO:
             }}};

        if (!options_set.empty())
        {
            auto &option = options_set.front();
            policy_actions[option](cmd_arguments_size, cmd, context);
        }
        else if (!one_value_keywords.empty())
        {
            auto &one_value_keyword = one_value_keywords_set.front();
            policy_actions[one_value_keyword](cmd_arguments_size, cmd, context);
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, policy_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(set)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___SET___";
    static ValueVector options = {
        "PARENT_SCOPE"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto set_impl = [&](dsl_some_abstract_arguments)
    {
        if (cmd_arguments_size < 2)
            return;
        auto parent_scope = context.vars["___SET____PARENT_SCOPE"] == "TRUE";
        auto &var_name = cmd.arguments[0];
        if (var_name.empty())
            return;
        auto &vars = context_sh_ptr->vars;
        auto &var = vars[var_name];
        var.clear();
        for (size_t i = 1; i < cmd_arguments_size; i++)
        {
            if (!var.empty())
                var += ";";
            auto val = dequote(cmd.arguments[i]);
            var += val;
        }
        if (parent_scope)
            context.mark_var(var_name);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, set_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(unset)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___UNSET___";
    static ValueVector options = {
        "CACHE",
        "PARENT_SCOPE"};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto unset_impl = [&](dsl_some_abstract_arguments)
    {
        if (cmd_arguments_size < 2)
            return;
        auto parent_scope = context.vars["___UNSET____PARENT_SCOPE"] == "TRUE";
        auto cache = context.vars["___UNSET____CACHE"] == "TRUE";
        auto &var_name = cmd.arguments[0];
        if (var_name.empty())
            return;
        auto &vars = context.vars;
        auto var_it = vars.find(var_name);
        if (var_it == vars.end())
            return;
        vars.erase(var_it);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, unset_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(find_path)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___FIND_PATH___";
    static ValueVector options = {
        "REQUIRED",
        "OPTIONAL",
        "NO_DEFAULT_PATH",
        "NO_PACKAGE_ROOT_PATH",
        "NO_CMAKE_PATH",
        "NO_CMAKE_ENVIRONMENT_PATH",
        "NO_CMAKE_SYSTEM_PATH",
        "NO_CMAKE_INSTALL_PREFIX",
        "NO_SYSTEM_ENVIRONMENT_PATH",
        "NO_CMAKE_FIND_ROOT_PATH",
        "ONLY_CMAKE_FIND_ROOT_PATH",
        "NO_CACHE",
        "CMAKE_FIND_ROOT_PATH_BOTH"};
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VALIDATOR",
        "DOC"};
    static ValueVector multi_value_keywords = {
        "NAMES",
        "HINTS",
        "PATHS"};
    auto find_path_impl = [&](dsl_some_abstract_arguments)
    {
        if (cmd_arguments_size < 1)
            return;
        auto &var_name = cmd.arguments[0];
        if (cmd_arguments_size > 2)
            throw std::runtime_error(R"([cmake] -- find_path arguments count should never be > 2)");
        auto names = split_string(context.vars["___FIND_PATH____NAMES"], ";");
        if (cmd_arguments_size == 2)
        {
            auto &one_name = cmd.arguments[1];
            names.push_back(one_name);
        }
        auto hints = split_string(context.vars["___FIND_PATH____HINTS"], ";");
        for (auto &hint_dir : hints)
        {
            auto hint_path = std::filesystem::path(hint_dir);
            for (auto &name_path : names)
            {
                auto concat_path = (hint_path / name_path);
                if (std::filesystem::exists(concat_path))
                {
                    context.vars[var_name] = hint_dir;
                    context.mark_var(var_name);
                    return;
                }
            }
        }
        context.vars[var_name] = (var_name + "-NOTFOUND");
        context.mark_var(var_name);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_path_impl);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(find_library)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___FIND_LIBRARY___";
    static ValueVector options = {
        "REQUIRED",
        "OPTIONAL",
        "NO_DEFAULT_PATH",
        "NO_PACKAGE_ROOT_PATH",
        "NO_CMAKE_PATH",
        "NO_CMAKE_ENVIRONMENT_PATH",
        "NO_CMAKE_SYSTEM_PATH",
        "NO_CMAKE_INSTALL_PREFIX",
        "NO_SYSTEM_ENVIRONMENT_PATH",
        "NO_CMAKE_FIND_ROOT_PATH",
        "ONLY_CMAKE_FIND_ROOT_PATH",
        "NO_CACHE",
        "CMAKE_FIND_ROOT_PATH_BOTH"};
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VALIDATOR",
        "DOC"};
    static ValueVector multi_value_keywords = {
        "PATH_SUFFIXES",
        "NAMES",
        "HINTS",
        "PATHS"};
    auto find_library_impl = [&](dsl_some_abstract_arguments)
    {
        static ValueVector default_suffixes = {
#if defined(_WIN32)
            ".lib",
            ".dll"
#elif defined(__linux__)
            ".so",
            ".a"
#elif defined(MACOS)
            ".a",
            ".dylib"
#endif
        };
        if (cmd_arguments_size < 1)
            return;
        auto &var_name = cmd.arguments[0];
        if (cmd_arguments_size > 2)
            throw std::runtime_error(R"([cmake] -- find_library arguments count should never be > 2)");
        auto all_suffixes = split_string(context.vars["___FIND_LIBRARY____PATH_SUFFIXES"], ";");
        all_suffixes.insert(all_suffixes.end(), default_suffixes.begin(), default_suffixes.end());
        auto names = split_string(context.vars["___FIND_LIBRARY____NAMES"], ";");
        if (cmd_arguments_size == 2)
        {
            auto &one_name = cmd.arguments[1];
            names.push_back(one_name);
        }
        auto hints = split_string(context.vars["___FIND_LIBRARY____HINTS"], ";");
        auto paths = split_string(context.vars["___FIND_LIBRARY____PATHS"], ";");
        hints.insert(hints.end(), paths.begin(), paths.end());
        for (auto &hint_dir : hints)
        {
            auto hint_path = std::filesystem::path(hint_dir);
            for (auto &name_string : names)
            {
                for (auto &suffix : all_suffixes)
                {
                    auto library_path = (hint_path / (name_string + suffix));
                    if (std::filesystem::exists(library_path))
                    {
                        context.vars[var_name] = library_path.string();
                        context.mark_var(var_name);
                        return;
                    }
                }
            }
        }
        context.vars[var_name] = (var_name + "-NOTFOUND");
        context.mark_var(var_name);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_library_impl);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(find_program)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___FIND_PROGRAM___";
    static ValueVector options = {
        "REQUIRED",
        "OPTIONAL",
        "NO_DEFAULT_PATH",
        "NO_PACKAGE_ROOT_PATH",
        "NO_CMAKE_PATH",
        "NO_CMAKE_ENVIRONMENT_PATH",
        "NO_CMAKE_SYSTEM_PATH",
        "NO_CMAKE_INSTALL_PREFIX",
        "NO_SYSTEM_ENVIRONMENT_PATH",
        "NO_CMAKE_FIND_ROOT_PATH",
        "ONLY_CMAKE_FIND_ROOT_PATH",
        "NO_CACHE",
        "CMAKE_FIND_ROOT_PATH_BOTH"};
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VALIDATOR",
        "DOC"};
    static ValueVector multi_value_keywords = {
        "PATH_SUFFIXES",
        "NAMES",
        "HINTS",
        "PATHS"};
    auto find_program_impl = [&](dsl_some_abstract_arguments)
    {
        static ValueVector default_suffixes = {
#if defined(_WIN32)
            ".exe",
#endif
            ""};
        if (cmd_arguments_size < 1)
            return;
        auto &var_name = cmd.arguments[0];
        if (cmd_arguments_size > 2)
            throw std::runtime_error(R"([cmake] -- find_library arguments count should never be > 2)");
        auto all_suffixes = split_string(context.vars["___FIND_PROGRAM____PATH_SUFFIXES"], ";");
        all_suffixes.insert(all_suffixes.end(), default_suffixes.begin(), default_suffixes.end());
        auto names = split_string(context.vars["___FIND_PROGRAM____NAMES"], ";");
        if (cmd_arguments_size == 2)
        {
            auto &one_name = cmd.arguments[1];
            names.push_back(one_name);
        }
        auto hints = split_string(context.vars["___FIND_PROGRAM____HINTS"], ";");
        auto paths = split_string(context.vars["___FIND_PROGRAM____PATHS"], ";");
        hints.insert(hints.end(), paths.begin(), paths.end());
        for (auto &hint_dir : hints)
        {
            auto hint_path = std::filesystem::path(hint_dir);
            for (auto &name_string : names)
            {
                for (auto &suffix : all_suffixes)
                {
                    auto program_path = (hint_path / (name_string + suffix));
                    if (std::filesystem::exists(program_path))
                    {
                        context.vars[var_name] = program_path.string();
                        context.vars[var_name + "_FOUND"] = "TRUE";
                        context.mark_var(var_name);
                        return;
                    }
                }
            }
        }
        context.vars[var_name] = (var_name + "-NOTFOUND");
        context.vars[var_name + "_FOUND"] = "FALSE";
        context.mark_var(var_name);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_program_impl);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(mark_as_advanced)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___MARK_AS_ADVANCED___";
    static ValueVector options = {
        "CLEAR",
        "FORCE",
    };
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto mark_as_advanced_impl = [&](dsl_some_abstract_arguments)
    {
        auto &clear = context.vars["___MARK_AS_ADVANCED____CLEAR"];
        auto mark_bool = clear != "TRUE";
        auto &force = context.vars["___MARK_AS_ADVANCED____FORCE"];
        auto force_bool = force == "TRUE";
        for (size_t i = 0; i < cmd_arguments_size; i++)
        {
            auto &var_name = cmd.arguments[i];
            context.mark_var(var_name, mark_bool || force_bool);
        }
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, mark_as_advanced_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(cmake_parse_arguments)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___CMAKE_PARSE_COMMANDS___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {"PARSE_ARGV"};
    static ValueVector multi_value_keywords = {};
    auto cmake_parse_arguments_impl = [&](dsl_some_abstract_arguments)
    {
        if (context.evaluating_block_deque.empty())
            throw std::runtime_error("[cmake] -- cmake_parse_arguments must be called within a function() block");
        auto block_ptr = context.evaluating_block_deque.front();
        auto block_cmd_ptr = context.evaluating_block_cmd_deque.front();
        auto &block = *block_ptr;
        if (block.type != Block::function)
            throw std::runtime_error("[cmake] -- cmake_parse_arguments must be called within a function() block");

        auto &function_block = dynamic_cast<function_Block &>(block);

        auto cmd_arguments_data = cmd.arguments.data();
        std::string prefix_str((cmd_arguments_size >= 1) ? dequote(cmd_arguments_data[0]) : "");
        std::string options_str((cmd_arguments_size >= 2) ? dequote(cmd_arguments_data[1]) : "");
        std::string one_value_keywords_str((cmd_arguments_size >= 3) ? dequote(cmd_arguments_data[2]) : "");
        std::string multi_value_keywords_str((cmd_arguments_size >= 4) ? dequote(cmd_arguments_data[3]) : "");

        auto options = split_string(options_str, ";");
        auto one_value_keywords = split_string(one_value_keywords_str, ";");
        auto multi_value_keywords = split_string(multi_value_keywords_str, ";");

        auto [new_arguments,
              new_all_keys_and_vals,
              new_options,
              new_one_value_keywords,
              new_multi_value_keywords] = parse_all_keys_and_vals(context,
                                                                  prefix_str,
                                                                  multi_value_keywords,
                                                                  one_value_keywords,
                                                                  options,
                                                                  block_cmd_ptr->arguments.size(),
                                                                  *block_cmd_ptr);

        insert_to_map_from_map(context.vars, new_all_keys_and_vals);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, cmake_parse_arguments_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(file)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___FILE___";
    static ValueVector options = {
        "EXPAND_TILDE",
        "NEWLINE_CONSUME",
        "NO_HEX_CONVERSION",
        "HEX",
    };
    static ValueVector one_value_keywords = {
        "OFFSET",
        "LIMIT",
        "OUTPUT",
        "CONTENT",
        "DESTINATION",
        "BASE_DIRECTORY",
        "LENGTH_MAXIMUM",
        "LENGTH_MINIMUM",
        "LIMIT_COUNT",
        "LIMIT_INPUT",
        "LIMIT_OUTPUT",
        "REGEX",
        "ENCODING"};
    static ValueVector multi_value_keywords = {
        "PERMISSIONS"};
    auto file_impl = [&](dsl_all_abstract_arguments)
    {
        static std::unordered_map<std::string, dz::function<void(ParseContext &, dsl_all_abstract_arguments_real)>> action_fns = {
            {"READ", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 2)
                     throw std::runtime_error("[cmake] -- arguments passed to file(READ <filename> <variable> [OFFSET <offset>] [LIMIT <max-in>] [HEX]) is less than required: (2)");
                 auto &filename = cmd.arguments[0];
                 auto &variable = cmd.arguments[1];
                 auto offset_used = in_vec(one_value_keywords_set, "OFFSET");
                 auto limit_used = in_vec(one_value_keywords_set, "LIMIT");
                 long long offset = offset_used ? std::stoll(dequote(context.vars["___FILE____OFFSET"])) : 0;
                 long long limit = limit_used ? std::stoll(dequote(context.vars["___FILE____LIMIT"])) : -1;
                 auto file_data = get_file_data(filename, offset, limit);
                 context.vars[variable] = file_data;
                 return;
             }},
            {"STRINGS", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 2)
                     throw std::runtime_error("[cmake] -- arguments passed to file(STRINGS <filename> <variable> <options>...) is less than required: (2)");
                 auto &filename = cmd.arguments[0];
                 auto &variable = cmd.arguments[1];
                 auto file_data = get_file_data(filename);
                 replaceAll(file_data, "\r", "");
                 auto file_split = split_string(file_data, "\n");
                 std::vector<std::string> filtered_split = {};
                 auto regex_used = in_vec(one_value_keywords_set, "REGEX");
                 if (regex_used)
                 {
                     filtered_split.reserve(file_split.size());
                     std::regex rgx(dequote(context.vars["___FILE____REGEX"]));
                     for (auto &str : file_split)
                     {
                         std::smatch match;
                         if (std::regex_search(str.cbegin(), str.cend(), match, rgx))
                         {
                             filtered_split.push_back(str);
                         }
                     }
                 }
                 else
                 {
                     filtered_split = file_split;
                 }
                 context.vars[variable] = join_string_vec(filtered_split, ";");
                 return;
             }},
            // TODO: Replace with supported hashing providers
            {"<HASH>", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"TIMESTAMP", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"WRITE", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 2)
                     throw std::runtime_error("[cmake] -- arguments passed to file(WRITE <filename> <content>...) is less than required: (2)");
                 auto &filename = cmd.arguments[0];
                 std::ofstream ofst(filename, std::ios::out | std::ios::binary | std::ios::trunc);
                 for (size_t arg_i = 1; arg_i < cmd_arguments_size; arg_i++)
                     ofst << dequote(cmd.arguments[arg_i]);
                 return;
             }},
            {"APPEND", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 2)
                     throw std::runtime_error("[cmake] -- arguments passed to file(APPEND <filename> <content>...) is less than required: (2)");
                 auto &filename = cmd.arguments[0];
                 std::ofstream ofst(filename, std::ios::out | std::ios::binary | std::ios::app);
                 for (size_t arg_i = 1; arg_i < cmd_arguments_size; arg_i++)
                     ofst << dequote(cmd.arguments[arg_i]);
                 return;
             }},
            {"TOUCH", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"TOUCH_NOCREATE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"GENERATE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"CONFIGURE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"GLOB", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"GLOB_RECURSE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"MAKE_DIRECTORY", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"REMOVE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"REMOVE_RECURSE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"RENAME", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"COPY_FILE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"COPY", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"INSTALL", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"SIZE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"READ_SYMLINK", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"CREATE_LINK", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"CHMOD", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"CHMOD_RECURSE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"REAL_PATH", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"RELATIVE_PATH", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"TO_CMAKE_PATH", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"TO_NATIVE_PATH", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"DOWNLOAD", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"UPLOAD", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"LOCK", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"ARCHIVE_CREATE", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"ARCHIVE_EXTRACT", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
            {"GET_RUNTIME_DEPENDENCIES", [](auto &context, dsl_all_abstract_arguments)
             {
                 return;
             }},
        };
        if (cmd_arguments_size < 1)
            throw std::runtime_error("[cmake] -- no arguments passed to file()");
        auto action = cmd.arguments[0];
        auto action_it = action_fns.find(action);
        if (action_it == action_fns.end())
            throw std::runtime_error("[cmake] -- <ACTION> '" + action + "' is not a valid file(<ACTION>)");
        auto action_cmd = cmd;
        action_cmd.arguments.erase(action_cmd.arguments.begin());
        auto action_cmd_arguments_size = cmd_arguments_size;
        action_cmd_arguments_size--;
        action_it->second(context, action_cmd_arguments_size, action_cmd, options_set, one_value_keywords_set, multi_value_keywords_set);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, file_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

dsl_fn_project_def(string)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___STRING___";
    static ValueVector options = {};
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto string_impl = [&](dsl_all_abstract_arguments)
    {
        static std::unordered_map<std::string, dz::function<void(ParseContext &, dsl_all_abstract_arguments_real)>> action_fns = {
            {"REGEX", [](auto &context, dsl_all_abstract_arguments)
             {
                 if (cmd_arguments_size < 1)
                     throw std::runtime_error("[cmake] -- arguments passed to string(REGEX <COMMAND> <argument>...) is less than required: (1)");
                 auto command = cmd.arguments[0];

                 static std::unordered_map<std::string, dz::function<void(ParseContext &, dsl_all_abstract_arguments_real)>> command_fns = {
                     {"MATCH", [](auto &context, dsl_all_abstract_arguments)
                      {
                          if (cmd_arguments_size < 3)
                              throw std::runtime_error("[cmake] -- arguments passed to string(REGEX MATCH <regular_expression> <output_variable> <input> [<input>...]) is less than required: (3)");
                          auto regular_expression = dequote(cmd.arguments[0]);
                          auto output_variable = dequote(cmd.arguments[1]);
                          std::regex rgx(regular_expression);
                          std::string input;
                          for (size_t arg_i = 2; arg_i < cmd_arguments_size; arg_i++)
                              input += dequote(cmd.arguments[arg_i]);
                          std::smatch match;
                          if (std::regex_search(input, match, rgx))
                              context.vars[output_variable] = match.str();
                          else
                              context.vars[output_variable] = "";
                          return;
                      }},
                     {"MATCHALL", [](auto &context, dsl_all_abstract_arguments)
                      {
                          if (cmd_arguments_size < 3)
                              throw std::runtime_error("[cmake] -- arguments passed to string(REGEX MATCHALL <regular_expression> <output_variable> <input> [<input>...]) is less than required: (3)");
                          auto regular_expression = dequote(cmd.arguments[0]);
                          auto output_variable = dequote(cmd.arguments[1]);
                          std::regex rgx(regular_expression);
                          std::string input;
                          for (size_t arg_i = 2; arg_i < cmd_arguments_size; arg_i++)
                              input += dequote(cmd.arguments[arg_i]);
                          std::sregex_iterator it(input.begin(), input.end(), rgx);
                          std::sregex_iterator end;
                          std::vector<std::string> matches;
                          for (; it != end; ++it)
                              matches.push_back(it->str());
                          context.vars[output_variable] = join_string_vec(matches, ";");
                          return;
                      }},

                     {"REPLACE", [](auto &context, dsl_all_abstract_arguments)
                      {
                          if (cmd_arguments_size < 4)
                              throw std::runtime_error("[cmake] -- arguments passed to string(REGEX REPLACE <regular_expression> <replacement_expression> <output_variable> <input> [<input>...]) is less than required: (4)");
                          auto regular_expression = dequote(cmd.arguments[0]);
                          auto replacement_expression = dequote(cmd.arguments[1]);
                          auto &output_variable = cmd.arguments[2];
                          std::regex rgx(regular_expression);
                          std::string input;
                          for (size_t arg_i = 3; arg_i < cmd_arguments_size; arg_i++)
                              input += dequote(cmd.arguments[arg_i]);
                          std::string result = std::regex_replace(input, rgx, replacement_expression);
                          context.vars[output_variable] = result;
                          return;
                      }}};

                 auto command_it = command_fns.find(command);
                 if (command_it == command_fns.end())
                     throw std::runtime_error("[cmake] -- <COMMAND> '" + command + "' is not a valid string(REGEX <COMMAND>)");
                 auto command_cmd = cmd;
                 command_cmd.arguments.erase(command_cmd.arguments.begin());
                 auto command_cmd_arguments_size = cmd_arguments_size;
                 command_cmd_arguments_size--;
                 command_it->second(context, command_cmd_arguments_size, command_cmd, options_set, one_value_keywords_set, multi_value_keywords_set);
             }},
            {"APPEND", [](auto &context, dsl_all_abstract_arguments) {
                if (cmd_arguments_size < 2)
                    throw std::runtime_error("[cmake] -- arguments passed to string(APPEND <output_variable> <input> [<input>...]) is less than required: (2)");
                auto& output_variable = cmd.arguments[0];
                auto& output_val = context.vars[output_variable];
                auto arguments_data = cmd.arguments.data();
                auto arguments_size = cmd.arguments.size();
                for (size_t arg_i = 1; arg_i < arguments_size; arg_i++)
                    output_val += dequote(arguments_data[arg_i]);
                return;
             }},
        };
        if (cmd_arguments_size < 1)
            throw std::runtime_error("[cmake] -- no arguments passed to string()");
        auto action = cmd.arguments[0];
        auto action_it = action_fns.find(action);
        if (action_it == action_fns.end())
            throw std::runtime_error("[cmake] -- <ACTION> '" + action + "' is not a valid string(<ACTION>)");
        auto action_cmd = cmd;
        action_cmd.arguments.erase(action_cmd.arguments.begin());
        auto action_cmd_arguments_size = cmd_arguments_size;
        action_cmd_arguments_size--;
        action_it->second(context, action_cmd_arguments_size, action_cmd, options_set, one_value_keywords_set, multi_value_keywords_set);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, string_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

void dz::cmake::Project::print()
{
    std::cout << "Project Name: " << name << std::endl;
    auto target_c = 1;
    for (auto &[target_name, target_sh_ptr] : targets)
    {
        std::cout << "\tTarget(" << target_c++ << "): " << target_name << std::endl;
        auto &target = *target_sh_ptr;
        std::cout << "\t\tType: " << target.GetTypeStr() << std::endl
                  << "\t\tLink Type: " << target.GetLinkTypeStr() << std::endl;
        std::cout << "\tSources:" << std::endl;
        auto s_c = 1;
        for (auto &source_file : target.sources)
        {
            std::cout << "\t\t" << s_c++ << ": " << source_file << std::endl;
        }
        std::cout << "\tIncludes:" << std::endl;
        auto i_c = 1;
        for (auto &include_dir : target.includeDirs)
        {
            std::cout << "\t\t" << i_c++ << ": " << include_dir << std::endl;
        }
        std::cout << "\tLink Libraries:" << std::endl;
        auto l_c = 1;
        for (auto &link_library : target.linkLibs)
        {
            std::cout << "\t\t" << l_c++ << ": " << link_library << std::endl;
        }
    }
}

namespace dz::cmake
{
    DSL_Map Project::generate_dsl_map()
    {
        DSL_Map map = {
            dsl_entry(macro),
            dsl_entry(endmacro),
            dsl_entry(function),
            dsl_entry(endfunction),
            dsl_entry(foreach),
            dsl_entry(endforeach),
            dsL_entry_str("if", _if),
            dsL_entry_key(elseif, _if),
            dsL_entry_str("else", _else),
            dsL_entry_str("continue", _continue),
            dsL_entry_str("break", _break),
            dsL_entry_str("return", _return),
            dsl_entry(endif),
            dsl_entry(add_library),
            dsl_entry(add_executable),
            dsl_entry(target_include_directories),
            dsl_entry(target_link_libraries),
            dsl_entry(project),
            dsl_entry(find_package),
            dsl_entry(message),
            dsl_entry(get_filename_component),
            dsl_entry(list),
            dsl_entry(cmake_policy),
            dsl_entry(set),
            dsl_entry(unset),
            dsl_entry(find_path),
            dsl_entry(find_library),
            dsl_entry(find_program),
            dsl_entry(mark_as_advanced),
            dsl_entry(cmake_parse_arguments),
            dsl_entry(file),
            dsl_entry(string),
        };
        return map;
    }
}

dz::cmake::ValueVector dz::cmake::Project::determine_find_package_dirs(const std::string &pkg)
{
    auto x_dir = pkg + "_DIR";
    auto &vars = context_sh_ptr->vars;
    auto it = vars.find(x_dir);
    if (it != vars.end())
    {
        return {it->second};
    }

    static std::string _MODULE_PATH_STR = "CMAKE_MODULE_PATH";
    dz::cmake::ValueVector _module_path_split;
    auto _mod_it = vars.find(_MODULE_PATH_STR);
    if (_mod_it != vars.end())
    {
        _module_path_split = split_string(_mod_it->second, ";");
    }
    auto default_path = "./" + pkg + "/lib/cmake/" + pkg;
#if defined(_WIN32)
    dz::cmake::ValueVector candidates = {
        "C:/Program Files/" + pkg + "/lib/cmake/" + pkg,
        "C:/Program Files (x86)/" + pkg + "/lib/cmake/" + pkg};
    default_path = "C:/Program Files/" + pkg + "/lib/cmake/" + pkg;
#elif defined(__linux__)
    dz::cmake::ValueVector candidates = {
        "/usr/lib/cmake/" + pkg,
        "/usr/local/lib/cmake/" + pkg,
        "/usr/share/cmake/" + pkg};
    default_path = "/usr/lib/cmake/" + pkg; // fallback
#elif defined(__APPLE__) && !defined(TARGET_OS_IPHONE)
    dz::cmake::ValueVector candidates = {
        "/usr/local/lib/cmake/" + pkg,
        "/opt/homebrew/lib/cmake/" + pkg,
        "/usr/lib/cmake/" + pkg};
    default_path = "/usr/local/lib/cmake/" + pkg; // fallback
#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // iOS runtime: inside app bundle (Resources)
    // e.g. <AppBundle>/Resources/cmake/<Pkg>
    dz::cmake::ValueVector candidates = {};
    default_path = "cmake/" + pkg; // relative to bundle Resources
    auto it_prefix = vars.find("CMAKE_RUNTIME_PREFIX");
    if (it_prefix != vars.end())
        return it_prefix->second + "/cmake/" + pkg;
#elif defined(__ANDROID__)
    // Android runtime: inside APK assets or app files dir
    // e.g. /data/data/<app>/files/cmake/<Pkg> or assets/cmake/<Pkg>
    dz::cmake::ValueVector candidates = {};
    default_path = "assets/cmake/" + pkg; // relative to APK assets
    auto it_prefix = vars.find("CMAKE_RUNTIME_PREFIX");
    if (it_prefix != vars.end())
        return it_prefix->second + "/cmake/" + pkg;
#endif
    candidates.insert(candidates.begin(), _module_path_split.begin(), _module_path_split.end());
    candidates.push_back(default_path);
    dz::cmake::ValueVector found_candidates;
    for (auto &c : candidates)
    {
        if (std::filesystem::exists(c))
        {
            found_candidates.push_back(c);
        }
    }
    return found_candidates;
}

dsl_fn_project_def(find_package)
{
    auto &context = *context_sh_ptr;
    static std::string prefix = "___FIND_PACKAGE";
    static ValueVector options = {
        "REQUIRED",
        "QUIET",
        "EXACT"};
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VERSION"};
    static ValueVector multi_value_keywords = {
        "COMPONENTS"};
    auto find_package_impl = [&](dsl_some_abstract_arguments, const auto &options_set, const auto &one_value_keywords_set, const auto &multi_value_keywords_set)
    {
        if (cmd_arguments_size < 1)
            return;
        auto pkg = cmd.arguments[0];
        bool required = in_vec(options_set, "REQUIRED");
        bool quiet = in_vec(options_set, "QUIET");
        bool exact = in_vec(options_set, "EXACT");
        auto candidate_dirs = determine_find_package_dirs(pkg);
        if (candidate_dirs.empty())
        {
            if (!required)
                return;
        _throw:
            throw std::runtime_error(
                R"(
    [cmake] CMake Error:
    [cmake]   By not providing "Find)" +
                pkg + R"(.cmake" in CMAKE_MODULE_PATH this project has
    [cmake]   asked CMake to find a package configuration file provided by ")" +
                pkg + R"(", but
    [cmake]   CMake did not find one.
    [cmake] 
    [cmake]   Could not find a package configuration file provided by ")" +
                pkg + R"(" with any of
    [cmake]   the following names:
    [cmake] 
    [cmake]     )" +
                pkg + R"(Config.cmake
    [cmake]     )" +
                pkg + R"(-config.cmake
    [cmake] 
    [cmake]   Add the installation prefix of ")" +
                pkg + R"(" to CMAKE_PREFIX_PATH or set ")" + pkg + R"(_DIR"
    [cmake]   to a directory containing one of the above files.  If ")" +
                pkg + R"(" provides a
    [cmake]   separate development package or SDK, be sure it has been installed.
    )");
        }
        std::string found_config;
        std::string f_dir;
        std::vector<std::pair<std::string, size_t>> f_candidates;
        auto candidate_dirs_size = candidate_dirs.size();
        auto candidate_dirs_data = candidate_dirs.data();
        for (size_t c_index = 0; c_index < candidate_dirs_size; c_index++)
        {
            auto &c_dir = candidate_dirs_data[c_index];
            f_candidates.push_back({c_dir + "/" + pkg + "Config.cmake", c_index});
            f_candidates.push_back({c_dir + "/" + pkg + "-config.cmake", c_index});
            f_candidates.push_back({c_dir + "/Find" + pkg + ".cmake", c_index});
        };
        for (auto &config_pair : f_candidates)
        {
            if (std::filesystem::exists(config_pair.first))
            {
                found_config = config_pair.first;
                f_dir = candidate_dirs_data[config_pair.second];
                goto _continue;
            }
        }
        if (found_config.empty())
            goto _throw;
    _continue:
        std::string config_content;
        {
            std::ifstream i(found_config, std::ios::in | std::ios::binary);
            i.seekg(0, std::ios::end);
            auto len = i.tellg();
            i.seekg(0, std::ios::beg);
            config_content.resize(len);
            i.read(config_content.data(), len);
        }
        {
            //
            context.vars["CMAKE_FIND_PACKAGE_NAME"] = pkg;
            context.vars[pkg + "_FIND_REQUIRED"] = required ? "TRUE" : "FALSE";
            context.vars[pkg + "_FIND_QUIETLY"] = quiet ? "TRUE" : "FALSE";
            context.vars[pkg + "_FIND_REGISTRY_VIEW"] = context.vars["___FIND_PACKAGE_REGISTRY_VIEW"].empty() ? "FALSE" : "TRUE";
            auto &version = context.vars["___FIND_PACKAGE_VERSION"];
            context.vars[pkg + "_FIND_VERSION"] = version;
            auto components = split_string(version, ".");
            static auto component_index_to_string = [](char component_index)
            {
                switch (component_index)
                {
                case 0:
                    return "MAJOR";
                case 1:
                    return "MINOR";
                case 2:
                    return "PATCH";
                case 3:
                    return "TWEAK";
                default:
                    throw std::runtime_error("[cmake] -- component_index is greater than 4");
                }
            };
            char component_index = 0;
            for (auto &component : components)
            {
                context.vars[pkg + "_FIND_VERSION_" + component_index_to_string(component_index++)] = component;
            }
            context.vars[pkg + "_FIND_VERSION_COUNT"] = std::to_string(component_index);
            context.vars[pkg + "_FIND_VERSION_EXACT"] = exact ? "TRUE" : "FALSE";
            context.vars[pkg + "_COMPONENTS"] = context.vars["___FIND_PACKAGE_COMPONENTS"];
            //
            // setup interface variables
        }
        dz::cmake::CommandParser::parseContentWithProject(*this, config_content);
    };
    auto [context_function, context_clear] = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_package_impl);
    context_function(cmd_arguments_size, cmd);
    context_clear();
    return;
}

bool dz::cmake::ConditionNode::BoolVar(const std::string &var) const
{
    return var != "false" && var != "FALSE" && var != "off" && var != "OFF";
}

#define condition_case(CN_OP, OP) \
    case CN_OP:                   \
        term = (lhs OP rhs);      \
        break
#define condition_case_numeric(CN_OP, OP)            \
    case CN_OP:                                      \
        term = (std::stoll(lhs) OP std::stoll(rhs)); \
        break

bool dz::cmake::ConditionNode::Evaluate(Project &project) const
{
    auto &vars = project.context_sh_ptr->vars;
    auto &context = *project.context_sh_ptr;
    auto children_size = children.size();
    switch (op)
    {
    case ConditionOp::Group:
    {
        size_t i = 0;
        bool have = false;
        bool result = false;
        ConditionOp logic = ConditionOp::Or;
        while (i < children_size)
        {
            bool invert = false;
            while (i < children_size && isNot(*children[i]))
            {
                invert = !invert;
                ++i;
            }
            if (i >= children_size)
                break;
            bool term = false;
            auto cur_child_op = children[i]->op;
            if (
                cur_child_op == ConditionOp::Defined ||
                cur_child_op == ConditionOp::Exists)
            {
                i += 1;
                if (i >= children_size)
                    break;
                auto &r_child = *children[i];
                auto &r_child_val = r_child.value;
                switch (cur_child_op)
                {
                case ConditionOp::Defined:
                {
                    auto var_it = vars.find(r_child_val);
                    term = (var_it != vars.end());
                    break;
                }
                case ConditionOp::Exists:
                {
                    term = std::filesystem::exists(r_child_val);
                    break;
                }
                }
            }
            else if (i + 1 < children_size)
            {
                auto mid_op = children[i + 1]->op;
                switch (mid_op)
                {
                case ConditionOp::Strequal:
                case ConditionOp::Equal:
                case ConditionOp::Less:
                case ConditionOp::Greater:
                case ConditionOp::LessEqual:
                case ConditionOp::GreaterEqual:
                {
                    auto &l_child = *children[i];
                    auto lhs = identify_child(context, l_child);
                    i += 2;
                    if (i >= children_size)
                        break;
                    auto &r_child = *children[i];
                    auto rhs = identify_child(context, r_child);
                    switch (mid_op)
                    {
                        condition_case(ConditionOp::Strequal, ==);
                        condition_case(ConditionOp::Equal, ==);
                        condition_case_numeric(ConditionOp::Less, <);
                        condition_case_numeric(ConditionOp::Greater, >);
                        condition_case_numeric(ConditionOp::LessEqual, <=);
                        condition_case_numeric(ConditionOp::GreaterEqual, >=);
                    }
                    break;
                }
                case ConditionOp::InList:
                {
                    auto &l_child = *children[i];
                    auto lhs = identify_child(context, l_child);
                    i += 2;
                    if (i >= children_size)
                        break;
                    auto &r_child = *children[i];
                    auto var = identify_child(context, r_child);
                    auto var_split = split_string(var, ";");
                    auto f_it = std::find(var_split.begin(), var_split.end(), lhs);
                    term = (f_it != var_split.end());
                    break;
                }
                default:
                {
                    goto _bool;
                }
                }
            }
            else
            {
            _bool:
                term = toBool(project, context, *children[i]);
            }
            term = invert ? (!term) : term;
        _post_term:
            if (!have)
            {
                result = term;
                have = true;
            }
            else
            {
                if (logic == ConditionOp::And)
                {
                    result = result && term;
                }
                else
                {
                    result = result || term;
                }
            }
            ++i;
            if (i < children_size && (children[i]->op == ConditionOp::And || children[i]->op == ConditionOp::Or))
            {
                logic = children[i]->op;
                ++i;
            }
        }
        return have ? result : false;
    }
    case ConditionOp::Identifier:
    {
        auto it = vars.find(value);
        if (it == vars.end())
            return false;
        return truthy(it->second);
    }
    case ConditionOp::Literal:
    {
        return truthy(value);
    }
    case ConditionOp::Not:
    {
        if (children.empty())
            return false;
        return !children[0]->Evaluate(project);
    }
    case ConditionOp::And:
    case ConditionOp::Or:
    {
        if (children.size() < 2)
            return false;
        bool a = children[0]->Evaluate(project);
        bool b = children[1]->Evaluate(project);
        if (op == ConditionOp::And)
        {
            return a && b;
        }
        else
        {
            return a || b;
        }
    }
    default:
    {
        return false;
    }
    }
}

std::shared_ptr<dz::cmake::Project> dz::cmake::CommandParser::parseFile(const std::string &path)
{
    std::ifstream in(path);
    if (!in.is_open())
        return {{}};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parseContent(content);
}

void dz::cmake::CommandParser::parseContentWithProject(Project &project, const std::string &content)
{
    auto &context = *project.context_sh_ptr;

    auto old_pos = context.pos;
    auto old_content_ptr = context.content_ptr;

    context.content_ptr = &content;
    context.pos = 0;

    execute_context_til_break(context, project);

    context.content_ptr = old_content_ptr;
    context.pos = old_pos;
}

void dz::cmake::CommandParser::execute_context_til_break(ParseContext &context, Project &project)
{
    while (context.pos < context.content_ptr->size() && (context.parse_to_pos == std::string::npos ||
                                                         context.pos < context.parse_to_pos))
    {
        Command cmd;
        if (!get_next_command(context, project, cmd))
            break;

        process_cmd(context, project, cmd);
    }
}

bool dz::cmake::CommandParser::get_next_command(ParseContext &context, Project &project, Command &out_cmd)
{
    skipWhitespaceAndComments(*context.content_ptr, context.pos);
    if (context.pos >= context.content_ptr->size())
        return false;
    size_t open = context.content_ptr->find('(', context.pos);
    if (open == std::string::npos)
        return false;
    auto start_cmd_pos = context.pos;
    std::string name = context.content_ptr->substr(context.pos, open - context.pos);
    trim(name);
    size_t close = findMatchingParen(*context.content_ptr, open);
    if (close == std::string::npos)
        return false;
    std::string args = context.content_ptr->substr(open + 1, close - open - 1);
    context.pos = close + 1;

    auto end_cmd_pos = context.pos;

    out_cmd = Command(context.content_ptr, start_cmd_pos, end_cmd_pos, name, args);

    return true;
}

void dz::cmake::CommandParser::process_cmd(ParseContext &context, Project &project, Command &cmd)
{
    static ValueVector endblock_names = {
        "endmacro",
        "endfunction",
        "endblock",
        "endforeach"};

    if (!context.evaluating_block_deque.empty() &&
        (context.just_triggered_break ||
         context.just_triggered_return ||
         context.just_triggered_continue))
    {
        Block::Type resuming_at;
        if (context.just_triggered_break || context.just_triggered_continue)
        {
            if (cmd.name == "endforeach" || cmd.name == "endif")
                goto _eval;
            else
                return;
        }
        else if (context.just_triggered_return)
        {
            if (cmd.name == "endfunction" || cmd.name == "endif")
                goto _eval;
            else
                return;
        }
        else if (context.just_triggered_continue)
        {
            if (cmd.name == "endforeach")
            {
                context.just_triggered_continue = false;
                goto _eval;
            }
            else if (cmd.name == "endif")
                goto _eval;
            else
                return;
        }
    }
    else if (!context.block_stack.empty())
    {
        auto type = context.block_stack.top()->type;
        if (context.recording_cmds_to_block_top)
        {
            if (type == Block::function && cmd.name == "endfunction")
            {
                context.function_depth--;
                if (context.function_depth == 0)
                    goto _eval;
            }
            else if (type == Block::macro && cmd.name == "endmacro")
            {
                context.macro_depth--;
                if (context.macro_depth == 0)
                    goto _eval;
            }
            else if (type == Block::block && cmd.name == "endblock")
            {
                context.block_depth--;
                if (context.block_depth == 0)
                    goto _eval;
            }
            if (type == Block::function && cmd.name == "function")
            {
                context.function_depth++;
            }
            else if (type == Block::macro && cmd.name == "macro")
            {
                context.macro_depth++;
            }
            else if (type == Block::block && cmd.name == "block")
            {
                context.block_depth++;
            }
            context.block_stack.top()->body.push_back(cmd);
            return;
        }
        goto _eval;
    }
    else
    {
    _eval:
        if (cmd.name == "function")
            context.function_depth++;
        else if (cmd.name == "macro")
            context.macro_depth++;
        else if (cmd.name == "block")
            context.block_depth++;
        varize(cmd, context);
        cmd.Evaluate(project);
    }
}

std::shared_ptr<dz::cmake::Project> dz::cmake::CommandParser::parseContent(const std::string &content)
{
    auto context_sh_ptr = std::make_shared<ParseContext>();
    return parseContent(content, context_sh_ptr);
}

std::shared_ptr<dz::cmake::Project> dz::cmake::CommandParser::parseContent(
    const std::string &content,
    const std::shared_ptr<dz::cmake::ParseContext> &context_sh_ptr)
{
    auto project_sh_ptr = std::make_shared<Project>(context_sh_ptr);
    context_sh_ptr->root_project = project_sh_ptr;
    parseContentWithProject(*project_sh_ptr, content);
    return project_sh_ptr;
}

void dz::cmake::CommandParser::skipWhitespaceAndComments(const std::string &s, size_t &pos)
{
    while (pos < s.size())
    {
        if (isspace(s[pos]))
        {
            ++pos;
            continue;
        }
        if (s[pos] == '#')
        {
            if (pos + 2 < s.size() && s[pos + 1] == '[')
            {
                pos = pos + 2;

                static std::regex end_start_comment_regex("=*\\[");

                auto startIt = s.begin() + pos;
                auto endIt = s.end();

                std::smatch match;

                if (std::regex_search(startIt, endIt, match, end_start_comment_regex))
                {
                    pos = match.position(0) + pos + match.str(0).size();

                    static std::regex end_comment_regex("\\]=*\\]");

                    auto startIt = s.begin() + pos;
                    auto endIt = s.end();

                    std::smatch match;

                    if (std::regex_search(startIt, endIt, match, end_comment_regex))
                    {
                        pos = match.position(0) + pos + match.str(0).size();
                    }
                    else
                    {
                        pos = s.size();
                    }
                    continue;
                }
            }
            while (pos < s.size() && s[pos] != '\n')
                ++pos;
            continue;
        }
        break;
    }
}

void dz::cmake::CommandParser::trim(std::string &s)
{
    while (!s.empty() && isspace(s.front()))
        s.erase(s.begin());
    while (!s.empty() && isspace(s.back()))
        s.pop_back();
}

void dz::cmake::CommandParser::tokenize(const std::string &s, dz::cmake::ValueVector &out)
{
    std::string current;
    int parenDepth = 0;
    bool inQuotes = false;
    bool wasEscape = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];

        if (c == '\\' && !wasEscape)
        {
            wasEscape = true;
        }
        else if (c == '"')
        {
            if (inQuotes && !wasEscape)
            {
                current.push_back(c);
                out.push_back(current);
                current.clear();
                inQuotes = false;
            }
            else
            {
                if (!wasEscape && !current.empty())
                {
                    out.push_back(current);
                    current.clear();
                }
                current.push_back(c);
                inQuotes = true;
                if (wasEscape)
                    wasEscape = false;
            }
        }
        else if (!inQuotes && isspace(c) && parenDepth == 0)
        {
            if (!current.empty())
            {
                out.push_back(current);
                current.clear();
            }
        }
        else
        {
            if (c == '(' && !inQuotes)
                parenDepth++;
            if (c == ')' && !inQuotes)
                parenDepth--;
            if (wasEscape)
            {
                switch (c)
                {
                case 'n':
                    c = '\n';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case '\\':
                    c = '\\';
                    break;
                }
                wasEscape = false;
            }
            current.push_back(c);
        }
    }
    if (!current.empty())
        out.push_back(current);
}

void dz::cmake::CommandParser::varize_str(std::string &str, ParseContext &parse_context, bool dequite)
{
    size_t off = 0;
    replaceAll(str, "\\n", "\n");
    replaceAll(str, "\\\"", "\"");
    auto &vars = parse_context.vars;
    while (true)
    {
        static std::string start_var = "${";
        static std::string end_var = "}";
        auto start_pos = str.find(start_var, off);
        if (start_pos == std::string::npos)
        {
            break;
        }
        auto var_start_pos = start_pos + start_var.size();
        auto end_pos = str.find(end_var);
        if (end_pos == std::string::npos)
        {
            throw std::runtime_error(R"([cmake] CMake Error:
[cmake]   Syntax error
[cmake] 
[cmake]   when parsing string
[cmake] 
[cmake]     )" + str + R"(
[cmake] 
[cmake]   There is an unterminated variable reference.)");
        }
        auto var_len = end_pos - var_start_pos;
        auto block_len = (end_pos - start_pos) + 1;
        auto block = str.substr(start_pos, block_len);
        auto var = str.substr(var_start_pos, var_len);
        auto var_it = vars.find(var);
        if (var_it == vars.end())
        {
            replaceAll(str, block, "");
        }
        else
        {
            auto var_val = dequote(var_it->second);
            replaceAll(str, block, var_val);
        }
    }
    envize_str(str, parse_context);
}

void dz::cmake::CommandParser::envize_str(std::string &str, ParseContext &parse_context)
{
    size_t off = 0;
    auto &env = parse_context.env;
    while (true)
    {
        static std::string start_var = "$ENV{";
        static std::string end_var = "}";
        auto start_pos = str.find(start_var, off);
        if (start_pos == std::string::npos)
        {
            break;
        }
        auto var_start_pos = start_pos + start_var.size();
        auto end_pos = str.find(end_var);
        if (end_pos == std::string::npos)
        {
            break;
        }
        auto var_len = end_pos - var_start_pos;
        auto block_len = (end_pos - start_pos) + 1;
        auto block = str.substr(start_pos, block_len);
        auto var = str.substr(var_start_pos, var_len);
        auto env_it = env.find(var);
        if (env_it == env.end())
        {
            replaceAll(str, block, "");
        }
        else
        {
            auto var_val = dequote(env_it->second);
            replaceAll(str, block, var_val);
        }
    }
}

void dz::cmake::CommandParser::varize(Command &cmd, ParseContext &parse_context)
{
    for (auto &arg : cmd.arguments)
    {
        varize_str(arg, parse_context);
    }
}

size_t dz::cmake::CommandParser::findMatchingParen(const std::string &s, size_t open)
{
    int depth = 0;
    for (size_t i = open; i < s.size(); ++i)
    {
        if (s[i] == '(')
            depth++;
        else if (s[i] == ')')
        {
            depth--;
            if (depth == 0)
                return i;
        }
    }
    return std::string::npos;
}