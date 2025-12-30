#pragma once
#include "allocator.h"
#include "string_view.h"
#include "format.h"

namespace webcc
{
    class string
    {
    private:
        char *m_data = nullptr;
        uint32_t m_len = 0;

        static uint32_t strlen(const char *s)
        {
            uint32_t l = 0;
            if (!s) return 0;
            while (s[l])
                l++;
            return l;
        }

    public:
        using iterator = char*;
        using const_iterator = const char*;

        string() : m_data(nullptr), m_len(0) {}

        string(const char *s)
        {
            if (s) {
                m_len = strlen(s);
                m_data = (char *)webcc::malloc(m_len + 1);
                for (uint32_t i = 0; i < m_len; i++)
                    m_data[i] = s[i];
                m_data[m_len] = '\0';
            } else {
                m_data = nullptr;
                m_len = 0;
            }
        }

        string(const char *s, uint32_t len)
        {
            m_len = len;
            if (len > 0) {
                m_data = (char *)webcc::malloc(m_len + 1);
                for (uint32_t i = 0; i < m_len; i++)
                    m_data[i] = s[i];
                m_data[m_len] = '\0';
            } else {
                m_data = nullptr;
            }
        }

        string(const string_view& sv) : string(sv.data(), sv.length()) {}

        // Copy constructor
        string(const string& other)
        {
            m_len = other.m_len;
            if (other.m_data) {
                m_data = (char *)webcc::malloc(m_len + 1);
                for (uint32_t i = 0; i <= m_len; i++)
                    m_data[i] = other.m_data[i];
            } else {
                m_data = nullptr;
            }
        }

        // Copy assignment
        string& operator=(const string& other)
        {
            if (this != &other)
            {
                webcc::free(m_data);
                m_len = other.m_len;
                if (other.m_data) {
                    m_data = (char *)webcc::malloc(m_len + 1);
                    for (uint32_t i = 0; i <= m_len; i++)
                        m_data[i] = other.m_data[i];
                } else {
                    m_data = nullptr;
                }
            }
            return *this;
        }

        // Move constructor (Zero-cost transfer of ownership)
        string(string &&other) noexcept
        {
            m_data = other.m_data;
            m_len = other.m_len;
            other.m_data = nullptr;
            other.m_len = 0;
        }

        // Move assignment
        string& operator=(string&& other) noexcept
        {
            if (this != &other)
            {
                webcc::free(m_data);
                m_data = other.m_data;
                m_len = other.m_len;
                other.m_data = nullptr;
                other.m_len = 0;
            }
            return *this;
        }

        ~string() { webcc::free(m_data); }

        template <typename... Args>
        static string concat(Args... args)
        {
            // 1. We use a formatter on the stack to build the string first
            // This avoids multiple heap allocations
            webcc::formatter<1024> temp;
            if constexpr (sizeof...(Args) > 0) {
                (temp << ... << args); // C++17 Fold Expression
            }

            // 2. Now we allocate the exact size needed once
            return string(temp.c_str());
        }

        const char *c_str() const { return m_data ? m_data : ""; }
        const char *data() const { return m_data; }
        char *data() { return m_data; }
        
        uint32_t length() const { return m_len; }
        uint32_t size() const { return m_len; }
        bool empty() const { return m_len == 0; }

        iterator begin() { return m_data; }
        iterator end() { return m_data + m_len; }
        const_iterator begin() const { return m_data; }
        const_iterator end() const { return m_data + m_len; }

        operator string_view() const { return string_view(m_data, m_len); }
        
        char& operator[](uint32_t i) { return m_data[i]; }
        const char& operator[](uint32_t i) const { return m_data[i]; }

        bool operator==(const string& other) const {
            return string_view(*this) == string_view(other);
        }
        bool operator!=(const string& other) const {
            return !(*this == other);
        }
        bool operator==(const char* other) const {
            return string_view(*this) == string_view(other);
        }
        bool operator!=(const char* other) const {
            return !(*this == other);
        }
    };
} // namespace webcc
