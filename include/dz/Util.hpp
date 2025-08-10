#pragma once
#include <string>
#include <cstdlib>
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
        mbstowcs_s(&size,&wstr[0],wstr.size()+1,str.c_str(),str.size());
        return wstr;
    }

    inline static std::string wstring_to_string(const std::wstring& wstr) {
        std::string str;
        size_t size;
        str.resize(wstr.length());
        wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());
        return str;
    }
}