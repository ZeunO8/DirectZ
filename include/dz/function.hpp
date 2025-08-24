#pragma once
#include <stdexcept>
#include <cassert>

namespace dz
{
    template <typename>
    struct function;

    template <typename TReturn, typename... Args>
    struct function<TReturn(Args...)>
    {
    private:
        using TOAFunction = TReturn (*)(function<TReturn(Args...)> &, Args...);

        TOAFunction bound_a = 0;

        static constexpr size_t SBO_SIZE = 32;
        alignas(void*) char storage[SBO_SIZE] = {};
        short storage_used = 0;

    public:
        function() = default;

        function(const function& o):
            bound_a(o.bound_a),
            storage_used(o.storage_used)
        {
            memcpy(storage, o.storage, storage_used);
        }

        template <typename TOFunction>
        function(TOFunction f)
        {
            struct adapter
            {
                const TOFunction f;

                adapter(const TOFunction &f) : f(f) {}

                static TReturn invoke(function &self, Args... args)
                {
                    auto &a = *reinterpret_cast<adapter *>(self.storage);
                    return (a.f)(args...);
                }
            };

            storage_used = sizeof(adapter);
            assert(storage_used <= SBO_SIZE);
            memcpy(storage, &f, storage_used);

            bound_a = &adapter::invoke;
        }

        template <typename TObject, typename TOFunction>
        function(TObject *o, TOFunction f)
        {
            struct adapter
            {
                TObject *o;
                TOFunction f;
                adapter(TObject *o, TOFunction f) : o(o),
                                                    f(f)
                {
                }

                static TReturn invoke(function &self, Args... args)
                {
                    auto &a = *reinterpret_cast<adapter *>(self.storage);
                    return ((a.o)->*(a.f))(args...);
                }
            };

            storage_used = sizeof(adapter);
            assert(storage_used <= SBO_SIZE);
            memcpy(storage, &o, sizeof(TObject*));
            int offset = sizeof(TObject*);
            memcpy(storage + offset, &f, sizeof(TOFunction));

            bound_a = &adapter::invoke;
        }

        TReturn operator()(Args... args)
        {
            if (bound_a)
                return bound_a(*this, args...);
            else if constexpr (requires { TReturn(0); })
                return TReturn(0);
            else if constexpr (requires { TReturn(); })
                return TReturn();
            else if constexpr (requires { TReturn{}; })
                return TReturn{};
            else if constexpr (std::is_same_v<TReturn, void>)
                return;
            else
            {
                static_assert(false && "No path for TReturn");
                throw std::runtime_error("No path for TReturn");
            }
        }

        function& operator=(const function& o)
        {
            bound_a = o.bound_a;
            storage_used = o.storage_used;
            memcpy(storage, o.storage, storage_used);
            return *this;
        }

        bool operator==(const function& o)
        {
            return bound_a == o.bound_a;
        }

        bool operator!=(const function& o)
        {
            return bound_a != o.bound_a;
        }

		constexpr explicit operator bool() const noexcept
		{
			return bound_a;
		};

		constexpr bool operator!() const noexcept
		{
			return !bound_a;
		};
    };
}