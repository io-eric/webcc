#pragma once
#include <stddef.h>

namespace webcc
{
    // Minimal initializer_list implementation
    template <typename T>
    class initializer_list
    {
    private:
        const T* m_begin;
        size_t m_size;

        // Private constructor used by the compiler
        constexpr initializer_list(const T* begin, size_t size) noexcept
            : m_begin(begin), m_size(size) {}

    public:
        using value_type = T;
        using reference = const T&;
        using const_reference = const T&;
        using size_type = size_t;
        using iterator = const T*;
        using const_iterator = const T*;

        constexpr initializer_list() noexcept : m_begin(nullptr), m_size(0) {}

        constexpr size_t size() const noexcept { return m_size; }
        constexpr const T* begin() const noexcept { return m_begin; }
        constexpr const T* end() const noexcept { return m_begin + m_size; }
    };

    template <typename T>
    constexpr const T* begin(initializer_list<T> il) noexcept { return il.begin(); }

    template <typename T>
    constexpr const T* end(initializer_list<T> il) noexcept { return il.end(); }
}
