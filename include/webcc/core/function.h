#pragma once
#include "allocator.h"
#include "utility.h"
#include "new.h"

namespace webcc
{
    // Small buffer optimization size - callables up to this size are stored inline
    constexpr size_t SBO_SIZE = 32;

    template <typename>
    class function; // Primary template (undefined)

    // Specialization for function types
    template <typename R, typename... Args>
    class function<R(Args...)>
    {
    private:
        // Type-erased callable storage
        struct callable_base
        {
            virtual ~callable_base() = default;
            virtual R invoke(Args... args) = 0;
            virtual callable_base* clone(void* storage, bool use_sbo) const = 0;
            virtual size_t size() const = 0;
        };

        template <typename F>
        struct callable_impl : callable_base
        {
            F func;

            template <typename Fn>
            callable_impl(Fn&& f) : func(webcc::forward<Fn>(f)) {}

            R invoke(Args... args) override
            {
                return func(webcc::forward<Args>(args)...);
            }

            callable_base* clone(void* storage, bool use_sbo) const override
            {
                if (use_sbo)
                {
                    return new (storage) callable_impl(func);
                }
                else
                {
                    void* mem = webcc::malloc(sizeof(callable_impl));
                    return new (mem) callable_impl(func);
                }
            }

            size_t size() const override { return sizeof(callable_impl); }
        };

        // Small buffer for SBO (Small Buffer Optimization)
        // We align to 16 bytes because WASM SIMD types (v128) require 16-byte alignment.
        // Even though WASM is 32-bit (pointers are 4 bytes), we must respect the
        // alignment requirements of captured types to avoid performance penalties or faults.
        alignas(16) char m_storage[SBO_SIZE];
        callable_base* m_callable = nullptr;
        bool m_uses_sbo = false;

        void destroy()
        {
            if (m_callable)
            {
                m_callable->~callable_base();
                if (!m_uses_sbo)
                {
                    webcc::free(m_callable);
                }
                m_callable = nullptr;
                m_uses_sbo = false;
            }
        }

    public:
        function() = default;

        function(decltype(nullptr)) : m_callable(nullptr), m_uses_sbo(false) {}

        // Constructor from callable (functions, lambdas, functors). Not from a function itself,
        // however it's qualified: a const rvalue (a const member of a closure being moved) would
        // otherwise pick this over the copy constructor and wrap itself without end
        template <typename F>
            requires (!__is_same(typename remove_reference<F>::type, function) &&
                      !__is_same(typename remove_reference<F>::type, const function))
        function(F&& f)
        {
            using Impl = callable_impl<typename remove_reference<F>::type>;

            if constexpr (sizeof(Impl) <= SBO_SIZE)
            {
                m_callable = new (m_storage) Impl(webcc::forward<F>(f));
                m_uses_sbo = true;
            }
            else
            {
                void* mem = webcc::malloc(sizeof(Impl));
                m_callable = new (mem) Impl(webcc::forward<F>(f));
                m_uses_sbo = false;
            }
        }

        // Copy constructor
        function(const function& other)
        {
            if (other.m_callable)
            {
                if (other.m_uses_sbo)
                {
                    m_callable = other.m_callable->clone(m_storage, true);
                    m_uses_sbo = true;
                }
                else
                {
                    m_callable = other.m_callable->clone(nullptr, false);
                    m_uses_sbo = false;
                }
            }
        }

        // Move constructor
        function(function&& other)
        {
            if (other.m_uses_sbo)
            {
                // Need to clone into our storage
                m_callable = other.m_callable->clone(m_storage, true);
                m_uses_sbo = true;
                other.destroy();
            }
            else
            {
                m_callable = other.m_callable;
                m_uses_sbo = false;
                other.m_callable = nullptr;
            }
        }

        ~function()
        {
            destroy();
        }

        // Copy assignment
        function& operator=(const function& other)
        {
            if (this != &other)
            {
                destroy();
                if (other.m_callable)
                {
                    if (other.m_uses_sbo)
                    {
                        m_callable = other.m_callable->clone(m_storage, true);
                        m_uses_sbo = true;
                    }
                    else
                    {
                        m_callable = other.m_callable->clone(nullptr, false);
                        m_uses_sbo = false;
                    }
                }
            }
            return *this;
        }

        // Move assignment
        function& operator=(function&& other)
        {
            if (this != &other)
            {
                destroy();
                if (other.m_uses_sbo)
                {
                    m_callable = other.m_callable->clone(m_storage, true);
                    m_uses_sbo = true;
                    other.destroy();
                }
                else
                {
                    m_callable = other.m_callable;
                    m_uses_sbo = false;
                    other.m_callable = nullptr;
                }
            }
            return *this;
        }

        // Assign nullptr
        function& operator=(decltype(nullptr))
        {
            destroy();
            return *this;
        }

        // Invoke
        R operator()(Args... args) const
        {
            if (m_callable)
            {
                return m_callable->invoke(webcc::forward<Args>(args)...);
            }
            // Undefined behavior if called on empty function
            // In a real implementation, this would throw std::bad_function_call
            return R();
        }

        // Check if callable
        explicit operator bool() const { return m_callable != nullptr; }

        // Swap
        void swap(function& other)
        {
            function temp(webcc::move(*this));
            *this = webcc::move(other);
            other = webcc::move(temp);
        }
    };

    // Non-member swap
    template <typename R, typename... Args>
    void swap(function<R(Args...)>& a, function<R(Args...)>& b)
    {
        a.swap(b);
    }

} // namespace webcc
