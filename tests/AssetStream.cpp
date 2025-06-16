#include <iostream>
#include <streambuf> // For std::streambuf
#include <vector>    // For std::vector
#include <string>    // For std::string
#include <iomanip>   // For std::hex, std::setw, etc.
#include <algorithm> // For std::min

/**
 * @class VarLenMemBuf
 * @brief A custom stream buffer that uses a dynamically resizable std::vector<char>
 * as its underlying storage.
 *
 * This class inherits from std::streambuf and overrides key virtual functions
 * to provide stream I/O operations on an in-memory buffer. The buffer can
 * grow as needed, and it maintains separate positions for reading (get) and
 * writing (put).
 */
class VarLenMemBuf : public std::streambuf {
public:
    VarLenMemBuf() {
        // Pointers are initially null. They are set on the first write or when
        // content is loaded via str().
    }

    // Non-copyable
    VarLenMemBuf(const VarLenMemBuf&) = delete;
    VarLenMemBuf& operator=(const VarLenMemBuf&) = delete;

    // Method to get the buffer contents.
    std::string str() const {
        // The buffer's content is simply the data stored in the vector.
        return std::string(buffer_.data(), buffer_.size());
    }

    // Method to set the buffer contents.
    void str(const std::string& s) {
        buffer_.assign(s.begin(), s.end());
        // After setting content, update the stream pointers to reflect the new state.
        update_pointers();
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
     *
     * In our design, this is only called when we are appending to the buffer.
     */
    int_type overflow(int_type c = traits_type::eof()) override {
        if (c == traits_type::eof()) {
            return traits_type::eof();
        }

        // Save current get pointer offset because vector reallocation will invalidate pointers.
        std::ptrdiff_t get_offset = eback() ? gptr() - eback() : 0;
        
        // We are writing past the current end, so append the new character.
        buffer_.push_back(traits_type::to_char_type(c));
        
        // The buffer has grown. Update all pointers to reflect the new size,
        // placing the put pointer at the new end, ready for the next append.
        update_pointers(get_offset, buffer_.size());

        return c;
    }

    /**
     * @brief Implements seeking to an absolute or relative position.
     */
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        
        off_type new_pos;
        switch (dir) {
            case std::ios_base::beg:
                new_pos = off;
                break;
            case std::ios_base::cur:
                if (which & std::ios_base::in)
                    new_pos = (gptr() - eback()) + off;
                else if (which & std::ios_base::out)
                    new_pos = (pptr() - pbase()) + off;
                else
                    return pos_type(off_type(-1)); // Should not happen
                break;
            case std::ios_base::end:
                new_pos = buffer_.size() + off;
                break;
            default:
                 return pos_type(off_type(-1));
        }

        // New position must be within the valid range [0, size].
        if (new_pos < 0 || new_pos > static_cast<off_type>(buffer_.size())) {
            return pos_type(off_type(-1));
        }

        // Move the requested pointer(s).
        if (which & std::ios_base::in) {
            setg(eback(), eback() + new_pos, egptr());
        }
        if (which & std::ios_base::out) {
            setp(pbase(), epptr());
            pbump(new_pos);
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
     * @param get_offset The desired offset for the get pointer.
     * @param put_offset The desired offset for the put pointer.
     */
    void update_pointers(std::ptrdiff_t get_offset = 0, std::ptrdiff_t put_offset = 0) {
        char* base = buffer_.data();
        char* end = base + buffer_.size();

        // The get area is always the entire buffer.
        setg(base, base + get_offset, end);
        
        // The put area is also the entire buffer. This allows overwriting anywhere.
        // epptr() is set to the end of the data, so any write at the end
        // will correctly trigger overflow() to append.
        setp(base, end);
        
        // Restore the put pointer to its desired position.
        pbump(put_offset);
    }

    std::vector<char> buffer_;
};


/**
 * @class VarLenMemStream
 * @brief An iostream that uses a VarLenMemBuf for in-memory I/O.
 *
 * This class provides a convenient iostream interface over the custom
 * VarLenMemBuf. It handles the initialization and provides helper methods
 * like str() to easily access the buffer's content.
 */
class VarLenMemStream : public std::iostream {
public:
    VarLenMemStream() : std::iostream(&m_buf) {}
    
    VarLenMemStream(const std::string& s) : std::iostream(&m_buf) {
        m_buf.str(s);
    }

    /**
     * @brief Returns a copy of the internal buffer as a std::string.
     * @return std::string containing the buffer's contents.
     */
    std::string str() {
        // Before getting the string, sync to make sure all written data is readable.
        this->sync();
        return m_buf.str();
    }
    
    /**
     * @brief Sets the content of the buffer from a string.
     * @param s The string to load into the buffer.
     */
    void str(const std::string& s) {
        m_buf.str(s);
    }

private:
    VarLenMemBuf m_buf;
};

// --- Main function to demonstrate usage ---
int main() {
    std::cout << "--- C++ Variable-Length Memory Stream Demo ---" << std::endl << std::endl;

    // 1. Create an instance of the stream
    VarLenMemStream mem_stream;

    // 2. Write data to the stream (demonstrates growing)
    std::cout << "Step 1: Writing data to the stream..." << std::endl;
    int magic_number = 42;
    double pi = 3.14159;
    mem_stream << "Hello, World! " << "Magic number: " << magic_number << ". PI is approx " << pi << ".";
    mem_stream << " Let's write a much longer string to ensure the buffer resizes as needed.";
    mem_stream << std::string(300, '*'); // Write 300 asterisks

    std::cout << "  Write complete. Stream content:" << std::endl;
    std::cout << "  \"" << mem_stream.str() << "\"" << std::endl;
    std::cout << "  Current buffer size: " << mem_stream.str().length() << " chars." << std::endl << std::endl;

    // 3. Read data from the stream (from the beginning)
    std::cout << "Step 2: Reading data back from the beginning..." << std::endl;
    std::string greeting, temp;
    int read_magic;
    double read_pi;

    // The stream's read pointer is still at the beginning because we haven't read yet.
    mem_stream >> greeting >> temp; // Reads "Hello," and "World!"
    mem_stream.ignore(2); // Ignore space and M
    mem_stream >> temp >> temp; // Reads "agic" and "number:"
    mem_stream >> read_magic;   // Reads 42
    mem_stream.ignore(1); // Ignore .
    mem_stream >> temp >> temp >> temp; // Reads "PI", "is", "approx"
    mem_stream >> read_pi;      // Reads 3.14159

    std::cout << "  Data read from stream:" << std::endl;
    std::cout << "  Greeting: " << greeting << " " << temp << std::endl;
    std::cout << "  Magic Number: " << read_magic << std::endl;
    std::cout << "  PI: " << read_pi << std::endl << std::endl;
    
    // 4. Demonstrate seeking
    std::cout << "Step 3: Demonstrating seekg (read) and seekp (write)..." << std::endl;
    
    // Use seekg to jump to the magic number (position 28)
    std::cout << "  Seeking read pointer (seekg) to position 28..." << std::endl;
    mem_stream.seekg(28);
    int magic_again;
    mem_stream >> magic_again;
    std::cout << "  Read after seekg: " << magic_again << std::endl;

    // Use seekp to overwrite data in the middle of the stream
    std::cout << "  Seeking write pointer (seekp) to position 7 to overwrite 'World'..." << std::endl;
    mem_stream.seekp(7);
    mem_stream << "Memory"; // Overwrites "World!" with "Memory"

    // Use seekg to go back to the beginning and read the modified string
    mem_stream.seekg(0);
    std::string line;
    std::getline(mem_stream, line, '.'); // Read up to the first period
    std::cout << "  Reading from start after seekp overwrite: \"" << line << ".\"" << std::endl << std::endl;
    
    std::cout << "Final stream content after all operations:" << std::endl;
    std::cout << "  \"" << mem_stream.str() << "\"" << std::endl;

    return 0;
}
