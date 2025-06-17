#pragma once

namespace vlen {
    inline void write_u64(std::ostream& out, uint64_t value, size_t& wrote_bytes) {
        while (value >= 0x80) {
            out.put(static_cast<char>((value & 0x7F) | 0x80));
            wrote_bytes++;
            value >>= 7;
        }
        out.put(static_cast<char>(value));
        wrote_bytes++;
    }

    inline bool read_u64(std::istream& in, uint64_t& result, size_t& read_bytes) {
        if (!in.good()) return false;
        result = 0;
        int shift = 0;
        while (true) {
            int byte = in.get();
            read_bytes++;
            if (byte == EOF) break;
            result |= (static_cast<uint64_t>(byte & 0x7F) << shift);
            if ((byte & 0x80) == 0) break;
            shift += 7;
        }
        return true;
    }
}