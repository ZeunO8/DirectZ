#pragma once
#include <stdexcept>
#include <cassert>
#include "Arena.hpp"

namespace dz
{
    inline static Arena<16384> function_arena = {};

    template <typename>
    struct function;

    template <typename TReturn, typename... Args>
    struct function<TReturn(Args...)>
    {
    private:
        static constexpr size_t MAX_SBO_SIZE = (sizeof(void*) * 4);
        using TOAFunction = TReturn (*)(function<TReturn(Args...)> &, Args...);
        using DestroyFunction = void (*)(function<TReturn(Args...)> &);

        size_t* ref_count = 0;
        TOAFunction invoke = 0;
        DestroyFunction destroy = 0;
        alignas(void*) char sbo[MAX_SBO_SIZE] = {0};
        bool arena_allocated = false;
        char* storage = nullptr;
        size_t storage_size = 0;
        
        void ensure_storage(size_t adapter_size) {
            if(adapter_size <= MAX_SBO_SIZE) {
                storage = sbo;
            }
            else {
                storage = (char*)function_arena.arena_malloc(adapter_size);
                arena_allocated = true;
            }
            storage_size = adapter_size;
        }

        void reset()
        {
            if (arena_allocated)
            {
                if (*ref_count == 1) {
                    if (destroy)
                        destroy(*this);
                    function_arena.arena_free(storage, storage_size);
                }
            }
            if (ref_count)
            {
                --(*ref_count);
                if (*ref_count == 0)
                    function_arena.arena_free(ref_count, sizeof(size_t));
            }
        }

    public:
        function():
            ref_count((size_t*)function_arena.arena_malloc(sizeof(size_t)))
        {
            *ref_count = 1;
        };

        function(const function& o)
        {
            (*this) = o;
        }

        template <typename TOFunction>
        function(TOFunction f):
            function()
        {
            struct adapter
            {
                TOFunction f;
                adapter(TOFunction f):
                    f(f)
                {}

                static TReturn invoke(function &self, Args... args)
                {
                    auto &a = *reinterpret_cast<adapter *>(self.storage);
                    return (a.f)(args...);
                }

                static void destroy(function &self)
                {
                    auto& a_ptr = (adapter*&)(self.storage);
                    a_ptr->~adapter();
                }
            };

            constexpr auto adapter_size = sizeof(adapter);
            ensure_storage(adapter_size);

            auto& adapter_ptr = (adapter*&)storage;
            ::new (adapter_ptr) adapter(f);

            invoke = &adapter::invoke;
            destroy = &adapter::destroy;
        }

        template <typename TObject, typename TOFunction>
        function(TObject *o, TOFunction f):
            function()
        {
            struct adapter
            {
                TObject *o;
                TOFunction f;

                static TReturn invoke(function &self, Args... args)
                {
                    auto &a = *reinterpret_cast<adapter *>(self.storage);
                    return ((a.o)->*(a.f))(args...);
                }
            };

            constexpr auto adapter_size = sizeof(adapter);
            ensure_storage(adapter_size);

            memcpy(storage, &o, sizeof(TObject*));
            int offset = sizeof(TObject*);
            memcpy(storage + offset, &f, sizeof(TOFunction));

            invoke = &adapter::invoke;
        }

        TReturn operator()(Args... args) const
        {
            if (invoke)
                return invoke((function&)(*this), args...);
            else if constexpr (std::is_same_v<TReturn, void>)
                return;
            else if constexpr (requires { TReturn(0); })
                return TReturn(0);
            else if constexpr (requires { TReturn(); })
                return TReturn();
            else if constexpr (requires { TReturn{}; })
                return TReturn{};
            else
            {
                static_assert(false && "No path for TReturn");
                throw std::runtime_error("No path for TReturn");
            }
        }

        function& operator=(const function& o)
        {
            reset();
            ref_count = o.ref_count;
            *(ref_count)++;
            arena_allocated = o.arena_allocated;
            invoke = o.invoke;
            storage_size = o.storage_size;
            if (arena_allocated)
                storage = o.storage;
            else {
                memcpy(sbo, o.storage, storage_size);
                storage = sbo;
            }
            return *this;
        }

        bool operator==(const function& o)
        {
            return (storage_size == o.storage_size) && (memcmp(storage, o.storage, storage_size));
        }

        bool operator!=(const function& o)
        {
            return !((*this) == o);
        }

		constexpr explicit operator bool() const noexcept
		{
			return invoke;
		}

		constexpr bool operator!() const noexcept
		{
			return !invoke;
		}

        ~function()
        {
            reset();
        }
    };
}