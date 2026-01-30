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

        // Private: take ownership of an existing buffer (used by concat)
        struct take_ownership_t {};
        string(char* data, uint32_t len, take_ownership_t) : m_data(data), m_len(len) {}

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
            // Hybrid formatter: stack (1024) first, heap if overflow
            webcc::hybrid_formatter<1024> buf;
            if constexpr (sizeof...(Args) > 0) {
                (buf << ... << args);
            }
            
            // If on heap, take ownership directly (no copy)
            if (buf.on_heap()) {
                uint32_t len = buf.length();
                char* data = buf.release();
                return string(data, len, take_ownership_t{});
            }
            
            // Otherwise copy from stack buffer
            return string(buf.c_str());
        }

        const char *c_str() const { return m_data ? m_data : ""; }
        const char *data() const { return m_data; }
        char *data() { return m_data; }
        
        uint32_t length() const { return m_len; }
        uint32_t size() const { return m_len; }
        bool empty() const { return m_len == 0; }

        // Get character at index as a single-character string
        string at(uint32_t index) const {
            if (index >= m_len) return string();
            return string(m_data + index, 1);
        }

        // Get substring (standard library compatible)
        // substr(pos, len) - get substring starting at pos with length len
        string substr(uint32_t pos, uint32_t len = 0xFFFFFFFF) const {
            if (pos >= m_len) return string();
            uint32_t actual_len = (len > m_len - pos) ? (m_len - pos) : len;
            return string(m_data + pos, actual_len);
        }

        // Check if string contains substring (C++23 standard)
        bool contains(const string& needle) const {
            if (needle.m_len == 0) return true;
            if (needle.m_len > m_len) return false;
            for (uint32_t i = 0; i <= m_len - needle.m_len; i++) {
                bool match = true;
                for (uint32_t j = 0; j < needle.m_len && match; j++) {
                    if (m_data[i + j] != needle.m_data[j]) match = false;
                }
                if (match) return true;
            }
            return false;
        }

        bool contains(const char* needle) const {
            return contains(string(needle));
        }

        // Trim whitespace from start
        string trim_start() const {
            if (m_len == 0) return string();
            uint32_t start = 0;
            while (start < m_len && (m_data[start] == ' ' || m_data[start] == '\t' || 
                                      m_data[start] == '\n' || m_data[start] == '\r')) {
                start++;
            }
            if (start == m_len) return string();
            return string(m_data + start, m_len - start);
        }

        // Trim whitespace from end
        string trim_end() const {
            if (m_len == 0) return string();
            uint32_t end = m_len;
            while (end > 0 && (m_data[end - 1] == ' ' || m_data[end - 1] == '\t' || 
                               m_data[end - 1] == '\n' || m_data[end - 1] == '\r')) {
                end--;
            }
            if (end == 0) return string();
            return string(m_data, end);
        }

        // Trim whitespace from both ends
        string trim() const {
            return trim_start().trim_end();
        }

        // Parse string as integer
        int to_int() const {
            if (m_len == 0 || !m_data) return 0;
            int result = 0;
            uint32_t i = 0;
            bool negative = false;
            // Skip leading whitespace
            while (i < m_len && (m_data[i] == ' ' || m_data[i] == '\t')) i++;
            // Handle sign
            if (i < m_len && m_data[i] == '-') { negative = true; i++; }
            else if (i < m_len && m_data[i] == '+') { i++; }
            // Parse digits
            while (i < m_len && m_data[i] >= '0' && m_data[i] <= '9') {
                result = result * 10 + (m_data[i] - '0');
                i++;
            }
            return negative ? -result : result;
        }

        // Parse string as float (stops at second decimal point or non-digit)
        double to_float() const {
            if (m_len == 0 || !m_data) return 0.0;
            double result = 0.0;
            double fraction = 0.0;
            double divisor = 1.0;
            uint32_t i = 0;
            bool negative = false;
            bool in_fraction = false;
            // Skip leading whitespace
            while (i < m_len && (m_data[i] == ' ' || m_data[i] == '\t')) i++;
            // Handle sign
            if (i < m_len && m_data[i] == '-') { negative = true; i++; }
            else if (i < m_len && m_data[i] == '+') { i++; }
            // Parse digits (stop at second decimal point)
            while (i < m_len) {
                if (m_data[i] == '.') {
                    if (in_fraction) break;  // Second decimal point - stop parsing
                    in_fraction = true;
                    i++;
                    continue;
                }
                if (m_data[i] < '0' || m_data[i] > '9') break;
                if (in_fraction) {
                    divisor *= 10.0;
                    fraction += (m_data[i] - '0') / divisor;
                } else {
                    result = result * 10.0 + (m_data[i] - '0');
                }
                i++;
            }
            result += fraction;
            return negative ? -result : result;
        }

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

        // String concatenation
        string operator+(const string& other) const {
            return concat(*this, other);
        }
        string operator+(const char* other) const {
            return concat(*this, other);
        }
        string operator+(int val) const {
            formatter<32> fmt;
            fmt << val;
            return concat(*this, fmt.c_str());
        }
        string operator+(float val) const {
            formatter<32> fmt;
            fmt << val;
            return concat(*this, fmt.c_str());
        }
        string operator+(double val) const {
            formatter<32> fmt;
            fmt << val;
            return concat(*this, fmt.c_str());
        }
        string& operator+=(const string& other) {
            *this = concat(*this, other);
            return *this;
        }
        string& operator+=(const char* other) {
            *this = concat(*this, other);
            return *this;
        }
        string& operator+=(char c) {
            char buf[2] = {c, '\0'};
            *this = concat(*this, buf);
            return *this;
        }
        string& operator+=(int val) {
            formatter<32> fmt;
            fmt << val;
            *this = concat(*this, fmt.c_str());
            return *this;
        }
        string& operator+=(float val) {
            formatter<32> fmt;
            fmt << val;
            *this = concat(*this, fmt.c_str());
            return *this;
        }
        string& operator+=(double val) {
            formatter<32> fmt;
            fmt << val;
            *this = concat(*this, fmt.c_str());
            return *this;
        }

        // Lexicographic comparison (for character range checks)
        bool operator<(const string& other) const {
            uint32_t min_len = m_len < other.m_len ? m_len : other.m_len;
            for (uint32_t i = 0; i < min_len; i++) {
                if (m_data[i] < other.m_data[i]) return true;
                if (m_data[i] > other.m_data[i]) return false;
            }
            return m_len < other.m_len;
        }
        bool operator<(const char* other) const {
            return *this < string(other);
        }
        bool operator>(const string& other) const {
            return other < *this;
        }
        bool operator>(const char* other) const {
            return *this > string(other);
        }
        bool operator<=(const string& other) const {
            return !(other < *this);
        }
        bool operator<=(const char* other) const {
            return *this <= string(other);
        }
        bool operator>=(const string& other) const {
            return !(*this < other);
        }
        bool operator>=(const char* other) const {
            return *this >= string(other);
        }
    };
} // namespace webcc
