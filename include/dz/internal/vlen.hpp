#pragma once

namespace vlen {
    inline void write_u64(std::ostream& out, uint64_t value, size_t& wrote_bytes) {
        out.write((const char*)&value, sizeof(value));
        wrote_bytes += sizeof(value);
    }

    inline bool read_u64(std::istream& in, uint64_t& result, size_t& read_bytes) {
        if (!in.good()) return false;
        result = 0;
        in.read((char*)&result, sizeof(result));
        read_bytes += sizeof(result);
        return true;
    }
}