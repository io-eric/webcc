#pragma once
#include "allocator.h"
#include "new.h"
#include "utility.h"

namespace webcc
{
    template<typename T>
    class unique_ptr
    {
    private:
        T* m_ptr = nullptr;

    public:
        unique_ptr() : m_ptr(nullptr) {}
        explicit unique_ptr(T* p) : m_ptr(p) {}

        ~unique_ptr()
        {
            reset();
        }

        // No copy
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;

        // Move
        unique_ptr(unique_ptr&& other) : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        unique_ptr& operator=(unique_ptr&& other)
        {
            if (this != &other)
            {
                reset(other.release());
            }
            return *this;
        }

        T* get() const { return m_ptr; }
        T* operator->() const { return m_ptr; }
        T& operator*() const { return *m_ptr; }

        T* release()
        {
            T* p = m_ptr;
            m_ptr = nullptr;
            return p;
        }

        void reset(T* p = nullptr)
        {
            if (m_ptr)
            {
                m_ptr->~T();
                webcc::free(m_ptr);
            }
            m_ptr = p;
        }

        explicit operator bool() const { return m_ptr != nullptr; }
    };

    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args)
    {
        void* mem = webcc::malloc(sizeof(T));
        if (!mem) return unique_ptr<T>(nullptr);
        
        // Construct using placement new
        new(mem) T(webcc::forward<Args>(args)...);
        
        return unique_ptr<T>((T*)mem);
    }

} // namespace webcc
