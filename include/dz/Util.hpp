#pragma once
#include <string>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <unordered_set>
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

    inline static std::vector<std::vector<std::string>> split_ranges(const std::vector<std::string> &ranges, const std::string &split_str)
    {
        std::vector<std::vector<std::string>> split;
        split.reserve(ranges.size());
        for (auto& str : ranges)
        {
            split.push_back(split_string(str, split_str));
        }
        return split;
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

    inline static bool is_quoted(const auto& var_value)
    {
        return (var_value.size() >= 2 && var_value[0] == '"' && var_value[var_value.size() - 1] == '"');
    }

    inline static std::string dequote(const std::string &quoted_str)
    {
        if (!is_quoted(quoted_str))
            return quoted_str;
        auto new_str = quoted_str;
        if (new_str.empty())
            assert(false);
        if (new_str[0] == '"')
            new_str.erase(0, 1);
        if (new_str.empty())
            assert(false);
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

    inline static auto in_vec(const auto vec, const auto& val)
    {
        auto vec_begin = vec.begin();
        auto vec_end = vec.end();
        return std::find(vec_begin, vec_end, val) != vec_end;
    }

    inline static auto in_map(const auto& map, const auto& val)
    {
        return map.find(val) != map.end();
    }

    inline static auto insert_to_map_from_vec_with_string_prefix_prepended(auto& map_to, const auto& vec_from, const std::string& prefix)
    {
        auto abs_prefix = (prefix.empty() ? "" : (prefix + "_"));
        for (auto& var_str : vec_from) {
            map_to[abs_prefix + var_str];
        }
    }

    inline static auto insert_to_map_from_map(auto& map_to, const auto& map_from)
    {
        map_to.insert(map_from.begin(), map_from.end());
    }

    inline static auto remove_to_map_from_map(auto& map_to, const auto& map_from)
    {
        for (auto& [key, val] : map_from) {
            map_to.erase(key);
        }
    }

    inline static auto is_var(const auto &var_value)
    {
        auto start_pos = var_value.find("${");
        if (start_pos == std::string::npos)
            return false;
        auto end_pos = var_value.find("}", start_pos);
        return end_pos != std::string::npos;
    }

    inline static auto is_env_var(const auto &var_value)
    {
        auto start_pos = var_value.find("${ENV");
        if (start_pos == std::string::npos)
            return false;
        auto end_pos = var_value.find("}", start_pos);
        return end_pos != std::string::npos;
    }

    inline static bool is_numeric(const auto& var_value)
    {
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
        return false;
    }

    inline static auto is_literal(const auto &var_value)
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
        if (is_numeric(var_value))
            return true;

        // String literal ("val")
        if (is_quoted(var_value))
            return true;

        return false;
    }
    inline static auto truthy(const std::string &s)
    {
        std::string u = to_upper(s);
        static std::unordered_set<std::string> falsey_set = {
            "0",
            "FALSE",
            "OFF",
            "NO",
            "N",
            "IGNORE",
            "NOTFOUND",
        };
        auto falsey_it = falsey_set.find(u);
        if (s.empty() || falsey_it != falsey_set.end() || (u.size() > 9 && u.rfind("-NOTFOUND") == u.size() - 9))
            return false;
        return true;
    }

    static size_t get_range_min(const std::vector<std::vector<std::string>>& ranges)
    {
        size_t x = (std::numeric_limits<size_t>::max)();
        for (auto& range_ns : ranges)
        {
            auto range_size = range_ns.size();
            x = (std::min)(range_size, x);
        }
        return x;
    }

    static size_t get_range_max(const std::vector<std::vector<std::string>>& ranges)
    {
        size_t x = (std::numeric_limits<size_t>::lowest)();
        for (auto& range_ns : ranges)
        {
            auto range_size = range_ns.size();
            x = (std::max)(range_size, x);
        }
        return x;
    }
}