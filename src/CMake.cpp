#include <dz/CMake.hpp>
#include <dz/Util.hpp>

void dz::cmake::Macro::execute(Project &project, const Command &cmd, std::unordered_map<std::string, std::string> &vars) const
{
    std::unordered_map<std::string, std::string> localVars = vars;
    for (size_t i = 0; i < params.size() && i < cmd.arguments.size(); ++i)
        localVars[params[i]] = cmd.arguments[i];
    for (auto &macroCmd : body)
    {
        Command expanded = macroCmd;
        for (auto &arg : expanded.arguments)
        {
            size_t start = 0;
            while (true)
            {
                size_t pos = arg.find("${", start);
                if (pos == std::string::npos)
                    break;
                size_t end = arg.find('}', pos + 2);
                if (end == std::string::npos)
                    break;
                std::string varName = arg.substr(pos + 2, end - (pos + 2));
                auto itv = localVars.find(varName);
                if (itv != localVars.end())
                    arg.replace(pos, end - pos + 1, itv->second);
                start = pos + 1;
            }
        }
        project.addCommand(expanded, localVars);
    }
}

dz::cmake::Project::Project():
    dsl_map(generate_dsl_map())
{}

void dz::cmake::Project::add_lib_or_exe(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
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

void dz::cmake::Project::target_include_directories(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
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

void dz::cmake::Project::target_link_libraries(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
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

void dz::cmake::Project::message(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
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
    std::string args_concat;
    for (auto i = (cmake_msg_type == CMakeMessageType::UNSET ? 0 : 1); i < cmd_arguments_size; i++)
    {
        args_concat += cmd.arguments[i];
        if (i < cmd_arguments_size - 1)
        {
            args_concat += " ";
        }
    }
    switch (cmake_msg_type)
    {
    case CMakeMessageType::UNSET:
        std::cout << "[cmake] " << args_concat << std::endl;
        break;
    case CMakeMessageType::STATUS:
        std::cout << "[cmake] -- " << args_concat << std::endl;
        break;
    case CMakeMessageType::WARNING:
        std::cout << R"([cmake] CMake Warning:
[cmake]   )" << args_concat
                  << std::endl;
        break;
    case CMakeMessageType::FATAL_ERROR:
        throw std::runtime_error(R"([cmake] CMake Error:
[cmake]   )" + args_concat);
    }
}

void dz::cmake::Project::get_filename_component(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
{
    if (cmd_arguments_size < 3)
        return;

    const std::string &varName = cmd.arguments[0];
    std::string inputPath = cmd.arguments[1];
    const std::string &mode = cmd.arguments[2];

    // Expand variables inside inputPath first
    size_t start = 0;
    while (true)
    {
        size_t pos = inputPath.find("${", start);
        if (pos == std::string::npos)
            break;
        size_t end = inputPath.find('}', pos + 2);
        if (end == std::string::npos)
            break;
        std::string varNameSub = inputPath.substr(pos + 2, end - (pos + 2));
        auto itv = vars.find(varNameSub);
        if (itv != vars.end())
            inputPath.replace(pos, end - pos + 1, itv->second);
        start = pos + 1;
    }

    std::filesystem::path p(inputPath);

    if (mode == "ABSOLUTE")
    {
        p = std::filesystem::absolute(p).lexically_normal();
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

void dz::cmake::Project::project(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
{
    if (cmd_arguments_size < 1)
        return;
    name = cmd.arguments[0];
}

void dz::cmake::Project::addCommand(const Command &cmd, std::unordered_map<std::string, std::string> &vars)
{
    auto it_macro = macros.find(cmd.name);
    if (it_macro != macros.end())
    {
        const Macro &m = it_macro->second;
        m.execute(*this, cmd, vars);
        return;
    }

    auto cmd_arguments_size = cmd.arguments.size();

    auto cmd_it = dsl_map.find(cmd.name);
    if (cmd_it == dsl_map.end()) {
        throw std::runtime_error(R"(
[cmake] CMake Error:
[cmake]   Unknown CMake command ")" + cmd.name + R"(".
)");
    }

    cmd_it->second(cmd_arguments_size, cmd, vars);
}

dz::cmake::Project::DSL_Map dz::cmake::Project::generate_dsl_map() {
    DSL_Map map = {
        {
            "add_library",
            std::bind(&Project::add_lib_or_exe, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "add_executable",
            std::bind(&Project::add_lib_or_exe, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "target_include_directories",
            std::bind(&Project::target_include_directories, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "target_link_libraries",
            std::bind(&Project::target_link_libraries, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "project",
            std::bind(&Project::project, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "find_package",
            std::bind(&Project::find_package, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "message",
            std::bind(&Project::message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        },
        {
            "get_filename_component",
            std::bind(&Project::get_filename_component, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        }
    };
    return map;
}

std::string dz::cmake::Project::determineFindPackageDir(const std::string &pkg, const std::unordered_map<std::string, std::string> &vars)
{
    auto x_dir = pkg + "_DIR";
    auto it = vars.find(x_dir);
    if (it != vars.end())
    {
        return it->second;
    }

#if defined(_WIN32)
    {
        std::vector<std::string> candidates = {
            "C:/Program Files/" + pkg + "/lib/cmake/" + pkg,
            "C:/Program Files (x86)/" + pkg + "/lib/cmake/" + pkg};
        for (auto &c : candidates)
        {
            if (std::filesystem::exists(c))
                return c;
        }
        return "C:/Program Files/" + pkg + "/lib/cmake/" + pkg;
    }
#elif defined(__linux__)
    {
        std::vector<std::string> candidates = {
            "/usr/lib/cmake/" + pkg,
            "/usr/local/lib/cmake/" + pkg,
            "/usr/share/cmake/" + pkg};
        for (auto &c : candidates)
        {
            if (std::filesystem::exists(c))
                return c;
        }
        return "/usr/lib/cmake/" + pkg; // fallback
    }

#elif defined(__APPLE__) && !defined(TARGET_OS_IPHONE)
    {
        std::vector<std::string> candidates = {
            "/usr/local/lib/cmake/" + pkg,
            "/opt/homebrew/lib/cmake/" + pkg,
            "/usr/lib/cmake/" + pkg};
        for (auto &c : candidates)
        {
            if (std::filesystem::exists(c))
                return c;
        }
        return "/usr/local/lib/cmake/" + pkg; // fallback
    }

#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // iOS runtime: inside app bundle (Resources)
    // e.g. <AppBundle>/Resources/cmake/<Pkg>
    {
        auto it_prefix = vars.find("CMAKE_RUNTIME_PREFIX");
        if (it_prefix != vars.end())
            return it_prefix->second + "/cmake/" + pkg;
        return "cmake/" + pkg; // relative to bundle Resources
    }

#elif defined(__ANDROID__)
    // Android runtime: inside APK assets or app files dir
    // e.g. /data/data/<app>/files/cmake/<Pkg> or assets/cmake/<Pkg>
    {
        auto it_prefix = vars.find("CMAKE_RUNTIME_PREFIX");
        if (it_prefix != vars.end())
            return it_prefix->second + "/cmake/" + pkg;
        return "assets/cmake/" + pkg; // relative to APK assets
    }

#else
    return "./" + pkg + "/lib/cmake/" + pkg;
#endif
}

void dz::cmake::Project::find_package(size_t cmd_arguments_size, const Command &cmd, std::unordered_map<std::string, std::string> &vars)
{
    if (cmd_arguments_size < 1)
        return;
    auto pkg = cmd.arguments[0];
    bool required = cmd_arguments_size > 1 && cmd.arguments[1] == "REQUIRED";
    auto dir = determineFindPackageDir(pkg, vars);
    if (dir.empty() || !std::filesystem::exists(dir))
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
    auto config = dir + "/" + pkg + "Config.cmake";
    auto config_ = dir + "/" + pkg + "-config.cmake";
    std::string found_config;
    if (std::filesystem::exists(config))
        found_config = config;
    else if (std::filesystem::exists(config_))
        found_config = config_;
    if (found_config.empty())
        goto _throw;
    std::string config_content;
    {
        std::ifstream i(found_config, std::ios::in | std::ios::binary);
        i.seekg(0, std::ios::end);
        auto len = i.tellg();
        i.seekg(0, std::ios::beg);
        config_content.resize(len);
        i.read(config_content.data(), len);
    }
    dz::cmake::CommandParser::parseContentWithProject(*this, config_content, vars);
}

bool dz::cmake::ConditionNode::BoolVar(const std::string &var) const
{
    return var != "false" && var != "FALSE" && var != "off" && var != "OFF";
}

bool dz::cmake::ConditionNode::Evaluate(std::unordered_map<std::string, std::string> &vars) const
{
    switch (op)
    {
    case ConditionOp::Literal:
        return (value == "TRUE" || value == "ON" || value == "1");
    case ConditionOp::Identifier:
    {
        auto it = vars.find(value);
        return it != vars.end() ? BoolVar(it->second) : false;
    }
    case ConditionOp::Not:
        return children[0]->Evaluate(vars);
    case ConditionOp::And:
        return children[0]->Evaluate(vars) && children[1]->Evaluate(vars);
    case ConditionOp::Or:
        return children[0]->Evaluate(vars) || children[1]->Evaluate(vars);
    case ConditionOp::Group:
        return children[0]->Evaluate(vars);
    }
    return false;
}

void dz::cmake::ConditionalBlock::EvaluateAndAdd(Project &proj, std::unordered_map<std::string, std::string> &vars)
{
    bool executed = false;
    for (auto &b : branches)
    {
        if (b.first->Evaluate(vars))
        {
            for (auto &cmd : b.second)
                proj.addCommand(cmd, vars);
            executed = true;
            break;
        }
    }
    if (!executed)
    {
        for (auto &cmd : elseBranch)
            proj.addCommand(cmd, vars);
    }
}

dz::cmake::Project dz::cmake::CommandParser::parseFile(const std::string &path, std::unordered_map<std::string, std::string> &env)
{
    std::ifstream in(path);
    if (!in.is_open())
        return {};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parseContent(content, env);
}

void dz::cmake::CommandParser::parseContentWithProject(Project &project, const std::string &content, std::unordered_map<std::string, std::string> &env)
{
    size_t pos = 0;
    std::stack<ConditionalBlock *> condStack;
    ConditionalBlock *currentBlock = nullptr;
    bool insideMacro = false;
    std::string macroName;
    std::vector<std::string> macroParams;
    std::vector<Command> macroBody;

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
            std::vector<std::string> tokens;
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
            auto condition = parseCondition(args);
            if (!currentBlock)
            {
                currentBlock = new ConditionalBlock();
                condStack.push(currentBlock);
            }
            currentBlock->branches.push_back({condition, {}});
        }
        else if (name == "else")
        {
            if (!currentBlock)
            {
                currentBlock = new ConditionalBlock();
                condStack.push(currentBlock);
            }
            currentBlock->elseBranch.clear();
        }
        else if (name == "endif")
        {
            if (!condStack.empty())
            {
                ConditionalBlock *finished = condStack.top();
                condStack.pop();
                finished->EvaluateAndAdd(project, env);
                delete finished;
                currentBlock = condStack.empty() ? nullptr : condStack.top();
            }
        }
        else
        {
            Command cmd;
            cmd.name = name;
            tokenize(args, cmd.arguments);
            if (insideMacro)
            {
                macroBody.push_back(cmd);
            }
            else if (currentBlock && !currentBlock->branches.empty())
            {
                varize(cmd, env);
                if (!currentBlock->branches.back().first)
                    currentBlock->elseBranch.push_back(cmd);
                else
                    currentBlock->branches.back().second.push_back(cmd);
            }
            else
                project.addCommand(cmd, env);
        }
    }
}

dz::cmake::Project dz::cmake::CommandParser::parseContent(const std::string &content, std::unordered_map<std::string, std::string> &env)
{
    Project project;
    parseContentWithProject(project, content, env);
    return project;
}

std::shared_ptr<dz::cmake::ConditionNode> dz::cmake::CommandParser::parseCondition(const std::string &expr)
{
    auto node = std::make_shared<ConditionNode>();
    node->op = ConditionOp::Identifier;
    node->value = expr;
    return node;
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

void dz::cmake::CommandParser::tokenize(const std::string &s, std::vector<std::string> &out)
{
    std::string current;
    int parenDepth = 0;
    bool inQuotes = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];

        if (c == '"')
        {
            if (inQuotes)
            {
                out.push_back(current);
                current.clear();
                inQuotes = false;
            }
            else
            {
                if (!current.empty())
                {
                    out.push_back(current);
                    current.clear();
                }
                inQuotes = true;
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

void dz::cmake::CommandParser::varize(Command &cmd, const std::unordered_map<std::string, std::string> &env)
{
    for (auto &arg : cmd.arguments)
    {
        size_t off = 0;
        while (true)
        {
            static std::string start_var = "${";
            static std::string end_var = "}";
            auto start_pos = arg.find(start_var, off);
            if (start_pos == std::string::npos)
            {
                break;
            }
            auto var_start_pos = start_pos + start_var.size();
            auto end_pos = arg.find(end_var);
            if (end_pos == std::string::npos)
            {
                throw std::runtime_error(R"([cmake] CMake Error:
[cmake]   Syntax error
[cmake] 
[cmake]   when parsing string
[cmake] 
[cmake]     )" + arg + R"(
[cmake] 
[cmake]   There is an unterminated variable reference.)");
            }
            off = end_pos;
            auto var_len = end_pos - var_start_pos;
            auto block_len = (end_pos - start_pos) + 1;
            auto block = arg.substr(start_pos, block_len);
            auto var = arg.substr(var_start_pos, var_len);
            auto var_it = env.find(var);
            if (var_it == env.end())
            {
                replace(arg, block, "");
            }
            else
            {
                auto var_val = var_it->second;
                replace(arg, block, var_val);
            }
        }
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