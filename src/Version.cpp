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

    static uint64_t mul10_add(uint64_t x, unsigned d, bool &of)
    {
        if (x > UINT64_MAX / 10 || (x == UINT64_MAX / 10 && d > UINT64_MAX % 10))
        {
            of = true;
            return UINT64_MAX;
        }
        return x * 10 + d;
    }

    uint64_t Version::parseLeadingNumber(std::string_view s) const noexcept
    {
        uint64_t v = 0;
        bool overflow = false;
        size_t i = 0;
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        {
            v = mul10_add(v, static_cast<unsigned>(s[i] - '0'), overflow);
            if (overflow)
                return UINT64_MAX; // clamp on overflow
            ++i;
        }
        // If no digits, treat as 0
        return (i == 0) ? 0 : v;
    }

    std::vector<uint64_t> Version::componentsAsInteger() const
    {
        std::vector<uint64_t> out;
        out.reserve(components.size());
        for (const auto &c : components)
            out.push_back(parseLeadingNumber(c));
        return out;
    }

    size_t Version::normLenImpl(const std::vector<uint64_t> &v) noexcept
    {
        size_t n = v.size();
        while (n > 0 && v[n - 1] == 0)
            --n;
        return n;
    }

    size_t Version::normalizedLength(const std::vector<uint64_t> &v) const noexcept
    {
        return normLenImpl(v);
    }

    Version &Version::operator=(const Version &other)
    {
        components = other.components;
        components_as_integer = other.components_as_integer;
        return *this;
    }

    int Version::compare(const Version &other) const noexcept
    {
        const auto &a = components_as_integer;
        const auto &b = other.components_as_integer;

        const size_t an = normalizedLength(a);
        const size_t bn = normalizedLength(b);
        const size_t n = (an > bn) ? an : bn;

        for (size_t i = 0; i < n; ++i)
        {
            const uint64_t lhs = (i < an) ? a[i] : 0;
            const uint64_t rhs = (i < bn) ? b[i] : 0;
            if (lhs < rhs)
                return -1;
            if (lhs > rhs)
                return 1;
        }
        return 0;
    }

    bool Version::operator==(const Version &other) const noexcept { return compare(other) == 0; }
    bool Version::operator!=(const Version &other) const noexcept { return compare(other) != 0; }
    bool Version::operator<(const Version &other) const noexcept { return compare(other) < 0; }
    bool Version::operator<=(const Version &other) const noexcept { return compare(other) <= 0; }
    bool Version::operator>(const Version &other) const noexcept { return compare(other) > 0; }
    bool Version::operator>=(const Version &other) const noexcept { return compare(other) >= 0; }
}