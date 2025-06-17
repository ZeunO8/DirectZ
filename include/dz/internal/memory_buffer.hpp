#pragma once
#include <string>
#include <streambuf>
#include <vector>

/**
 * @class memory_buffer
 * @brief A custom stream buffer that uses a dynamically resizable std::vector<char>
 * as its underlying storage.
 *
 * This class inherits from std::streambuf and overrides key virtual functions
 * to provide stream I/O operations on an in-memory buffer. The buffer can
 * grow as needed, and it maintains separate positions for reading (get) and
 * writing (put).
 */
class memory_buffer : public std::streambuf {
public:
    memory_buffer() {
        // Set initial pointers for an empty buffer to avoid null pointer issues.
        update_pointers();
    }

    // Non-copyable
    memory_buffer(const memory_buffer&) = delete;
    memory_buffer& operator=(const memory_buffer&) = delete;

    // Method to get the buffer contents.
    std::string str() const {
        if (buffer_.empty()) {
            return {};
        }
        return std::string(buffer_.data(), buffer_.size());
    }

    // Method to set the buffer contents.
    void str(const std::string& s) {
        buffer_.assign(s.begin(), s.end());
        // After setting content, update the stream pointers to reflect the new state.
        update_pointers(0, buffer_.size()); // Set put pointer to the end
    }

protected:
    /**
     * @brief Called when the get area is exhausted. For an in-memory buffer, this means EOF.
     */
    int_type underflow() override {
        return traits_type::eof();
    }

    /**
     * @brief Called when a write occurs at or past the end of the put area.
     * @param c The character to be written.
     * @return The character `c` on success, or traits_type::eof() on failure.
     */
    int_type overflow(int_type c = traits_type::eof()) override {
        if (c == traits_type::eof()) {
            return traits_type::eof();
        }

        std::ptrdiff_t get_offset = eback() ? gptr() - eback() : 0;
        
        buffer_.push_back(traits_type::to_char_type(c));
        
        // The buffer has grown. Update all pointers, placing the put pointer at the new end.
        update_pointers(get_offset, buffer_.size());

        return c;
    }

    /**
     * @brief Implements seeking to an absolute or relative position. This is the corrected version.
     */
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        
        off_type new_pos;

        // Calculate the target absolute position based on direction
        switch (dir) {
            case std::ios_base::beg:
                new_pos = off;
                break;
            case std::ios_base::cur: {
                off_type current_pos = 0;
                if (which & std::ios_base::in) {
                    // If get pointer is valid, use it, otherwise position is 0
                    if (gptr() != nullptr) {
                        current_pos = gptr() - eback();
                    }
                } else if (which & std::ios_base::out) {
                    // If put pointer is valid, use it, otherwise position is 0
                    if (pptr() != nullptr) {
                        current_pos = pptr() - pbase();
                    }
                }
                new_pos = current_pos + off;
                break;
            }
            case std::ios_base::end:
                new_pos = buffer_.size() + off;
                break;
            default:
                return pos_type(off_type(-1));
        }

        // Validate that the new position is within the buffer bounds.
        if (new_pos < 0 || new_pos > static_cast<off_type>(buffer_.size())) {
            return pos_type(off_type(-1));
        }

        // Update the actual get/put pointers based on the valid new position.
        char* base = buffer_.data();
        char* end = base + buffer_.size();
        
        if (which & std::ios_base::in) {
            setg(base, base + new_pos, end);
        }
        if (which & std::ios_base::out) {
            setp(base, end);
            pbump(static_cast<int>(new_pos));
        }

        return pos_type(new_pos);
    }

    /**
     * @brief Implements seeking to an absolute position by deferring to seekoff.
     */
    pos_type seekpos(pos_type pos,
                     std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        return seekoff(pos, std::ios_base::beg, which);
    }

private:
    /**
     * @brief Central utility to set all streambuf pointers based on vector state.
     */
    void update_pointers(std::ptrdiff_t get_offset = 0, std::ptrdiff_t put_offset = 0) {
        // Use .data() which is safe even for empty vectors in C++11 and later.
        char* base = buffer_.data();
        char* end = base + buffer_.size();

        // Set get area
        setg(base, base + get_offset, end);
        
        // Set put area
        setp(base, end);
        pbump(static_cast<int>(put_offset));
    }

    std::vector<char> buffer_;
};