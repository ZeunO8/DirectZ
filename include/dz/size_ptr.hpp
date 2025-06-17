#pragma once
#include <cstddef>
#include <cstdlib>
#include <functional>

namespace dz
{
    struct default_noop
    {
        static void call(void* p) {}
    };

    struct default_delete_single
    {
        template <typename T>
        static void call(void* p) { delete static_cast<T*>(p); }
    };

    struct default_delete_array
    {
        template <typename T>
        static void call(void* p) { delete[] static_cast<T*>(p); }
    };

    struct default_free_deleter
    {
        static void call(void* p) { free(p); }
    };

    template <typename T>
    struct size_ptr
    {
        T* ptr = nullptr;
        size_t* size = nullptr;
        size_t* ref_c = nullptr;
        void(*deleter)(void*) = nullptr;

        size_ptr() = default;

        size_ptr(T* ptr, size_t size = 1, void(*deleter)(void*) = &default_delete_single::call<T>) :
            ptr(ptr),
            size(new size_t(size)),
            ref_c(new size_t(1)),
            deleter(deleter)
        {
        }

        size_ptr(const size_ptr& other) :
            ptr(other.ptr),
            size(other.size),
            ref_c(other.ref_c),
            deleter(other.deleter)
        {
            (*ref_c)++;
        }

        size_ptr& operator=(const size_ptr& other)
        {
            if (this != &other)
            {
                reset();
                ptr = other.ptr;
                size = other.size;
                ref_c = other.ref_c;
                deleter = other.deleter;
                (*ref_c)++;
            }
            return *this;
        }

        ~size_ptr()
        {
            reset();
        }

        void reset()
        {
            if (ref_c)
            {
                (*ref_c)--;
                if (!(*ref_c))
                {
                    if (deleter) deleter(static_cast<void*>(ptr));
                    delete size;
                    delete ref_c;
                }
            }
            ptr = nullptr;
            size = nullptr;
            ref_c = nullptr;
            deleter = nullptr;
        }

        T* operator->() { return ptr; }

        T& operator*() { return *ptr; }

        const T* get() const { return ptr; }

        size_t get_size() const { return size ? *size : 0; }
    };
}
