#include <dz/CMake.hpp>
#include <dz/Util.hpp>
#include <queue>
#include <unordered_set>
#include <cassert>

template<typename F>
dz::function<void(size_t, const dz::cmake::Command&)> abstractify_cmake_function(
    const std::shared_ptr<dz::cmake::ParseContext>& context_sh_ptr,
    const std::string& prefix,
    const dz::cmake::ValueVector& options,
    const dz::cmake::ValueVector& one_value_keywords,
    const dz::cmake::ValueVector& multi_value_keywords,
    F run_with_abstract_set,
    size_t parse_argv = 0,
    bool new_scope = true
)
{
    struct abstract_context
    {
        std::shared_ptr<dz::cmake::ParseContext> context_sh_ptr;
        std::string prefix;
        dz::cmake::ValueVector options;
        dz::cmake::ValueVector one_value_keywords;
        dz::cmake::ValueVector multi_value_keywords;
        F run_with_abstract_set;
        size_t parse_argv;
        bool new_scope;
    };
    auto abstract_context_ptr = new abstract_context{
        .context_sh_ptr = context_sh_ptr,
        .prefix = prefix,
        .options = options,
        .one_value_keywords = one_value_keywords,
        .multi_value_keywords = multi_value_keywords,
        .run_with_abstract_set = run_with_abstract_set,
        .parse_argv = parse_argv,
        .new_scope = new_scope,
    };
    return [abstract_context_ptr](auto cmd_arguments_size, const auto& cmd){
        auto& abs_con = *abstract_context_ptr;
        auto& context = *abs_con.context_sh_ptr;

        if ((cmd.name == "else" || cmd.name == "elseif") && (context.if_depth - 1) == context.valid_if_depth)
            static_assert(true);
        else if ((cmd.name != "endif") && context.if_depth != context.valid_if_depth)
            return;

        dz::cmake::VariableMap all_keys_and_vals;

        insert_to_map_from_vec_with_string_prefix_prepended(all_keys_and_vals, abs_con.multi_value_keywords, abs_con.prefix);
        insert_to_map_from_vec_with_string_prefix_prepended(all_keys_and_vals, abs_con.one_value_keywords, abs_con.prefix);
        insert_to_map_from_vec_with_string_prefix_prepended(all_keys_and_vals, abs_con.options, abs_con.prefix);

        std::string parsing_key;
        std::string* parsing_val = nullptr;
        dz::cmake::ValueVector new_arguments;

        dz::cmake::ValueVector options_set, one_value_keywords_set, multi_value_keywords_set;

        for (size_t i = 0; i < cmd_arguments_size; i++)
        {
            auto& current_arg = cmd.arguments[i];
            if (in_map(all_keys_and_vals, abs_con.prefix + "_" + current_arg)) {
                parsing_key = current_arg;
                parsing_val = &all_keys_and_vals[abs_con.prefix + "_" + parsing_key];
                if (in_vec(abs_con.options, parsing_key))
                {
                    (*parsing_val) = "TRUE";
                    options_set.push_back(parsing_key);
                    parsing_key.clear();
                }
                else if (in_vec(abs_con.one_value_keywords, parsing_key))
                {
                    one_value_keywords_set.push_back(parsing_key);
                }
                else // only possible other vec is multi
                {
                    multi_value_keywords_set.push_back(parsing_key);
                }
            }
            else if (parsing_key.empty()) {
                if ((abs_con.parse_argv != 0 && new_arguments.size() < abs_con.parse_argv) || !abs_con.parse_argv) {
                    new_arguments.push_back(current_arg);
                }
            }
            else if (in_vec(abs_con.one_value_keywords, parsing_key))
            {
                if (parsing_val->empty()) {
                    (*parsing_val) = current_arg;
                    parsing_key.clear();
                }
                else {
                    throw std::runtime_error("[cmake] '" + parsing_key + "' expects one value)");
                }
            }
            else if (in_vec(abs_con.multi_value_keywords, parsing_key))
            {
                (*parsing_val) += ((parsing_val->empty() ? "" : ";") + current_arg);
            }
        }
        {
            dz::cmake::Command new_cmd = cmd;
            new_cmd.arguments = new_arguments;
            auto old_marked_vars = context.marked_vars;
            auto old_context_vars = context.vars;
            auto old_if_depth = context.if_depth;
            auto old_valid_if_depth = context.valid_if_depth;
            if (abs_con.new_scope)
            {
                context.valid_if_depth = context.if_depth = 0;
            }
            insert_to_map_from_map(context.vars, all_keys_and_vals);
            if constexpr (requires { abs_con.run_with_abstract_set(new_cmd.arguments.size(), new_cmd, options_set, one_value_keywords_set, multi_value_keywords_set); } )
            {
                abs_con.run_with_abstract_set(new_cmd.arguments.size(), new_cmd, options_set, one_value_keywords_set, multi_value_keywords_set);
            }
            else if constexpr (requires { abs_con.run_with_abstract_set(new_cmd.arguments.size(), new_cmd); } )
            {
                abs_con.run_with_abstract_set(new_cmd.arguments.size(), new_cmd);
            }
            if (abs_con.new_scope) {
                auto new_context_vars = context.vars;
                context.vars = old_context_vars;
                context.restore_marked_vars(new_context_vars);
                context.marked_vars = old_marked_vars;
                context.valid_if_depth = old_valid_if_depth;
                context.if_depth = old_if_depth;
            }
            else {
                remove_to_map_from_map(context.vars, all_keys_and_vals);
            }
        }
        delete abstract_context_ptr;
    };
}

