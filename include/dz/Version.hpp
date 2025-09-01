#pragma once
#include <string>
#include <vector>
namespace dz
{
    struct Version
    {
        std::vector<std::string> components;
        std::vector<size_t> components_as_integer;
        Version() = delete;
        Version(const std::string& version_str);
        Version(const Version& other);
        std::vector<size_t> componentsAsInteger();
        Version& operator=(const Version& other);
        bool operator==(const Version& other);
        bool operator!=(const Version& other);
        bool operator<=(const Version& other);
        bool operator<(const Version& other);
        bool operator>=(const Version& other);
        bool operator>(const Version& other);
    };
}