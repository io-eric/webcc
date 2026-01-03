#pragma once
#include "vector.h"
#include "utility.h"

namespace webcc
{
    // Efficient circular buffer queue implementation
    // Optimized for WebAssembly with minimal allocations and cache-friendly access
    template <typename T>
    class queue
    {
    private:
        T* m_data = nullptr;
        size_t m_capacity = 0;
        size_t m_head = 0;      // Index of first element
        size_t m_tail = 0;      // Index where next element will be inserted
        size_t m_size = 0;

        void reallocate(size_t new_capacity)
        {
            T* new_data = (T*)webcc::malloc(new_capacity * sizeof(T));
            
            // Copy elements in order from head to tail
            if (m_data && m_size > 0)
            {
                size_t i = 0;
                try
                {
                    for (; i < m_size; ++i)
                    {
                        size_t old_idx = (m_head + i) & (m_capacity - 1);
                        new(&new_data[i]) T(webcc::move(m_data[old_idx]));
                    }
                }
                catch (...)
                {
                    for (size_t j = 0; j < i; ++j)
                    {
                        new_data[j].~T();
                    }
                    webcc::free(new_data);
                    throw;
                }

                for (size_t k = 0; k < m_size; ++k)
                {
                    size_t old_idx = (m_head + k) & (m_capacity - 1);
                    m_data[old_idx].~T();
                }
                webcc::free(m_data);
            }
            
            m_data = new_data;
            m_capacity = new_capacity;
            m_head = 0;
            m_tail = m_size;
        }

    public:
        queue() = default;

        ~queue()
        {
            clear();
            if (m_data) webcc::free(m_data);
        }

        // Copy constructor
        queue(const queue& other)
        {
            if (other.m_size > 0)
            {
                m_capacity = other.m_capacity;
                m_data = (T*)webcc::malloc(m_capacity * sizeof(T));
                m_size = other.m_size;
                
                for (size_t i = 0; i < m_size; ++i)
                {
                    size_t idx = (other.m_head + i) & (other.m_capacity - 1);
                    new(&m_data[i]) T(other.m_data[idx]);
                }
                
                m_head = 0;
                m_tail = m_size;
            }
        }

        // Move constructor
        queue(queue&& other) noexcept
            : m_data(other.m_data), m_capacity(other.m_capacity),
              m_head(other.m_head), m_tail(other.m_tail), m_size(other.m_size)
        {
            other.m_data = nullptr;
            other.m_capacity = 0;
            other.m_head = 0;
            other.m_tail = 0;
            other.m_size = 0;
        }

        // Copy assignment
        queue& operator=(const queue& other)
        {
            if (this != &other)
            {
                clear();
                if (m_data) webcc::free(m_data);
                
                m_data = nullptr;
                m_capacity = 0;
                m_head = 0;
                m_tail = 0;
                m_size = 0;
                
                if (other.m_size > 0)
                {
                    m_capacity = other.m_capacity;
                    m_data = (T*)webcc::malloc(m_capacity * sizeof(T));
                    m_size = other.m_size;
                    
                    for (size_t i = 0; i < m_size; ++i)
                    {
                        size_t idx = (other.m_head + i) & (other.m_capacity - 1);
                        new(&m_data[i]) T(other.m_data[idx]);
                    }
                    
                    m_head = 0;
                    m_tail = m_size;
                }
            }
            return *this;
        }

        // Move assignment
        queue& operator=(queue&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                if (m_data) webcc::free(m_data);
                
                m_data = other.m_data;
                m_capacity = other.m_capacity;
                m_head = other.m_head;
                m_tail = other.m_tail;
                m_size = other.m_size;
                
                other.m_data = nullptr;
                other.m_capacity = 0;
                other.m_head = 0;
                other.m_tail = 0;
                other.m_size = 0;
            }
            return *this;
        }

        void push(const T& value)
        {
            if (m_size == m_capacity)
            {
                size_t new_cap = m_capacity == 0 ? 8 : m_capacity * 2;
                reallocate(new_cap);
            }
            
            new(&m_data[m_tail]) T(value);
            m_tail = (m_tail + 1) & (m_capacity - 1);
            ++m_size;
        }

        void push(T&& value)
        {
            if (m_size == m_capacity)
            {
                size_t new_cap = m_capacity == 0 ? 8 : m_capacity * 2;
                reallocate(new_cap);
            }
            
            new(&m_data[m_tail]) T(webcc::move(value));
            m_tail = (m_tail + 1) & (m_capacity - 1);
            ++m_size;
        }

        template<typename... Args>
        void emplace(Args&&... args)
        {
            if (m_size == m_capacity)
            {
                size_t new_cap = m_capacity == 0 ? 8 : m_capacity * 2;
                reallocate(new_cap);
            }
            
            new(&m_data[m_tail]) T(webcc::forward<Args>(args)...);
            m_tail = (m_tail + 1) & (m_capacity - 1);
            ++m_size;
        }

        void pop()
        {
            if (m_size > 0)
            {
                m_data[m_head].~T();
                m_head = (m_head + 1) & (m_capacity - 1);
                --m_size;
            }
        }

        T& front() { return m_data[m_head]; }
        const T& front() const { return m_data[m_head]; }

        T& back() 
        { 
            if (m_capacity == 0) return *m_data;
            size_t back_idx = (m_tail == 0) ? m_capacity - 1 : m_tail - 1;
            return m_data[back_idx]; 
        }
        
        const T& back() const 
        { 
            if (m_capacity == 0) return *m_data;
            size_t back_idx = (m_tail == 0) ? m_capacity - 1 : m_tail - 1;
            return m_data[back_idx]; 
        }

        size_t size() const { return m_size; }
        bool empty() const { return m_size == 0; }

        void clear()
        {
            while (m_size > 0)
            {
                m_data[m_head].~T();
                m_head = (m_head + 1) & (m_capacity - 1);
                --m_size;
            }
            m_head = 0;
            m_tail = 0;
        }
    };
} // namespace webcc
