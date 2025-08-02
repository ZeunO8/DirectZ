#pragma once
#include <string>
#include <cctype>
#include <codecvt>

namespace dz {
    inline static std::string to_lower(const std::string& str) {
        std::string nstr(str.size(), 0);
        size_t i = 0;
        for (auto& elem : str) {
            nstr[i++] = std::tolower(elem);
        }
        return nstr;
    }

    inline static std::wstring string_to_wstring(const std::string& str) {
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter = {};
        return converter.from_bytes(str);
    }
}