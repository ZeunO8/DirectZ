/**
 * @file ProgramArgs.hpp
 * @brief Command line argument parser for programs, parsing options and positional arguments.
 */
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace dz
{
    /**
     * @brief Parses and stores command line program arguments.
     * 
     * Splits command line input into options (key-value pairs) and positional arguments.
     * The options map keys are strings stripped of the leading dash.
     * The arguments vector holds all positional arguments in order.
     */
    struct ProgramArgs
    {
        /**
         * @brief Map storing options parsed from command line arguments.
         * 
         * Keys are option names (without leading dash).
         * Values are option values, or empty string if no value was provided.
         */
        std::unordered_map<std::string, std::string> options;

        /**
         * @brief Vector of positional arguments parsed from command line.
         * 
         * Arguments that do not begin with a dash are stored here, preserving order.
         */
        std::vector<std::string> arguments;

        /**
         * @brief Constructs the ProgramArgs object by parsing command line arguments.
         * 
         * Parses `argc` and `argv` from main() according to rules:
         * - Arguments starting with '-' are treated as options.
         * - If an option is followed by a non-option argument, that is its value.
         * - Otherwise, the option value is an empty string.
         * - Non-option arguments are collected as positional arguments.
         * 
         * @param argc Number of command line arguments.
         * @param argv Array of C-string arguments.
         */
        ProgramArgs(int argc, char** argv)
        {
            for (int i = 1; i < argc; ++i)
            {
                std::string current = argv[i];

                // Check if current argument is an option (starts with '-')
                if (current.size() > 1 && current[0] == '-')
                {
                    // Extract key by removing leading dash
                    std::string key = current.substr(1);

                    // Check if there is a next argument to potentially use as value
                    if ((i + 1) < argc)
                    {
                        std::string value = argv[i + 1];

                        // If next argument is empty or starts with '-', treat as no value
                        if (value.empty() || value[0] == '-')
                        {
                            options[key] = "";
                        }
                        else
                        {
                            // Otherwise, consume next argument as option value
                            options[key] = value;
                            ++i; // Skip next argument since it was used as value
                        }
                    }
                    else
                    {
                        // No next argument, option has empty value
                        options[key] = "";
                    }
                }
                else
                {
                    // Argument does not start with '-', treat as positional argument
                    arguments.push_back(current);
                }
            }
        }
    };
}