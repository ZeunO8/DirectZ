#include <iostream>
#include <fstream>
#include <streambuf>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <type_traits>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <memory>

struct FileHandle
{
    enum LOCATION
    {
        PATH,
        ASSET
    };
    LOCATION location;
    std::string path;
    std::shared_ptr<std::iostream> open(std::ios_base::openmode ios)
    {
        auto final_path = path;
        switch (location)
        {
            case ASSET:
            #if defined(ANDROID)
                // get android asset
                break;
            #else
                final_path = /*assets_location + */ path;
            #endif
            case PATH:
                return std::make_shared<std::fstream>(final_path.c_str(), ios);
        }
        throw "";
    }
};

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

template <typename KeyT, typename ValueT>
class OptimizedKeyValueStream {
public:
    struct HeaderEntry {
        KeyT key;
        uint64_t offset;
        uint64_t size;
    };

    OptimizedKeyValueStream(const FileHandle& file_handle):
        file_handle(file_handle),
        m_append_stream_ptr(this->file_handle.open(std::ios::in | std::ios::out | std::ios::app | std::ios::binary)),
        m_append_stream(*m_append_stream_ptr)
    {
        readHeader();
    }

    void write(const KeyT& key, const ValueT& value) {
        std::string buffer = serialize(value);
        auto it = m_keyToIndex.find(key);
        if (it != m_keyToIndex.end()) {
            erase(key);
        }

        m_append_stream.clear();
        m_append_stream.seekp(0, std::ios::end);
        std::streampos pos = m_append_stream.tellp();
        if (pos == std::streampos(-1)) {
            throw std::runtime_error("Invalid tellp() after seekp to end");
        }

        uint64_t offset = static_cast<uint64_t>(pos) - header_size;
        size_t wrote_bytes = 0;
        vlen::write_u64(m_append_stream, buffer.size(), wrote_bytes);
        m_append_stream.write(buffer.data(), buffer.size());
        wrote_bytes += buffer.size();

        HeaderEntry entry { key, offset, wrote_bytes };
        m_keyToIndex[key] = m_entries.size();
        m_entries.push_back(entry);
        writeHeader();
    }

    bool read(const KeyT& key, ValueT& out) {
        auto it = m_keyToIndex.find(key);
        if (it == m_keyToIndex.end()) return false;

        const HeaderEntry& entry = m_entries[it->second];
        m_append_stream.clear();
        m_append_stream.seekg(entry.offset + header_size);

        size_t read_bytes = 0;
        uint64_t size = 0;
        if (!vlen::read_u64(m_append_stream, size, read_bytes))
            return false;
        if (size != (entry.size - read_bytes)) return false;

        std::vector<char> buf(size);
        m_append_stream.read(buf.data(), size);
        read_bytes += size;
        if (m_append_stream.gcount() != static_cast<std::streamsize>(size)) return false;

        out = deserialize(buf);
        return true;
    }

    bool erase(const KeyT& key) {
        auto it = m_keyToIndex.find(key);
        if (it == m_keyToIndex.end()) return false;

        auto j = it->second;
        auto entry_iter = m_entries.begin() + j;

        auto offset = entry_iter->offset;
        auto size = entry_iter->size;

        std::string left, right;
        left.resize(offset + header_size);
        auto end_data = offset + header_size + size;
        m_append_stream.seekg(0, std::ios::end);
        size_t end = m_append_stream.tellg();
        right.resize(end - end_data);
        m_append_stream.seekg(0);
        m_append_stream.read(left.data(), left.size());
        m_append_stream.seekg(end_data);
        m_append_stream.read(right.data(), right.size());
        rewrite_stream(left, right);

        for (uint64_t i = j + 1; i < m_entries.size(); i++)
            m_entries[i].offset -= size;
        m_entries.erase(entry_iter);
        m_keyToIndex.clear();
        for (uint64_t i = 0; i < m_entries.size(); ++i) {
            m_keyToIndex[m_entries[i].key] = i;
        }
        writeHeader();
        return true;
    }

private:
    FileHandle file_handle;
    std::shared_ptr<std::iostream> m_append_stream_ptr;
    std::iostream& m_append_stream;
    std::vector<HeaderEntry> m_entries;
    std::unordered_map<KeyT, uint64_t> m_keyToIndex;
    size_t header_size = 0;

