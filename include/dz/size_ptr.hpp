/**
 * @file size_ptr.hpp
 * @brief Defines a reference-counted smart pointer with size tracking and custom deleter support.
 */
#pragma once

#include <cstddef>
#include <cstdlib>
#include <functional>

namespace dz
{
    /**
     * @brief Default no-op deleter.
     */
    struct default_noop
    {
        static void call(void* p) {}
    };

    /**
     * @brief Default deleter for single heap-allocated objects.
     */
    struct default_delete_single
    {
        template <typename T>
        static void call(void* p) { delete static_cast<T*>(p); }
    };

    /**
     * @brief Default deleter for arrays.
     */
    struct default_delete_array
    {
        template <typename T>
        static void call(void* p) { delete[] static_cast<T*>(p); }
    };

    /**
     * @brief Default deleter using `free()`.
     */
    struct default_free_deleter
    {
        static void call(void* p) { free(p); }
    };

    /**
     * @brief A reference-counted smart pointer with associated size and custom deleter.
     *
     * @tparam T The type of the object being pointed to.
     */
    template <typename T>
    struct size_ptr
    {
        T* ptr = nullptr;                   /**< Raw pointer to the data. */
        size_t* size = nullptr;             /**< Shared pointer to the size of the data. */
        size_t* ref_c = nullptr;            /**< Reference count shared among copies. */
        void(*deleter)(void*) = nullptr;    /**< Function pointer to the deleter function. */

        /**
         * @brief Default constructor.
         */
        size_ptr() = default;

        /**
         * @brief Construct with data pointer, size, and deleter.
         *
         * @param ptr Raw pointer to the data.
         * @param size Number of elements.
         * @param deleter Deleter function to call on destruction.
         */
        size_ptr(T* ptr, size_t size = 1, void(*deleter)(void*) = &default_delete_single::call<T>) :
            ptr(ptr),
            size(new size_t(size)),
            ref_c(new size_t(1)),
            deleter(deleter)
        {
        }

        /**
         * @brief Copy constructor (increments reference count).
         *
         * @param other Another size_ptr to copy from.
         */
        size_ptr(const size_ptr& other) :
            ptr(other.ptr),
            size(other.size),
            ref_c(other.ref_c),
            deleter(other.deleter)
        {
            (*ref_c)++;
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other Another size_ptr to assign from.
         * @return Reference to this.
         */
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

        /**
         * @brief Destructor. Decrements reference count and frees if zero.
         */
        ~size_ptr()
        {
            reset();
        }

        /**
         * @brief Reset the smart pointer, decrementing the reference count and deleting resources if needed.
         */
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

        /**
         * @brief Pointer access operator.
         *
         * @return Pointer to the data.
         */
        T* operator->() { return ptr; }

        /**
         * @brief Dereference operator.
         *
         * @return Reference to the data.
         */
        T& operator*() { return *ptr; }

        /**
         * @brief Gets the raw pointer.
         *
         * @return Const pointer to the data.
         */
        const T* get() const { return ptr; }

        /**
         * @brief Gets the number of elements pointed to.
         *
         * @return Size in elements.
         */
        size_t get_size() const { return size ? *size : 0; }
    };
} // namespace dz
