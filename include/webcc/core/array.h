#pragma once
#include "algorithm.h" // for sort
#include <stdint.h>
#include <stddef.h>

namespace webcc
{
    // Aggregate type - supports brace initialization: array<int, 3> a = {1, 2, 3};
    template <typename T, size_t N>
    struct array
    {
        T m_data[N];  // Public for aggregate initialization

        // Accessors
        constexpr T *data() { return m_data; }
        constexpr const T *data() const { return m_data; }
        constexpr size_t size() const { return N; }
        constexpr bool empty() const { return N == 0; }

        // Find index of first occurrence of value, returns -1 if not found
        int index_of(const T& value) const
        {
            for (size_t i = 0; i < N; ++i)
            {
                if (m_data[i] == value)
                    return static_cast<int>(i);
            }
            return -1;
        }

        // Check if array contains value
        bool contains(const T& value) const
        {
            return index_of(value) >= 0;
        }

        // Fill entire array with value
        void fill(const T& value)
        {
            for (size_t i = 0; i < N; ++i)
            {
                m_data[i] = value;
            }
        }

        // Sort elements in ascending order
        void sort()
        {
            webcc::sort(begin(), end());
        }

        // Bounds checking (Crucial for WASM debugging)
        T &operator[](size_t i)
        {
            return m_data[i];
        }

        const T &operator[](size_t i) const
        {
            return m_data[i];
        }

        // Iterator support for: for(auto& x : my_array)
        T *begin() { return &m_data[0]; }
        T *end() { return &m_data[N]; }

        const T *begin() const { return &m_data[0]; }
        const T *end() const { return &m_data[N]; }
    };
} // namespace webcc