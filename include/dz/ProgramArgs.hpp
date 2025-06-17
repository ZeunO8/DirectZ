#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace dz
{
    struct ProgramArgs
    {
        std::unordered_map<std::string, std::string> options;
        std::vector<std::string> arguments;

        ProgramArgs(int argc, char** argv)
        {
            for (int i = 1; i < argc; ++i)
            {
                std::string current = argv[i];

                if (current.size() > 1 && current[0] == '-')
                {
                    std::string key = current.substr(1);

                    if ((i + 1) < argc)
                    {
                        std::string value = argv[i + 1];
                        if (value.empty() || value[0] == '-')
                        {
                            options[key] = "";
                        }
                        else
                        {
                            options[key] = value;
                            ++i;
                        }
                    }
                    else
                    {
                        options[key] = "";
                    }
                }
                else
                {
                    arguments.push_back(current);
                }
            }
        }
    };
}