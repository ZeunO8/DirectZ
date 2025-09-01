#include <dz/Version.hpp>
#include <regex>
#include <dz/Util.hpp>
namespace dz
{
    Version::Version(const std::string &version_str) : components(split_string(version_str, ".")),
                                                       components_as_integer(componentsAsInteger())
    {
    }
    Version::Version(const Version &other)
    {
        (*this) = other;
    }
    std::vector<size_t> Version::componentsAsInteger()
    {
        std::vector<size_t> integer_components;
        for (auto &component : components)
        {
            static std::regex number_rgx(R"(^\d+)");
            std::smatch match;
            if (std::regex_search(component, match, number_rgx))
            {
                integer_components.push_back(std::stoull(match[0].str()));
            }
            else
            {
                integer_components.push_back(0);
            }
        }
        return integer_components;
    }
    Version &Version::operator=(const Version &other)
    {
        components = other.components;
        components_as_integer = other.components_as_integer;
        return *this;
    }
    bool Version::operator==(const Version &other)
    {
        return components_as_integer == other.components_as_integer;
    }
    bool Version::operator!=(const Version &other)
    {
        return !(*this == other);
    }
    bool Version::operator<=(const Version &other)
    {
        return *this < other || *this == other;
    }
    bool Version::operator<(const Version &other)
    {
        size_t max_len = (std::max)(components_as_integer.size(), other.components_as_integer.size());
        for (size_t i = 0; i < max_len; i++)
        {
            size_t lhs = i < components_as_integer.size() ? components_as_integer[i] : 0;
            size_t rhs = i < other.components_as_integer.size() ? other.components_as_integer[i] : 0;
            if (lhs < rhs)
                return true;
            if (lhs > rhs)
                return false;
        }
        return false;
    }
    bool Version::operator>=(const Version &other)
    {
        return !(*this < other);
    }
    bool Version::operator>(const Version &other)
    {
        return !(*this <= other);
    }
}