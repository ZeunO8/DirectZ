#pragma once
#include <cstring>
namespace dz
{
    template<typename T>
    T* zmalloc(size_t size, const T& value)
    {
        auto ptr = (T*)calloc(size, sizeof(T));
        if (ptr == nullptr)
        {
            return nullptr;
        }
        for (size_t i = 0; i < size; i++)
        {
            new (ptr + i) T(value);
        }
        return ptr;
    }
    template<typename T>
    void zfree(T* ptr, size_t size)
    {
        if (ptr == nullptr)
        {
            return;
        }
        for (size_t i = 0; i < size; i++)
        {
            ptr[i].~T();
        }
        free(ptr);
    }
}