    void readHeader() {
        m_append_stream.clear();

        m_append_stream.seekg(0, std::ios::end);
        auto end = m_append_stream.tellg();
        if (!end)
            return;

        m_append_stream.seekg(0);
        if (!m_append_stream.good()) return;

        uint64_t read_bytes = 0;
        uint64_t count = 0;
        if (!vlen::read_u64(m_append_stream, count, read_bytes))
            return;
        for (uint64_t i = 0; i < count; ++i) {
            HeaderEntry e;
            m_append_stream.read(reinterpret_cast<char*>(&e.key), sizeof(KeyT));
            read_bytes += sizeof(KeyT);
            if (!vlen::read_u64(m_append_stream, e.offset, read_bytes))
                return;
            if (!vlen::read_u64(m_append_stream, e.size, read_bytes))
                return;
            m_entries.push_back(e);
            m_keyToIndex[e.key] = i;
        }

        header_size = read_bytes;
    }

    void rewrite_stream(const std::string& left, const std::string& right)
    {
        auto trunc_stream_ptr = file_handle.open(std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
        auto& trunc_stream = *trunc_stream_ptr;
        trunc_stream.seekp(0);
        trunc_stream.write(left.data(), left.size());
        trunc_stream.write(right.data(), right.size());
    }

    void writeHeader() {
        size_t wrote_bytes = 0;
        std::stringstream ss;
        vlen::write_u64(ss, m_entries.size(), wrote_bytes);
        for (const auto& e : m_entries) {
            ss.write(reinterpret_cast<const char*>(&e.key), sizeof(KeyT));
            wrote_bytes += sizeof(KeyT);
            vlen::write_u64(ss, e.offset, wrote_bytes);
            vlen::write_u64(ss, e.size, wrote_bytes);
        }
        std::string h = ss.str();

        m_append_stream.seekg(0, std::ios::end);
        size_t end = m_append_stream.tellg();
        size_t data_size = end - header_size;
        std::string v;
        v.resize(data_size);
        m_append_stream.seekg(header_size);
        m_append_stream.read(v.data(), data_size);

        rewrite_stream(h, v);

        header_size = wrote_bytes;
    }

    template<typename T>
    std::string serialize_impl(const T& val, std::true_type) {
        return val;
    }

    template<typename T>
    std::string serialize_impl(const T& val, std::false_type) {
        std::string s(sizeof(T), '\0');
        std::memcpy(&s[0], &val, sizeof(T));
        return s;
    }

    std::string serialize(const ValueT& val) {
        return serialize_impl(val, std::is_same<ValueT, std::string>());
    }

    template<typename T>
    T deserialize_impl(const std::vector<char>& buf, std::true_type) {
        return T(buf.begin(), buf.end());
    }

    template<typename T>
    T deserialize_impl(const std::vector<char>& buf, std::false_type) {
        T val;
        std::memcpy(&val, buf.data(), sizeof(T));
        return val;
    }

    ValueT deserialize(const std::vector<char>& buf) {
        return deserialize_impl<ValueT>(buf, std::is_same<ValueT, std::string>());
    }
};

int main() {
    const char* filename = "kvstore.bin";
    std::fstream fcheck(filename, std::ios::in | std::ios::binary);
    if (!fcheck) {
        std::ofstream create(filename, std::ios::out | std::ios::binary);
        // create.put(0); // placeholder byte to ensure valid seekp
        create.flush();
    }
    fcheck.close();

    FileHandle handle{FileHandle::PATH, filename};

    {
        OptimizedKeyValueStream<int, std::string> kv(handle);
        kv.write(1, "hello");
        kv.write(2, "world");
        kv.write(3, "foo");
        kv.write(2, "updated");
    }
    {
        OptimizedKeyValueStream<int, std::string> kv(handle);
        std::string out;
        if (kv.read(2, out)) std::cout << "Key 2: " << out << std::endl;
        if (kv.read(1, out)) std::cout << "Key 1: " << out << std::endl;
        if (kv.read(3, out)) std::cout << "Key 3: " << out << std::endl;
    }
    // kv.erase(1);
    return 0;
}