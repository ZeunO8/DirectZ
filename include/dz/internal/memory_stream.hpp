#pragma once
#include <iostream>
#include "memory_buffer.hpp"

/**
 * @class memory_stream
 * @brief An iostream that uses a memory_buffer for in-memory I/O.
 */
class memory_stream : public std::iostream {
public:
    memory_stream(std::ios_base::openmode ios) : std::iostream(&m_buf) {}
    
    memory_stream(const std::string& s) : std::iostream(&m_buf) {
        m_buf.str(s);
    }

    std::string str() {
        return m_buf.str();
    }
    
    void str(const std::string& s) {
        m_buf.str(s);
    }

    void mod(std::ios_base::openmode ios)
    {
        if (ios & std::ios::trunc)
        {
            m_buf.str("");
        }
    }

private:
    memory_buffer m_buf;
};