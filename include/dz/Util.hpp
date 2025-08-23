#pragma once
#include <string>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cctype>
#if defined(_WIN32)
#include <windows.h>
#else
#include <cstdlib>
extern char **environ;
#endif

namespace dz
{
    inline static std::string to_lower(const std::string &str)
    {
        std::string nstr(str.size(), 0);
        size_t i = 0;
        for (auto &elem : str)
        {
            nstr[i++] = std::tolower(elem);
        }
        return nstr;
    }

    inline static std::string to_upper(const std::string &str)
    {
        std::string nstr(str.size(), 0);
        size_t i = 0;
        for (auto &elem : str)
        {
            nstr[i++] = std::toupper(elem);
        }
        return nstr;
    }

    inline static std::wstring string_to_wstring(const std::string &str)
    {
        std::wstring wstr;
        size_t size;
        wstr.resize(str.length());
        mbstowcs(wstr.data(), str.c_str(), str.size());
        return wstr;
    }

    inline static std::string wstring_to_string(const std::wstring &wstr)
    {
        std::string str;
        size_t size;
        str.resize(wstr.length());
        wcstombs(str.data(), wstr.c_str(), wstr.size());
        return str;
    }

    inline static bool replace(std::string &str, const std::string &from, const std::string &to)
    {
        size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }

    inline static void replaceAll(std::string &str, const std::string &from, const std::string &to)
    {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }

    inline static std::vector<std::string> split_string(const std::string &str, const std::string &split_str)
    {
        if (str.empty())
            return {};

        std::vector<std::string> result;
        if (split_str.empty())
        {
            result.push_back(str);
            return result;
        }

        size_t start = 0;
        size_t pos = 0;
        while ((pos = str.find(split_str, start)) != std::string::npos)
        {
            result.push_back(str.substr(start, pos - start));
            start = pos + split_str.size();
        }
        result.push_back(str.substr(start));

        return result;
    }

    inline static std::string join_string_vec(const std::vector<std::string> &vec, const std::string &join_str)
    {
        std::string joined_str;
        auto vec_size = vec.size();
        auto vec_data = vec.data();
        for (size_t i = 0; i < vec_size; ++i)
        {
            joined_str += vec_data[i];
            if (i < (vec_size - 1))
                joined_str += join_str;
        }
        return joined_str;
    }

    inline static std::string dequote(const std::string &quoted_str)
    {
        auto new_str = quoted_str;
        if (new_str.empty())
            return new_str;
        if (new_str[0] == '"')
            new_str.erase(0, 1);
        if (new_str.empty())
            return new_str;
        if (new_str[new_str.size() - 1] == '"')
            new_str.erase(new_str.size() - 1, 1);
        return new_str;
    }

    inline static std::unordered_map<std::string, std::string> get_all_env_vars()
    {
        std::unordered_map<std::string, std::string> env_map;

#if defined(_WIN32)
        LPWCH env_strings = GetEnvironmentStringsW();
        if (!env_strings)
        {
            return env_map;
        }

        LPWCH var = env_strings;
        while (*var)
        {
            std::wstring w_entry(var);
            std::string entry(w_entry.begin(), w_entry.end());
            size_t pos = entry.find('=');
            if (pos != std::string::npos)
            {
                std::string key = entry.substr(0, pos);
                std::string value = entry.substr(pos + 1);
                env_map[key] = value;
            }
            var += w_entry.size() + 1;
        }

        FreeEnvironmentStringsW(env_strings);

#else
        for (char **current = environ; *current; ++current)
        {
            std::string entry(*current);
            size_t pos = entry.find('=');
            if (pos != std::string::npos)
            {
                std::string key = entry.substr(0, pos);
                std::string value = entry.substr(pos + 1);
                env_map[key] = value;
            }
        }
#endif

        return env_map;
    }
}