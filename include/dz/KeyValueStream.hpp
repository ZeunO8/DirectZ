/**
 * @file KeyValueStream.hpp
 * @brief Provides a templated key-value stream for binary serialization to file with in-place update and erase.
 */
#pragma once

#include "FileHandle.hpp"
#include "internal/vlen.hpp"

namespace dz
{
    /**
     * @brief A key-value binary stream writer/reader with support for serialization, deserialization, and deletion.
     * @tparam KeyT Type of the key.
     * @tparam ValueT Type of the value.
     */
    template <typename KeyT, typename ValueT>
    class KeyValueStream {
    public:
        /**
         * @brief Represents metadata for a key-value entry in the file.
         */
        struct HeaderEntry {
            KeyT key;               /**< Key associated with the value. */
            uint64_t offset;        /**< Offset from the header end to the value. */
            uint64_t size;          /**< Size of the value data. */
        };

        /**
         * @brief Constructs a KeyValueStream with a reference to an existing file handle.
         * @param file_handle The file handle to operate on.
         */
        KeyValueStream(FileHandle& file_handle):
            file_handle(file_handle),
            m_append_stream_ptr(file_handle.open(std::ios::in | std::ios::out | std::ios::app | std::ios::binary)),
            m_append_stream(*m_append_stream_ptr)
        {
            readHeader();
        }

        /**
         * @brief Writes a key-value pair to the stream, replacing any existing value for the key.
         * @param key The key to write.
         * @param value The value to write.
         */
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
            vlen::write_u64(m_append_stream, buffer.size() + sizeof(uint64_t), wrote_bytes);
            m_append_stream.write(buffer.data(), buffer.size());
            wrote_bytes += buffer.size();

            HeaderEntry entry { key, offset, wrote_bytes };
            m_keyToIndex[key] = m_entries.size();
            m_entries.push_back(entry);
            writeHeader();
        }

        /**
         * @brief Reads a value by key.
         * @param key The key to look up.
         * @param out Output parameter for the deserialized value.
         * @return True if the value was found and read successfully, false otherwise.
         */
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
            if (size != entry.size) return false;

            std::vector<char> buf(size);
            m_append_stream.read(buf.data(), size);
            read_bytes += size;

            out = deserialize(size, buf);
            return true;
        }

        /**
         * @brief Removes a key-value pair from the stream.
         * @param key The key to erase.
         * @return True if erased successfully, false if the key did not exist.
         */
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
        FileHandle& file_handle;                                     /**< Underlying file handle. */
        std::shared_ptr<std::iostream> m_append_stream_ptr;          /**< Shared stream pointer for read/write. */
        std::iostream& m_append_stream;                               /**< Reference to the stream. */
        std::vector<HeaderEntry> m_entries;                           /**< Metadata for entries. */
        std::unordered_map<KeyT, uint64_t> m_keyToIndex;             /**< Mapping of key to entry index. */
        size_t header_size = 0;                                       /**< Size of the current header. */

        void readHeader() {
            m_append_stream.clear();

            m_append_stream.seekg(0, std::ios::end);
            auto end = m_append_stream.tellg();
            if (!end)
                return;

            m_append_stream.seekg(0);
            if (!m_append_stream.good()) return;

            size_t read_bytes = 0;
            uint64_t count = 0;
            if (!vlen::read_u64(m_append_stream, count, read_bytes))
                return;
            for (uint64_t i = 0; i < count; ++i) {
                HeaderEntry e;
                e.key = deserialize_key(m_append_stream, read_bytes);
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
                serialize_key(ss, e.key, wrote_bytes);
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

        /**
         * @brief Serializes a value into a string buffer.
         * @param val The value to serialize.
         * @return Serialized byte buffer.
         */
        std::string serialize(const ValueT& val) {
            if constexpr (std::is_same_v<ValueT, std::string>)
            {
                return val;
            }
            else if constexpr (std::is_same_v<ValueT, std::istream>)
            {
                std::string str;
                val.seekg(0, std::ios::end);
                auto size = val.tellg();
                str.resize(size);
                val.seekg(0);
                val.read(str.data(), size);
                return str;
            }
            else if constexpr (std::is_same_v<ValueT, Asset>)
            {
                std::string str;
                str.resize(*val.size);
                memcpy(str.data(), val.ptr, *val.size);
                return str;
            }
            else if constexpr (std::is_trivially_copyable_v<ValueT>)
            {
                std::string str;
                str.resize(sizeof(ValueT));
                memcpy(str.data(), &val, sizeof(ValueT));
                return str;
            }
            else
            {
                throw std::runtime_error("Unsupported serialize type");
            }
        }

        /**
         * @brief Deserializes a value from a byte buffer.
         * @param size Size of the value.
         * @param buf Byte buffer to read from.
         * @return Deserialized value.
         */
        ValueT deserialize(size_t size, const std::vector<char>& buf) {
            if constexpr (std::is_same_v<ValueT, std::string>)
            {
                ValueT str;
                str.resize(size);
                memcpy(str.data(), buf.data(), size);
                return str;
            }
            else if constexpr (std::is_same_v<ValueT, Asset>)
            {
                ValueT asset((char*)malloc(size), size, &default_free_deleter::call);
                memcpy(asset.ptr, buf.data(), size);
                return asset;
            }
            else if constexpr (std::is_trivially_copyable_v<ValueT>)
            {
                ValueT val;
                memcpy(&val, buf.data(), size);
                return val;
            }
            else
            {
                throw std::runtime_error("Unsupported deserialize type");
            }
        }

        /**
         * @brief Serializes a key to a stream.
         * @param stream Output stream.
         * @param key Key to serialize.
         * @param wrote_bytes Byte counter updated during write.
         */
        void serialize_key(std::ostream& stream, const KeyT& key, size_t& wrote_bytes)
        {
            if constexpr (std::is_trivially_copyable_v<KeyT>)
            {
                KeyT key;
                stream.write((const char*)&key, sizeof(KeyT));
                wrote_bytes += sizeof(KeyT);
            }
            else if constexpr (std::is_same_v<KeyT, std::string>)
            {
                auto size = key.size();
                vlen::write_u64(stream, size, wrote_bytes);
                stream.write(key.data(), size);
                wrote_bytes += size;
            }
        }

        /**
         * @brief Deserializes a key from a stream.
         * @param stream Stream to read from.
         * @param read_bytes Byte counter updated during read.
         * @return The deserialized key.
         */
        KeyT deserialize_key(std::istream& stream, size_t& read_bytes)
        {
            if constexpr (std::is_trivially_copyable_v<KeyT>)
            {
                KeyT key;
                stream.read((char*)&key, sizeof(KeyT));
                read_bytes += sizeof(KeyT);
                return key;
            }
            else if constexpr (std::is_same_v<KeyT, std::string>)
            {
                KeyT str;
                uint64_t size;
                if (!vlen::read_u64(stream, size, read_bytes))
                    return str;
                str.resize(size);
                stream.read(str.data(), size);
                read_bytes += size;
                return str;
            }
            else
            {
                throw std::runtime_error("Unsupported KeyT for deserialization");
            }
        }
    };
}