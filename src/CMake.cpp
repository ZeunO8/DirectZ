#include <dz/CMake.hpp>
#include <dz/Util.hpp>
#include <queue>
#include <unordered_set>
#include <cassert>

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

void dz::cmake::Project::add_lib_or_exe(size_t cmd_arguments_size, const Command &cmd)
{

    if (!cmd_arguments_size)
        return;
    Target::Type type = (cmd.name == "add_library") ? Target::Type::Library : Target::Type::Executable;
    auto target = std::make_shared<Target>(type, cmd.arguments[0]);
    for (size_t i = 1; i < cmd_arguments_size; ++i)
    {
        auto &arg = cmd.arguments[i];
        if (type == Target::Type::Library)
        {
            if (arg == "SHARED")
            {
                target->setShared();
                continue;
            }
            else if (arg == "STATIC")
            {
                target->setStatic();
                continue;
            }
        }
        target->addArgument(arg);
    }
    targets[target->getName()] = std::move(target);
}

void dz::cmake::Project::target_include_directories(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 2)
        return;
    auto it = targets.find(cmd.arguments[0]);
    if (it == targets.end())
        return;
    for (size_t i = 1; i < cmd_arguments_size; ++i)
    {
        auto &arg = cmd.arguments[i];
        if (arg == "PRIVATE" || arg == "PUBLIC")
            continue;
        it->second->addIncludeDir(arg);
    }
}

void dz::cmake::Project::target_link_libraries(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 2)
        return;
    auto it = targets.find(cmd.arguments[0]);
    if (it == targets.end())
        return;
    for (size_t i = 1; i < cmd_arguments_size; ++i)
    {
        auto &arg = cmd.arguments[i];
        if (arg == "PRIVATE" || arg == "PUBLIC")
            continue;
        it->second->addLinkLib(arg);
    }
}

void dz::cmake::Project::message(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 1)
    {
        std::cout << std::endl;
        return;
    }
    auto msg_type_str = cmd.arguments[0];
    CMakeMessageType cmake_msg_type = CMakeMessageType::UNSET;
    if (msg_type_str == "STATUS")
    {
        cmake_msg_type = CMakeMessageType::STATUS;
    }
    else if (msg_type_str == "WARNING")
    {
        cmake_msg_type = CMakeMessageType::WARNING;
    }
    else if (msg_type_str == "FATAL_ERROR")
    {
        cmake_msg_type = CMakeMessageType::FATAL_ERROR;
    }
    else if (msg_type_str == "AUTHOR_WARNING")
    {
        cmake_msg_type = CMakeMessageType::AUTHOR_WARNING;
    }
    std::string args_concat;
    auto og = (cmake_msg_type == CMakeMessageType::UNSET ? 0 : 1);
    for (auto i = og; i < cmd_arguments_size; i++)
    {
        std::string concat = "[cmake] ";
        if (i == og && cmake_msg_type != CMakeMessageType::UNSET)
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
}

void dz::cmake::Project::get_filename_component(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 3)
        return;

    const std::string &varName = cmd.arguments[0];
    std::string inputPath = cmd.arguments[1];
    const std::string &mode = cmd.arguments[2];

    // Expand variables inside inputPath first
    // cmake::CommandParser::varize_str(inputPath, *context_sh_ptr);

    auto &vars = context_sh_ptr->vars;

    std::filesystem::path p(inputPath);

    if (mode == "ABSOLUTE")
    {
        p = std::filesystem::absolute(p).lexically_normal();
        vars[varName] = p.string();
    }
    else if (mode == "REALPATH")
    {
        try
        {
            p = std::filesystem::weakly_canonical(p);
        }
        catch (const std::exception &)
        {
            // If even weakly_canonical somehow fails, fallback to absolute normalized
            p = std::filesystem::absolute(p).lexically_normal();
        }
        vars[varName] = p.string();
    }
    else if (mode == "PATH")
    {
        vars[varName] = p.parent_path().string();
    }
    else if (mode == "NAME")
    {
        vars[varName] = p.filename().string();
    }
    else
    {
        // fallback: just assign raw path
        vars[varName] = p.string();
    }
}

