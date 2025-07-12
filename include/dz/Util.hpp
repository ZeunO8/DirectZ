#pragma once
#include <string>
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
}