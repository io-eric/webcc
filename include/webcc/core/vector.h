#pragma once
#include "allocator.h"
#include "new.h"
#include "utility.h"
#include "algorithm.h" // for sort
#include <stdint.h>
#include <stddef.h>

namespace webcc
{
    template <typename T>
    class vector
    {
    private:
        T *m_data = nullptr;
        size_t m_size = 0;
        size_t m_capacity = 0;

        void reallocate(size_t new_capacity)
        {
            // 1. Allocate new block
            T *new_block = (T *)webcc::malloc(new_capacity * sizeof(T));
            if (!new_block)
                return; // Allocation failed

            // 2. Move existing elements
            if (m_data)
            {
                for (size_t i = 0; i < m_size; ++i)
                {
                    new (new_block + i) T(webcc::move(m_data[i]));
                    m_data[i].~T();
                }
                webcc::free(m_data);
            }

            m_data = new_block;
            m_capacity = new_capacity;
        }

    public:
        using iterator = T *;
        using const_iterator = const T *;

        vector() = default;

        // Variadic constructor for brace init {a,b,c} - works in freestanding/WASM
        template<typename U, typename... Args>
        vector(U&& first, Args&&... rest)
        {
            reserve(1 + sizeof...(rest));
            push_back(static_cast<T>(first));
            (push_back(static_cast<T>(rest)), ...);
        }

        explicit vector(size_t count)
        {
            resize(count);
        }

        ~vector()
        {
            clear();
            webcc::free(m_data);
        }

        // Copy constructor
        vector(const vector &other)
        {
            if (other.m_size > 0)
            {
                reserve(other.m_size);
                size_t limit = (m_capacity < other.m_size) ? m_capacity : other.m_size;
                for (size_t i = 0; i < limit; ++i)
                {
                    new (m_data + i) T(other.m_data[i]);
                }
                m_size = limit;
            }
        }

        // Copy assignment
        vector &operator=(const vector &other)
        {
            if (this != &other)
            {
                clear();
                reserve(other.m_size);
                size_t limit = (m_capacity < other.m_size) ? m_capacity : other.m_size;
                for (size_t i = 0; i < limit; ++i)
                {
                    new (m_data + i) T(other.m_data[i]);
                }
                m_size = limit;
            }
            return *this;
        }

        // Move constructor
        vector(vector &&other) noexcept
            : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity)
        {
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        // Move assignment
        vector &operator=(vector &&other) noexcept
        {
            if (this != &other)
            {
                clear();
                webcc::free(m_data);

                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;

                other.m_data = nullptr;
                other.m_size = 0;
                other.m_capacity = 0;
            }
            return *this;
        }

        void push_back(const T &value)
        {
            if (m_size == m_capacity)
            {
                size_t new_cap = m_capacity == 0 ? 4 : m_capacity * 2;
                reallocate(new_cap);
            }
            if (m_size < m_capacity)
            {
                new (m_data + m_size) T(value);
                m_size++;
            }
        }

        void push_back(T &&value)
        {
            if (m_size == m_capacity)
            {
                size_t new_cap = m_capacity == 0 ? 4 : m_capacity * 2;
                reallocate(new_cap);
            }
            if (m_size < m_capacity)
            {
                new (m_data + m_size) T(webcc::move(value));
                m_size++;
            }
        }

        template <typename... Args>
        void emplace_back(Args &&...args)
        {
            if (m_size == m_capacity)
            {
                size_t new_cap = m_capacity == 0 ? 4 : m_capacity * 2;
                reallocate(new_cap);
            }
            if (m_size < m_capacity)
            {
                new (m_data + m_size) T(webcc::forward<Args>(args)...);
                m_size++;
            }
        }

        void pop_back()
        {
            if (m_size > 0)
            {
                m_size--;
                m_data[m_size].~T();
            }
        }

        // Erase element at index, shifting remaining elements
        void erase(size_t index)
        {
            if (index >= m_size)
                return;
            m_data[index].~T();
            for (size_t i = index; i < m_size - 1; ++i)
            {
                new (m_data + i) T(webcc::move(m_data[i + 1]));
                m_data[i + 1].~T();
            }
            m_size--;
        }

        void reserve(size_t new_capacity)
        {
            if (new_capacity > m_capacity)
            {
                reallocate(new_capacity);
            }
        }

        void resize(size_t new_size)
        {
            if (new_size > m_size)
            {
                reserve(new_size);
                size_t limit = (m_capacity < new_size) ? m_capacity : new_size;
                for (size_t i = m_size; i < limit; ++i)
                {
                    new (m_data + i) T();
                }
                m_size = limit;
            }
            else
            {
                for (size_t i = new_size; i < m_size; ++i)
                {
                    m_data[i].~T();
                }
                m_size = new_size;
            }
        }

        void clear()
        {
            for (size_t i = 0; i < m_size; ++i)
            {
                m_data[i].~T();
            }
            m_size = 0;
        }

        T &operator[](size_t index) { return m_data[index]; }
        const T &operator[](size_t index) const { return m_data[index]; }

        T *data() { return m_data; }
        const T *data() const { return m_data; }

        size_t size() const { return m_size; }
        size_t capacity() const { return m_capacity; }
        bool empty() const { return m_size == 0; }

        // Find index of first occurrence of value, returns -1 if not found
        int index_of(const T& value) const
        {
            for (size_t i = 0; i < m_size; ++i)
            {
                if (m_data[i] == value)
                    return static_cast<int>(i);
            }
            return -1;
        }

        // Check if vector contains value
        bool contains(const T& value) const
        {
            return index_of(value) >= 0;
        }

        // Remove element at index (alias for erase for consistency with Coi naming)
        void remove(size_t index)
        {
            erase(index);
        }

        // Sort elements in ascending order
        void sort()
        {
            webcc::sort(begin(), end());
        }

        iterator begin() { return m_data; }
        iterator end() { return m_data + m_size; }
        const_iterator begin() const { return m_data; }
        const_iterator end() const { return m_data + m_size; }
    };
} // namespace webcc