void dz::cmake::Project::project(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 1)
        return;
    name = cmd.arguments[0];
}

void dz::cmake::Project::list(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 3)
        return;
    auto arguments_data = cmd.arguments.data();
    auto &action = arguments_data[0];
    auto &var_name = arguments_data[1];
    if (var_name.empty())
        return;
    auto &vars = context_sh_ptr->vars;
    auto &var = vars[var_name];
    for (size_t i = 2; i < cmd_arguments_size; i++)
    {
        auto &val = arguments_data[i];
        if (action == "APPEND")
        {
            if (!var.empty())
                var += ";";
            var += dequote(val);
        }
        else if (action == "REMOVE_ITEM")
        {
            auto var_split = split_string(var, ";");
            auto f_it = std::find(var_split.begin(), var_split.end(), val);
            if (f_it != var_split.end())
            {
                var_split.erase(f_it);
                var = join_string_vec(var_split, ";");
            }
        }
    }
}

void dz::cmake::Project::cmake_policy(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size == 0)
        return;

    auto &context = *context_sh_ptr;
    auto &action = cmd.arguments[0];

    if (action == "PUSH")
    {
        context.policy_stack.emplace_front(); // push empty scope
        context.policy_push_just_called = true;
        return;
    }
    else if (action == "POP")
    {
        if (context.policy_stack.empty())
            return;

        auto policy_sh_ptr = context.policy_stack.front();
        context.policy_stack.pop_front(); // discard scope
        auto &policy = *policy_sh_ptr;
        context.policy_set_map.erase(policy.policy_name);
        return;
    }
    else if (action == "SET")
    {
        if (cmd_arguments_size < 3)
            return;

        auto &policy_name = cmd.arguments[1];
        auto &policy_value = cmd.arguments[2];

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
    }
}

void dz::cmake::Project::set(size_t cmd_arguments_size, const Command &cmd)
{
    if (cmd_arguments_size < 2)
        return;
    auto &var_name = cmd.arguments[0];
    if (var_name.empty())
        return;
    auto &vars = context_sh_ptr->vars;
    auto &var = vars[var_name];
    for (size_t i = 1; i < cmd_arguments_size; i++)
    {
        if (!var.empty())
            var += ";";
        auto &var_val = cmd.arguments[i];
        var += dequote(var_val);
    }
}

