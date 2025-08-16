#pragma once
#include "SharedMemory.hpp"

namespace dz
{
    template <typename T>
    struct SharedMemoryPtr
    {
        SharedMemory shm;
        T *ptr;

        SharedMemoryPtr() : shm(), ptr(nullptr) {}

        bool Create(const std::string &name)
        {
            if (!shm.Create(name, sizeof(T)))
                return false;
            if (!shm.Map())
                return false;
            ptr = ::new (shm.Data()) T(); // placement new, default ctor
            return true;
        }

        bool Open(const std::string &name)
        {
            if (!shm.Open(name, sizeof(T)))
                return false;
            if (!shm.Map())
                return false;
            ptr = reinterpret_cast<T *>(shm.Data());
            return true;
        }

        void Destroy()
        {
            if (ptr)
            {
                ptr->~T();
                ptr = nullptr;
            }
            shm.Destroy();
        }

        void Close()
        {
            ptr = nullptr;
            shm.Close();
        }

        T *operator->() { return ptr; }
        const T *operator->() const { return ptr; }
        T &operator*() { return *ptr; }
        const T &operator*() const { return *ptr; }

        bool Valid() const { return ptr != nullptr && shm.Valid(); }
    };
}