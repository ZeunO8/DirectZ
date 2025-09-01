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
        Version(const std::string &version_str);
        Version(const Version &other);

    private:
        uint64_t parseLeadingNumber(std::string_view s) const noexcept;
        std::vector<uint64_t> componentsAsInteger() const;
        static size_t normLenImpl(const std::vector<uint64_t> &v) noexcept;
        size_t normalizedLength(const std::vector<uint64_t> &v) const noexcept;
        int compare(const Version &other) const noexcept;

    public:
        Version &operator=(const Version &other);
        bool operator==(const Version &other) const noexcept;
        bool operator!=(const Version &other) const noexcept;
        bool operator<(const Version &other) const noexcept;
        bool operator<=(const Version &other) const noexcept;
        bool operator>(const Version &other) const noexcept;
        bool operator>=(const Version &other) const noexcept;
    };
}