template<typename F>
std::function<void(size_t, const dz::cmake::Command&)> abstractify_cmake_function(
    const std::shared_ptr<dz::cmake::ParseContext>& context_sh_ptr,
    const std::string& prefix,
    const dz::cmake::ValueVector& options,
    const dz::cmake::ValueVector& one_value_keywords,
    const dz::cmake::ValueVector& multi_value_keywords,
    F run_with_abstract_set,
    size_t parse_argv = 0
)
{
    static auto in_vec = [](const auto vec, const auto& str) -> bool {
        auto vec_begin = vec.begin();
        auto vec_end = vec.end();
        return std::find(vec_begin, vec_end, str) != vec_end;
    };
    static auto in_map = [](const auto& map, const auto& str) -> bool {
        return map.find(str) != map.end();
    };
    static auto insert_to_map_from_vec_with_prefix_prepended = [](auto& map_to, const auto& vec_from, const std::string& prefix) {
        for (auto& var_str : vec_from) {
            map_to[prefix + "_" + var_str];
        }
    };
    static auto insert_to_map_from_map = [](auto& map_to, const auto& map_from) {
        map_to.insert(map_from.begin(), map_from.end());
    };
    return [=](auto cmd_arguments_size, const auto& cmd){
        dz::cmake::VariableMap all_keys_and_vals;

        insert_to_map_from_vec_with_prefix_prepended(all_keys_and_vals, multi_value_keywords, prefix);
        insert_to_map_from_vec_with_prefix_prepended(all_keys_and_vals, one_value_keywords, prefix);
        insert_to_map_from_vec_with_prefix_prepended(all_keys_and_vals, options, prefix);

        std::string parsing_key;
        std::string* parsing_val = nullptr;
        dz::cmake::ValueVector new_arguments;

        for (size_t i = 0; i < cmd_arguments_size; i++)
        {
            auto& current_arg = cmd.arguments[i];
            if (in_map(all_keys_and_vals, prefix + "_" + current_arg)) {
                parsing_key = current_arg;
                parsing_val = &all_keys_and_vals[prefix + "_" + parsing_key];
                if (in_vec(options, parsing_key))
                {
                    (*parsing_val) = "ON";
                }
            }
            else if (parsing_key.empty()) {
                if ((parse_argv != 0 && new_arguments.size() < parse_argv) || !parse_argv) {
                    new_arguments.push_back(current_arg);
                }
            }
            else if (in_vec(one_value_keywords, parsing_key))
            {
                if (parsing_val->empty()) {
                    (*parsing_val) = current_arg;
                    parsing_key.clear();
                }
                else {
                    throw std::runtime_error("[cmake] " + parsing_key + "expects one value)");
                }
            }
            else if (in_vec(multi_value_keywords, parsing_key))
            {
                (*parsing_val) += ((parsing_val->empty() ? "" : ";") + current_arg);
            }
        }
        {
            dz::cmake::Command new_cmd = cmd;
            new_cmd.arguments = new_arguments;
            auto& context = *context_sh_ptr;
            auto old_marked_vars = context.marked_vars;
            auto old_context_vars = context.vars;
            insert_to_map_from_map(context.vars, all_keys_and_vals);
            run_with_abstract_set(new_cmd.arguments.size(), new_cmd);
            auto new_context_vars = context.vars;
            context.vars = old_context_vars;
            context.restore_marked_vars(new_context_vars);
            context.marked_vars = old_marked_vars;
        }
    };
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
    auto find_package_impl = [&](auto cmd_arguments_size, const Command& cmd)
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
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, find_package_impl);
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
        auto mark_bool = clear != "ON";
        auto& force = context.vars["___MARK_AS_ADVANCED_FORCE"];
        auto force_bool = force == "ON";
        for (size_t i = 0; i < cmd_arguments_size; i++)
        {
            auto& var_name = cmd.arguments[i];
            context.mark_var(var_name, mark_bool || force_bool);
        }
    };
    auto context_function = abstractify_cmake_function(context_sh_ptr, prefix, options, one_value_keywords, multi_value_keywords, mark_as_advanced_impl);
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
                  << "\t\tLink Type" << target.GetLinkTypeStr() << std::endl;
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
        {"add_library",
         std::bind(&Project::add_lib_or_exe, this, std::placeholders::_1, std::placeholders::_2)},
        {"add_executable",
         std::bind(&Project::add_lib_or_exe, this, std::placeholders::_1, std::placeholders::_2)},
        {"target_include_directories",
         std::bind(&Project::target_include_directories, this, std::placeholders::_1, std::placeholders::_2)},
        {"target_link_libraries",
         std::bind(&Project::target_link_libraries, this, std::placeholders::_1, std::placeholders::_2)},
        {"project",
         std::bind(&Project::project, this, std::placeholders::_1, std::placeholders::_2)},
        {"find_package",
         std::bind(&Project::find_package, this, std::placeholders::_1, std::placeholders::_2)},
        {"message",
         std::bind(&Project::message, this, std::placeholders::_1, std::placeholders::_2)},
        {"get_filename_component",
         std::bind(&Project::get_filename_component, this, std::placeholders::_1, std::placeholders::_2)},
        {"list",
         std::bind(&Project::list, this, std::placeholders::_1, std::placeholders::_2)},
        {"cmake_policy",
         std::bind(&Project::cmake_policy, this, std::placeholders::_1, std::placeholders::_2)},
        {"set",
         std::bind(&Project::set, this, std::placeholders::_1, std::placeholders::_2)},
        {"find_path",
         std::bind(&Project::find_path, this, std::placeholders::_1, std::placeholders::_2)},
        {"mark_as_advanced",
         std::bind(&Project::mark_as_advanced, this, std::placeholders::_1, std::placeholders::_2)},
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
    if (cmd_arguments_size < 1)
        return;
    auto pkg = cmd.arguments[0];
    bool required = cmd_arguments_size > 1 && cmd.arguments[1] == "REQUIRED";
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
    auto &vars = context_sh_ptr->vars;
    auto &current_list_dir = vars["CMAKE_CURRENT_LIST_DIR"];
    auto old_list_dir = current_list_dir;
    current_list_dir = f_dir;
    dz::cmake::CommandParser::parseContentWithProject(*this, config_content);
    current_list_dir = old_list_dir;
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
        std::function<std::string(const ConditionNode &)> toString = [&](const ConditionNode &n)
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
        std::function<bool(const ConditionNode &)> toBool = [&](const ConditionNode &n)
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
        auto is_literal = [&](const auto &var_value)
        {
            static const std::unordered_set<std::string> lit_map = {
                "TRUE", "true", "FALSE", "false",
                "ON", "on", "OFF", "off",
                "YES", "yes", "NO", "no",
                "Y", "y", "N", "n",
                "IGNORE", "ignore",
                "NOTFOUND", "notfound"};

            // Exact match in known literal set
            if (lit_map.find(var_value) != lit_map.end())
                return true;

            // Special case: anything ending with -NOTFOUND is a literal
            if (var_value.size() >= 9)
            {
                std::string suffix = var_value.substr(var_value.size() - 9);
                std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::toupper);
                if (suffix == "-NOTFOUND")
                    return true;
            }

            // Numeric literal (decimal, hex, octal)
            try
            {
                size_t idx = 0;

                char *p;
                long converted = strtol(var_value.c_str(), &p, 10);
                if (!(*p))
                {
                    return true;
                }
            }
            catch (...)
            {
            }

            // String literal ("val")
            auto q_start_pos = var_value.find("\"");
            if (q_start_pos == std::string::npos)
                return false;
            auto q_end_pos = var_value.find("\"", q_start_pos + 1);
            if (q_end_pos != std::string::npos)
                return true;

            return false;
        };
        auto is_var = [&](const auto &var_value)
        {
            auto start_pos = var_value.find("${");
            auto end_pos = var_value.find("}");
            return (start_pos != std::string::npos && end_pos != std::string::npos);
        };
        auto truthize = [&]()
        {
            for (auto &child_sh_ptr : children)
            {
                auto &child = *child_sh_ptr;
                if (child.op == ConditionOp::IdentifierOrLiteral)
                {
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
                std::string lhs = toString(*children[i]);
                i += 2;
                if (i >= children.size())
                    break;
                std::string rhs = toString(*children[i]);
                ++i;
                term = (lhs == rhs);
                if (invert)
                    term = !term;
            }
            else if (i + 1 < children.size() && children[i + 1]->op == ConditionOp::InList)
            {
                auto lhs = toString(*children[i]);
                i += 2;
                if (i >= children.size())
                    break;
                auto rhs = toString(*children[i]);
                auto var = vars[rhs];
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
                auto var_name = toString(*children[i]);
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

void dz::cmake::ConditionalBlock::Evaluate(Project &project)
{
    bool executed = false;
    auto &context = *project.context_sh_ptr;
    for (auto &b : branches)
    {
        if (b.first->Evaluate(project))
        {
            for (auto &evaluable_sh_ptr : b.second)
            {
                auto &evaluable = *evaluable_sh_ptr;
                evaluable.Varize(project);
                evaluable.Evaluate(project);
            }
            executed = true;
            break;
        }
    }
    if (!executed)
    {
        for (auto &evaluable_sh_ptr : elseBranch)
        {
            auto &evaluable = *evaluable_sh_ptr;
            evaluable.Varize(project);
            evaluable.Evaluate(project);
        }
    }
}

void dz::cmake::ConditionalBlock::Varize(Project &project)
{
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
    ConditionalBlock *currentBlock = nullptr;
    bool insideMacro = false;
    int if_depth = 0;
    std::stack<int> current_else_depth_stack;
    std::string macroName;
    dz::cmake::ValueVector macroParams;
    std::vector<Command> macroBody;

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

        if (name == "macro")
        {
            dz::cmake::ValueVector tokens;
            tokenize(args, tokens);
            if (tokens.empty())
                continue;
            macroName = tokens[0];
            macroParams.assign(tokens.begin() + 1, tokens.end());
            macroBody.clear();
            insideMacro = true;
        }
        else if (name == "endmacro")
        {
            if (insideMacro)
            {
                Macro m;
                m.params = macroParams;
                m.body = macroBody;
                project.macros[macroName] = std::move(m);
                insideMacro = false;
                macroName.clear();
                macroParams.clear();
                macroBody.clear();
            }
        }
        else if (name == "if" || name == "elseif")
        {
            if (!currentBlock)
            {
                currentBlock = new ConditionalBlock();
            }
            auto is_if = (name == "if");
            if (is_if)
                if_depth++;
            auto condition = parseCondition(args);
            if (if_depth > 1 && is_if)
            {
                auto innerBlock = new ConditionalBlock();
                auto innerBlock_sh_ptr = std::shared_ptr<ConditionalBlock>(innerBlock, [](ConditionalBlock* p) { delete p; });
                innerBlock->owned_by_sh_ptr = true;
                innerBlock->branches.push_back({condition, {}});
                innerBlock->parent_block = currentBlock;
                currentBlock->branches.back().second.push_back(innerBlock_sh_ptr);
                currentBlock = innerBlock;
            }
            else
            {
                currentBlock->branches.push_back({condition, {}});
            }
        }
        else if (name == "else")
        {
            if (!currentBlock)
            {
                throw std::runtime_error("[cmake] -- else without opening if");
            }
            currentBlock->elseBranch.clear();
            current_else_depth_stack.push(if_depth);
        }
        else if (name == "endif")
        {
            if (!current_else_depth_stack.empty() && current_else_depth_stack.top() == if_depth)
                current_else_depth_stack.pop();
            if (if_depth == 1)
            {
                currentBlock->Evaluate(project);
                if (!currentBlock->owned_by_sh_ptr)
                    delete currentBlock;
                currentBlock = nullptr;
            }
            else
            {
                currentBlock = currentBlock->parent_block;
            }
            if_depth--;
        }
        else
        {
            auto cmd_sh_ptr = std::make_shared<Command>();
            auto &cmd = *cmd_sh_ptr;
            cmd.name = name;
            tokenize(args, cmd.arguments);
            if (insideMacro)
            {
                macroBody.push_back(cmd);
            }
            else if (currentBlock && !currentBlock->branches.empty())
            {
                if (!current_else_depth_stack.empty() && current_else_depth_stack.top() == if_depth)
                    currentBlock->elseBranch.push_back(cmd_sh_ptr);
                else
                    currentBlock->branches.back().second.push_back(cmd_sh_ptr);
            }
            else
            {
                varize(cmd, context);
                cmd.Evaluate(project);
            }
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

std::shared_ptr<dz::cmake::ConditionNode> dz::cmake::CommandParser::parseCondition(const std::string &expr)
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
    dz::cmake::ValueVector args;
    tokenize(expr, args);

    auto curr_node = std::make_shared<ConditionNode>();
    curr_node->op = ConditionOp::Group;
    curr_node->value = expr;

    curr_node->children.reserve(args.size());
    for (auto &arg : args)
    {
        auto cond_node = std::make_shared<ConditionNode>();
        auto cond_it = condition_op_map.find(arg);
        if (cond_it != condition_op_map.end())
        {
            auto op = cond_it->second;
            cond_node->op = op;
            cond_node->value = arg;
        }
        else
        {
            cond_node->op = ConditionOp::IdentifierOrLiteral;
            cond_node->value = arg;
        }
        curr_node->children.push_back(cond_node);
    }

    return curr_node;
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

void dz::cmake::CommandParser::varize_str(std::string &str, ParseContext &parse_context)
{
    str = dequote(str);
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
        off = end_pos;
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
        off = end_pos;
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