void dz::cmake::Command::Evaluate(Project &project)
{
    auto it_macro = project.macros.find(name);
    if (it_macro != project.macros.end())
    {
        const Macro &m = it_macro->second;
        m.execute(project, *this);
        return;
    }

    auto cmd_arguments_size = arguments.size();

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

void dz::cmake::ConditionNode::ParseConditions(Project& project, size_t cmd_arguments_size, const Command& cmd)
{
    static std::unordered_map<std::string, ConditionOp> condition_op_map = {
        {"AND", ConditionOp::And},
        {"NOT", ConditionOp::Not},
        {"OR", ConditionOp::Or},
        {"STREQUAL", ConditionOp::Strequal},
        {"EQUAL", ConditionOp::Equal},
        {"IN_LIST", ConditionOp::InList},
        {"DEFINED", ConditionOp::Defined},
    };

    auto& context = *project.context_sh_ptr;

    op = ConditionOp::Group;
    value = join_string_vec(cmd.arguments, " ");

    children.reserve(cmd.arguments.size());
    for (auto &arg : cmd.arguments)
    {
        auto cond_node = std::make_shared<ConditionNode>();
        children.push_back(cond_node);

        auto& cond = *cond_node;
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

void dz::cmake::Macro::execute(Project &project, const Command &cmd) const
{
    auto &context = *project.context_sh_ptr;
    auto vars_copy = context.vars;
    auto local_vars = vars_copy;
    auto env_copy = context.env;
    auto local_env = env_copy;
    for (size_t i = 0; i < params.size() && i < cmd.arguments.size(); ++i)
        local_vars[params[i]] = cmd.arguments[i];
    context.vars = local_vars;
    context.env = local_env;
    for (auto &macroCmd : body)
    {
        Command expanded = macroCmd;
        for (auto &arg : expanded.arguments)
        {
            CommandParser::varize_str(arg, context);
        }
        expanded.Evaluate(project);
    }
    context.vars = vars_copy;
    context.env = env_copy;
}

void dz::cmake::ParseContext::restore_marked_vars(const VariableMap& old_vars)
{
    for (auto& [marked_var, is_marked] : marked_vars)
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

void dz::cmake::ParseContext::mark_var(const std::string& var, bool mark_bool)
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

void dz::cmake::Project::___macro(size_t cmd_arguments_size, const Command& cmd)
{

}

void dz::cmake::Project::___endmacro(size_t cmd_arguments_size, const Command& cmd)
{

}

void dz::cmake::Project::___if(size_t cmd_arguments_size, const Command& cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___IF";
    static ValueVector options = {
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto ___if_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        auto is_elseif = (cmd.name == "elseif");
        if (is_elseif && context.if_depth == context.valid_if_depth)
        {
            context.valid_if_depth++;
            return;
        }
        else if (is_elseif) {
            context.valid_if_depth--;
        }
        auto is_if = (cmd.name == "if");
        if (is_if)
            context.if_depth++;
        auto condition_sh_ptr = std::make_shared<ConditionNode>();
        auto& condition = *condition_sh_ptr;
        condition.ParseConditions(*this, cmd_arguments_size, cmd);
        if (condition.Evaluate(*this) || is_elseif)
            context.valid_if_depth++;
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___if_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::___else(size_t cmd_arguments_size, const Command& cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___ELSE";
    static ValueVector options = {
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto ___else_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        context.valid_if_depth++;
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___else_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::___endif(size_t cmd_arguments_size, const Command& cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___ENDIF";
    static ValueVector options = {
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto ___endif_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        context.if_depth--;
        while (context.valid_if_depth > context.if_depth)
            context.valid_if_depth--;
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, ___endif_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::add_library(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___ADD_LIBRARY";
    static ValueVector options = {
        "SHARED",
        "STATIC",
        "MODULE",
        "INTERFACE",
        "IMPORTED"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto add_library_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        if (!cmd_arguments_size)
            return;
        auto& target_name = cmd.arguments[0];
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, add_library_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::add_executable(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___ADD_EXECUTABLE";
    static ValueVector options = {
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto add_executable_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        if (!cmd_arguments_size)
            return;
        auto& target_name = cmd.arguments[0];
        auto target = std::make_shared<Target>(Target::Type::Executable, target_name);
        for (size_t i = 1; i < cmd_arguments_size; ++i)
        {
            auto &arg = cmd.arguments[i];
            target->addArgument(arg);
        }
        targets[target_name] = target;
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, add_executable_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::target_include_directories(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___TARGET_INCLUDE_DIRECTORIES";
    static ValueVector options = {
        "PRIVATE",
        "PUBLIC"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto target_include_directories_impl = [&](auto cmd_arguments_size, const Command& cmd)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, target_include_directories_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::target_link_libraries(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___TARGET_LINK_LIBRARIES";
    static ValueVector options = {
        "PRIVATE",
        "PUBLIC"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto target_link_libraries_impl = [&](auto cmd_arguments_size, const Command& cmd)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, target_link_libraries_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::message(size_t cmd_arguments_size, const Command &cmd)
{
    static std::unordered_map<std::string, CMakeMessageType> msg_type_map = {
        { "", CMakeMessageType::UNSET },
        { "STATUS", CMakeMessageType::STATUS },
        { "WARNING", CMakeMessageType::WARNING },
        { "FATAL_ERROR", CMakeMessageType::FATAL_ERROR },
        { "AUTHOR_WARNING", CMakeMessageType::AUTHOR_WARNING },
    };
    auto& context = *context_sh_ptr;
    static auto prefix = "___MESSAGE";
    static ValueVector options = {
        "STATUS",
        "WARNING",
        "FATAL_ERROR",
        "AUTHOR_WARNING",
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto message_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
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
            replace(arg, "\n", "\n[cmake]");
            concat += arg;
            if (i < cmd_arguments_size - 1)
            {
                concat += "\n";
            }
            args_concat += concat;
        }
        // CommandParser::varize_str(args_concat, context, true);
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, message_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::get_filename_component(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___GET_FILE_NAME_COMPONENT";
    static ValueVector options = {
        "ABSOLUTE",
        "REALPATH",
        "PATH",
        "NAME"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto get_filename_component_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        static std::unordered_map<std::string, dz::function<void(size_t, const Command &, ParseContext&, const std::string&, const std::filesystem::path&)>> gfc_actions = {
            { "", [](auto cmd_arguments_size, auto& cmd, auto& context, const auto& varName, const auto& p) {
                context.vars[varName] = p.string();
                return;
            } },
            { "ABSOLUTE", [](auto cmd_arguments_size, auto& cmd, auto& context, const auto& varName, const auto& p) {
                context.vars[varName] = std::filesystem::absolute(p).lexically_normal().string();
                return;
            } },
            { "REALPATH", [](auto cmd_arguments_size, auto& cmd, auto& context, const auto& varName, const auto& p) {
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
            } },
            { "PATH", [](auto cmd_arguments_size, auto& cmd, auto& context, const auto& varName, const auto& p) {
                context.vars[varName] = p.parent_path().string();
                return;
            } },
            { "NAME", [](auto cmd_arguments_size, auto& cmd, auto& context, const auto& varName, const auto& p) {
                context.vars[varName] = p.filename().string();
                return;
            } },
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, get_filename_component_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::project(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___PROJECT";
    static ValueVector options = {
    };
    static ValueVector one_value_keywords = {
        "VERSION",
        "COMPAT_VERSION",
        "DESCRIPTION",
        "HOMEPAGE_URL"
    };
    static ValueVector multi_value_keywords = {
        "LANGUAGES"
    };
    auto project_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        if (cmd_arguments_size < 1)
            return;
        name = cmd.arguments[0];

        if (in_vec(one_value_keywords_set, "VERSION"))
            version = context.vars["___PROJECT_VERSION"];
        if (in_vec(one_value_keywords_set, "COMPAT_VERSION"))
            compat_version = context.vars["___PROJECT_COMPAT_VERSION"];
        if (in_vec(one_value_keywords_set, "DESCRIPTION"))
            description = context.vars["___PROJECT_DESCRIPTION"];
        if (in_vec(one_value_keywords_set, "HOMEPAGE_URL"))
            homepage_url = context.vars["___PROJECT_HOMEPAGE_URL"];
        if (in_vec(multi_value_keywords_set, "LANGUAGES"))
            languages = split_string(context.vars["___PROJECT_LANGUAGES"], ";");
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, project_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::list(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___LIST";
    static ValueVector options = {
        "APPEND",
        "REMOVE_ITEM"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto list_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        static DSL_Map_With_Context list_actions = {
            { "APPEND", [](auto cmd_arguments_size, auto& cmd, auto& context) {
                if (cmd_arguments_size < 1)
                    return;
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
            } },
            { "REMOVE_ITEM", [](auto cmd_arguments_size, auto& cmd, auto& context) {
                if (cmd_arguments_size < 1)
                    return;
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
            } }
        };

        if (!options_set.empty())
        {
            auto& option = options_set.front();
            list_actions[option](cmd_arguments_size, cmd, context);
        }
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, list_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::cmake_policy(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___CMAKE_POLICY";
    static ValueVector options = {
        "PUSH",
        "POP",
        "SET",
        "GET"
    };
    static ValueVector one_value_keywords = {
        "VERSION"
    };
    static ValueVector multi_value_keywords = {
    };
    auto policy_impl = [&](auto cmd_arguments_size, const Command& cmd, const ValueVector& options_set, const ValueVector& one_value_keywords_set, const ValueVector& multi_value_keywords_set)
    {
        static DSL_Map_With_Context policy_actions = {
            { "PUSH", [](auto cmd_arguments_size, auto& cmd, auto& context) {
                context.policy_stack.emplace_front(); // push empty scope
                context.policy_push_just_called = true;
                return;
            } },
            { "POP", [](auto cmd_arguments_size, auto& cmd, auto& context) {
                if (context.policy_stack.empty())
                    return;

                auto policy_sh_ptr = context.policy_stack.front();
                context.policy_stack.pop_front(); // discard scope
                auto &policy = *policy_sh_ptr;
                context.policy_set_map.erase(policy.policy_name);
                return;
            } },
            { "SET", [](auto cmd_arguments_size, auto& cmd, auto& context) {
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
            } },
            { "GET", [](auto cmd_arguments_size, auto& cmd, auto& context) {
                // TODO:
            } },
            { "VERSION", [](auto cmd_arguments_size, auto& cmd, auto& context) {
                // TODO:
            } }
        };

        if (!options_set.empty())
        {
            auto& option = options_set.front();
            policy_actions[option](cmd_arguments_size, cmd, context);
        }
        else if (!one_value_keywords.empty())
        {
            auto& one_value_keyword = one_value_keywords_set.front();
            policy_actions[one_value_keyword](cmd_arguments_size, cmd, context);
        }
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, policy_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::set(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___SET";
    static ValueVector options = {
        "PARENT_SCOPE"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto set_impl = [&](auto cmd_arguments_size, const Command& cmd)
    {
        if (cmd_arguments_size < 2)
            return;
        auto parent_scope = context.vars["___SET_PARENT_SCOPE"] == "TRUE";
        auto &var_name = cmd.arguments[0];
        if (var_name.empty())
            return;
        auto &vars = context_sh_ptr->vars;
        auto &var = vars[var_name];
        for (size_t i = 1; i < cmd_arguments_size; i++)
        {
            if (!var.empty())
                var += ";";
            auto val = cmd.arguments[i];
            val = dequote(val);
            var += dequote(val);
        }
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, set_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::unset(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___UNSET";
    static ValueVector options = {
        "CACHE",
        "PARENT_SCOPE"
    };
    static ValueVector one_value_keywords = {
    };
    static ValueVector multi_value_keywords = {
    };
    auto unset_impl = [&](auto cmd_arguments_size, const Command& cmd)
    {
        if (cmd_arguments_size < 2)
            return;
        auto parent_scope = context.vars["___UNSET_PARENT_SCOPE"] == "TRUE";
        auto cache = context.vars["___UNSET_CACHE"] == "TRUE";
        auto &var_name = cmd.arguments[0];
        if (var_name.empty())
            return;
        auto& vars = context.vars;
        auto var_it = vars.find(var_name);
        if (var_it == vars.end())
            return;
        vars.erase(var_it);
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, unset_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::find_path(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___FIND_PATH";
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
        "CMAKE_FIND_ROOT_PATH_BOTH"
    };
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VALIDATOR",
        "DOC"
    };
    static ValueVector multi_value_keywords = {
        "NAMES",
        "HINTS",
        "PATHS"
    };
    auto find_path_impl = [&](auto cmd_arguments_size, const Command& cmd)
    {
        if (cmd_arguments_size < 1)
            return;
        auto& var_name = cmd.arguments[0];
        if (cmd_arguments_size > 2)
            throw std::runtime_error(R"([cmake] -- find_path arguments count should never be > 2)");
        auto names = split_string(context.vars["___FIND_PATH_NAMES"], ";");
        if (cmd_arguments_size == 2) {
            auto& one_name = cmd.arguments[1];
            names.push_back(one_name);
        }
        auto hints = split_string(context.vars["___FIND_PATH_HINTS"], ";");
        for (auto& hint_dir : hints)
        {
            auto hint_path = std::filesystem::path(hint_dir);
            for (auto& name_path : names)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_path_impl);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::find_library(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___FIND_LIBRARY";
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
        "CMAKE_FIND_ROOT_PATH_BOTH"
    };
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VALIDATOR",
        "DOC"
    };
    static ValueVector multi_value_keywords = {
        "PATH_SUFFIXES",
        "NAMES",
        "HINTS",
        "PATHS"
    };
    auto find_library_impl = [&](auto cmd_arguments_size, const Command& cmd)
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
        auto& var_name = cmd.arguments[0];
        if (cmd_arguments_size > 2)
            throw std::runtime_error(R"([cmake] -- find_library arguments count should never be > 2)");
        auto all_suffixes = split_string(context.vars["___FIND_LIBRARY_PATH_SUFFIXES"], ";");
        all_suffixes.insert(all_suffixes.end(), default_suffixes.begin(), default_suffixes.end());
        auto names = split_string(context.vars["___FIND_LIBRARY_NAMES"], ";");
        if (cmd_arguments_size == 2) {
            auto& one_name = cmd.arguments[1];
            names.push_back(one_name);
        }
        auto hints = split_string(context.vars["___FIND_LIBRARY_HINTS"], ";");
        auto paths = split_string(context.vars["___FIND_LIBRARY_PATHS"], ";");
        hints.insert(hints.end(), paths.begin(), paths.end());
        for (auto& hint_dir : hints)
        {
            auto hint_path = std::filesystem::path(hint_dir);
            for (auto& name_string : names)
            {
                for (auto& suffix : all_suffixes)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_library_impl);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::find_program(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___FIND_PROGRAM";
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
        "CMAKE_FIND_ROOT_PATH_BOTH"
    };
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VALIDATOR",
        "DOC"
    };
    static ValueVector multi_value_keywords = {
        "PATH_SUFFIXES",
        "NAMES",
        "HINTS",
        "PATHS"
    };
    auto find_program_impl = [&](auto cmd_arguments_size, const Command& cmd)
    {
        static ValueVector default_suffixes = {
#if defined(_WIN32)
            ".exe",
#endif
            ""
        };
        if (cmd_arguments_size < 1)
            return;
        auto& var_name = cmd.arguments[0];
        if (cmd_arguments_size > 2)
            throw std::runtime_error(R"([cmake] -- find_library arguments count should never be > 2)");
        auto all_suffixes = split_string(context.vars["___FIND_PROGRAM_PATH_SUFFIXES"], ";");
        all_suffixes.insert(all_suffixes.end(), default_suffixes.begin(), default_suffixes.end());
        auto names = split_string(context.vars["___FIND_PROGRAM_NAMES"], ";");
        if (cmd_arguments_size == 2) {
            auto& one_name = cmd.arguments[1];
            names.push_back(one_name);
        }
        auto hints = split_string(context.vars["___FIND_PROGRAM_HINTS"], ";");
        auto paths = split_string(context.vars["___FIND_PROGRAM_PATHS"], ";");
        hints.insert(hints.end(), paths.begin(), paths.end());
        for (auto& hint_dir : hints)
        {
            auto hint_path = std::filesystem::path(hint_dir);
            for (auto& name_string : names)
            {
                for (auto& suffix : all_suffixes)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_program_impl);
    context_function(cmd_arguments_size, cmd);
    return;
}

void dz::cmake::Project::mark_as_advanced(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___MARK_AS_ADVANCED";
    static ValueVector options = {
        "CLEAR",
        "FORCE",
    };
    static ValueVector one_value_keywords = {};
    static ValueVector multi_value_keywords = {};
    auto mark_as_advanced_impl = [&](auto cmd_arguments_size, const Command& cmd)
    {
        auto& clear = context.vars["___MARK_AS_ADVANCED_CLEAR"];
        auto mark_bool = clear != "TRUE";
        auto& force = context.vars["___MARK_AS_ADVANCED_FORCE"];
        auto force_bool = force == "TRUE";
        for (size_t i = 0; i < cmd_arguments_size; i++)
        {
            auto& var_name = cmd.arguments[i];
            context.mark_var(var_name, mark_bool || force_bool);
        }
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, mark_as_advanced_impl, 0, false);
    context_function(cmd_arguments_size, cmd);
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

dz::cmake::Project::DSL_Map dz::cmake::Project::generate_dsl_map()
{
    DSL_Map map = {
        {"macro",
         { this, &Project::___macro } },
        {"endmacro",
         { this, &Project::___endmacro } },
        {"if",
         { this, &Project::___if } },
        {"elseif",
         { this, &Project::___if } },
        {"else",
         { this, &Project::___else } },
        {"endif",
         { this, &Project::___endif } },
        {"add_library",
         { this, &Project::add_library } },
        {"add_executable",
         { this, &Project::add_executable } },
        {"target_include_directories",
         { this, &Project::target_include_directories } },
        {"target_link_libraries",
         { this, &Project::target_link_libraries } },
        {"project",
         { this, &Project::project } },
        {"find_package",
         { this, &Project::find_package } },
        {"message",
         { this, &Project::message } },
        {"get_filename_component",
         { this, &Project::get_filename_component } },
        {"list",
         { this, &Project::list } },
        {"cmake_policy",
         { this, &Project::cmake_policy } },
        {"set",
         { this, &Project::set } },
        {"unset",
         { this, &Project::unset } },
        {"find_path",
         { this, &Project::find_path } },
        {"find_library",
         { this, &Project::find_library } },
        {"find_program",
         { this, &Project::find_program } },
        {"mark_as_advanced",
         { this, &Project::mark_as_advanced } },
    };
    return map;
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

void dz::cmake::Project::find_package(size_t cmd_arguments_size, const Command &cmd)
{
    auto& context = *context_sh_ptr;
    static auto prefix = "___FIND_PACKAGE";
    static ValueVector options = {
        "REQUIRED",
        "QUIET",
        "EXACT"
    };
    static ValueVector one_value_keywords = {
        "REGISTRY_VIEW",
        "VERSION"
    };
    static ValueVector multi_value_keywords = {
        "COMPONENTS"
    };
    auto find_package_impl = [&](auto cmd_arguments_size, const Command& cmd, const auto& options_set, const auto& one_value_keywords_set, const auto& multi_value_keywords_set)
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
            auto& version = context.vars["___FIND_PACKAGE_VERSION"];
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
            for (auto& component : components)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_package_impl);
    context_function(cmd_arguments_size, cmd);
    return;
}

bool dz::cmake::ConditionNode::BoolVar(const std::string &var) const
{
    return var != "false" && var != "FALSE" && var != "off" && var != "OFF";
}

bool dz::cmake::ConditionNode::Evaluate(Project &project) const
{
    auto &vars = project.context_sh_ptr->vars;
    switch (op)
    {
    case ConditionOp::Group:
    {
        auto upper = [&](std::string s)
        {
            for (char &c : s)
            {
                c = (char)std::toupper((unsigned char)c);
            }
            return s;
        };
        auto truthy = [&](const std::string &s)
        {
            std::string u = upper(s);
            if (s.empty() || u == "0" || u == "FALSE" || u == "OFF" || u == "NO" || u == "N" || u == "IGNORE" || u == "NOTFOUND" || (u.size() > 9 && u.rfind("-NOTFOUND") == u.size() - 9))
                return false;
            return true;
        };
        dz::function<std::string(const ConditionNode &)> toString = [&](const ConditionNode &n)
        {
            switch (n.op)
            {
            case ConditionOp::Identifier:
            {
                auto it = vars.find(n.value);
                return it != vars.end() ? it->second : n.value;
            }
            case ConditionOp::Literal:
            {
                return n.value;
            }
            case ConditionOp::Group:
            {
                return n.Evaluate(project) ? std::string("TRUE") : std::string("FALSE");
            }
            default:
            {
                return n.value;
            }
            }
        };
        dz::function<bool(const ConditionNode &)> toBool = [&](const ConditionNode &n)
        {
            switch (n.op)
            {
            case ConditionOp::Identifier:
            {
                auto it = vars.find(n.value);
                return it != vars.end() ? truthy(it->second) : false;
            }
            case ConditionOp::Literal:
            {
                return truthy(n.value);
            }
            case ConditionOp::Group:
            {
                return n.Evaluate(project);
            }
            default:
            {
                return truthy(n.value);
            }
            }
        };
        auto isNot = [&](const ConditionNode &n)
        {
            if (n.op == ConditionOp::Not)
                return true;
            if (n.op == ConditionOp::Identifier || n.op == ConditionOp::Literal)
            {
                std::string u = upper(n.value);
                if (u == "!" || u == "NOT")
                    return true;
            }
            return false;
        };
        auto truthize = [&]()
        {
            for (auto &child_sh_ptr : children)
            {
                auto &child = *child_sh_ptr;
                if (child.op == ConditionOp::IdentifierOrLiteral)
                {
                    assert(false);
                    auto f_it = vars.find(child.value);
                    if (is_var(child.value) || (f_it == vars.end() && is_literal(child.value)))
                    {
                        cmake::CommandParser::varize_str(child.value, *project.context_sh_ptr);
                        // child.value = dequote(child.value);
                        child.op = ConditionOp::Literal;
                    }
                    else
                    {
                        child.op = ConditionOp::Identifier;
                    }
                }
            }
        };
        truthize();
        auto identifyVar = [&](auto& child) -> std::string
        {
            if (child.op == ConditionOp::Identifier) {
                auto var_it = vars.find(child.value);
                if (var_it == vars.end())
                    return child.value;
                return var_it->second;
            }
            return dequote(child.value);
        };
        size_t i = 0;
        bool have = false;
        bool result = false;
        ConditionOp logic = ConditionOp::Or;
        while (i < children.size())
        {
            bool invert = false;
            while (i < children.size() && isNot(*children[i]))
            {
                invert = !invert;
                ++i;
            }
            if (i >= children.size())
                break;
            bool term = false;
            if (i + 1 < children.size() && (children[i + 1]->op == ConditionOp::Strequal || children[i + 1]->op == ConditionOp::Equal))
            {
                auto& l_child = *children[i];
                auto lhs = identifyVar(l_child);
                i += 2;
                if (i >= children.size())
                    break;
                auto& r_child = *children[i];
                auto rhs = identifyVar(r_child);
                ++i;
                term = (lhs == rhs);
                if (invert)
                    term = !term;
            }
            else if (i + 1 < children.size() && children[i + 1]->op == ConditionOp::InList)
            {
                auto& l_child = *children[i];
                auto lhs = identifyVar(l_child);
                i += 2;
                if (i >= children.size())
                    break;
                auto& r_child = *children[i];
                auto var = identifyVar(r_child);
                ++i;
                auto var_split = split_string(var, ";");
                auto f_it = std::find(var_split.begin(), var_split.end(), lhs);
                term = (f_it != var_split.end());
                if (invert)
                    term = !term;
            }
            else if (children[i]->op == ConditionOp::Defined)
            {
                i += 1;
                if (i >= children.size())
                    break;
                auto& r_child = *children[i];
                auto& var_name = r_child.value;
                ++i;
                auto var_it = vars.find(var_name);
                term = (var_it != vars.end());
                if (invert)
                    term = !term;
            }
            else
            {
                bool v = toBool(*children[i]);
                ++i;
                term = invert ? (!v) : v;
            }
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
            if (i < children.size() && (children[i]->op == ConditionOp::And || children[i]->op == ConditionOp::Or))
            {
                logic = children[i]->op;
                ++i;
            }
        }
        return have ? result : false;
    }
    case ConditionOp::Identifier:
    {
        auto upper = [&](std::string s)
        {for(char&c:s){c=(char)std::toupper((unsigned char)c);}return s; };
        auto truthy = [&](const std::string &s)
        {std::string u=upper(s);if(s.empty()||u=="0"||u=="FALSE"||u=="OFF"||u=="NO"||u=="N"||u=="IGNORE"||u=="NOTFOUND"||(u.size()>9&&u.rfind("-NOTFOUND")==u.size()-9))return false;return true; };
        auto it = vars.find(value);
        if (it == vars.end())
            return false;
        return truthy(it->second);
    }
    case ConditionOp::Literal:
    {
        auto upper = [&](std::string s)
        {for(char&c:s){c=(char)std::toupper((unsigned char)c);}return s; };
        auto truthy = [&](const std::string &s)
        {std::string u=upper(s);if(s.empty()||u=="0"||u=="FALSE"||u=="OFF"||u=="NO"||u=="N"||u=="IGNORE"||u=="NOTFOUND"||(u.size()>9&&u.rfind("-NOTFOUND")==u.size()-9))return false;return true; };
        return truthy(value);
    }
    case ConditionOp::Not:
    {
        if (children.empty())
            return false;
        return !children[0]->Evaluate(project);
    }
    case ConditionOp::Defined:
    {
        if (children.empty())
            return false;

        return children[0]->Evaluate(project);
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
    case ConditionOp::Strequal:
    {
        if (children.size() != 2)
            return false;
        auto get = [&](const ConditionNode &n)
        {if(n.op==ConditionOp::Identifier){auto it=vars.find(n.value);return it!=vars.end()?it->second:std::string();}else if(n.op==ConditionOp::Literal){return n.value;}else{return std::string(n.Evaluate(project)?"TRUE":"FALSE");} };
        return get(*children[0]) == get(*children[1]);
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
    size_t pos = 0;

    auto &context = *project.context_sh_ptr;

    while (pos < content.size())
    {
        skipWhitespaceAndComments(content, pos);
        if (pos >= content.size())
            break;
        size_t open = content.find('(', pos);
        if (open == std::string::npos)
            break;
        std::string name = content.substr(pos, open - pos);
        trim(name);
        size_t close = findMatchingParen(content, open);
        if (close == std::string::npos)
            break;
        std::string args = content.substr(open + 1, close - open - 1);
        pos = close + 1;

        // if (name == "macro")
        // {
        //     dz::cmake::ValueVector tokens;
        //     tokenize(args, tokens);
        //     if (tokens.empty())
        //         continue;
        //     macroName = tokens[0];
        //     macroParams.assign(tokens.begin() + 1, tokens.end());
        //     macroBody.clear();
        //     insideMacro = true;
        // }
        // else if (name == "endmacro")
        // {
        //     if (insideMacro)
        //     {
        //         Macro m;
        //         m.params = macroParams;
        //         m.body = macroBody;
        //         project.macros[macroName] = std::move(m);
        //         insideMacro = false;
        //         macroName.clear();
        //         macroParams.clear();
        //         macroBody.clear();
        //     }
        // }
        auto cmd_sh_ptr = std::make_shared<Command>();
        auto &cmd = *cmd_sh_ptr;
        cmd.name = name;
        tokenize(args, cmd.arguments);
        if (context.insideMacro)
        {
            context.macroBody.push_back(cmd);
        }
        else
        {
            varize(cmd, context);
            cmd.Evaluate(project);
        }
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
                size_t eq_start = pos + 2;
                size_t eq_count = 0;
                while (eq_start < s.size() && s[eq_start] == '=')
                {
                    ++eq_start;
                    ++eq_count;
                }
                if (eq_start < s.size() && s[eq_start] == '[')
                {
                    pos = eq_start + 1;
                    std::string end_seq = "]" + std::string(eq_count, '=') + "]";
                    while (pos < s.size())
                    {
                        if (s[pos] == '#' && pos + end_seq.size() <= s.size() && s.compare(pos, end_seq.size(), end_seq) == 0)
                        {
                            pos += end_seq.size();
                            break;
                        }
                        else
                        {
                            ++pos;
                        }
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

        if (c == '\\')
        {
            if (!wasEscape)
                wasEscape = true;
            else
                wasEscape = false;
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
            current.push_back(c);
        }
    }
    if (!current.empty())
        out.push_back(current);
}

void dz::cmake::CommandParser::varize_str(std::string &str, ParseContext &parse_context, bool dequite)
{
    // if (!dequite)
    //     str = dequote(str);
    size_t off = 0;
    replace(str, "\\n", "\n");
    replace(str, "\\\"", "\"");
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
            replace(str, block, "");
        }
        else
        {
            auto var_val = dequote(var_it->second);
            replace(str, block, var_val);
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
            replace(str, block, "");
        }
        else
        {
            auto var_val = dequote(env_it->second);
            replace(str, block, var_val);
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