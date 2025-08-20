#pragma once
#include <array>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstdint>

namespace dz {
    struct _GUID_
    {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        std::array<uint8_t, 8> Data4;

        // Constructor - generate a new random GUID
        _GUID_()
        {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFFFF);
            std::uniform_int_distribution<uint16_t> dist16(0, 0xFFFF);
            std::uniform_int_distribution<uint16_t>  dist8(0, 0xFF);

            Data1 = dist32(gen);
            Data2 = dist16(gen);
            Data3 = dist16(gen);

            // Data4 first two bytes should follow RFC 4122 variant
            Data4[0] = dist8(gen) & 0xBF | 0x80; // variant bits 10xx
            Data4[1] = dist8(gen);
            for(int i = 2; i < 8; ++i)
                Data4[i] = dist8(gen);

            // Set version to 4 (random)
            Data3 = (Data3 & 0x0FFF) | 0x4000;
        }

        // Convert to standard GUID string format
        std::string to_string() const
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0')
                << std::setw(8) << Data1 << "-"
                << std::setw(4) << Data2 << "-"
                << std::setw(4) << Data3 << "-"
                << std::setw(2) << (int)Data4[0]
                << std::setw(2) << (int)Data4[1] << "-"
                << std::setw(2) << (int)Data4[2]
                << std::setw(2) << (int)Data4[3]
                << std::setw(2) << (int)Data4[4]
                << std::setw(2) << (int)Data4[5]
                << std::setw(2) << (int)Data4[6]
                << std::setw(2) << (int)Data4[7];
            return oss.str();
        }
    };
}