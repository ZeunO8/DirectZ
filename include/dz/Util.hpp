#pragma once
#include <string>
#include <stdlib.h>
#include <iostream>
#include <cctype>

namespace dz {
    inline static std::string to_lower(const std::string& str) {
        std::string nstr(str.size(), 0);
        size_t i = 0;
        for (auto& elem : str) {
            nstr[i++] = std::tolower(elem);
        }
        return nstr;
    }

    inline static std::string to_upper(const std::string& str) {
        std::string nstr(str.size(), 0);
        size_t i = 0;
        for (auto& elem : str) {
            nstr[i++] = std::toupper(elem);
        }
        return nstr;
    }

    inline static std::wstring string_to_wstring(const std::string& str) {
        std::wstring wstr;
        size_t size;
        wstr.resize(str.length());
        mbstowcs(wstr.data(), str.c_str(), str.size());
        return wstr;
    }

    inline static std::string wstring_to_string(const std::wstring& wstr) {
        std::string str;
        size_t size;
        str.resize(wstr.length());
        wcstombs(str.data(), wstr.c_str(), wstr.size());
        return str;
    }

    inline static bool replace(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = str.find(from);
        if(start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }

    inline static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
}