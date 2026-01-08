#pragma once
#include <stdint.h>
#include <stddef.h>

namespace webcc {

// --- Formatting Wrappers ---
// These don't do work yet, they just "tag" the data
struct hex {
    unsigned int value;
    explicit hex(unsigned int v) : value(v) {}
};

struct precision {
    float value;
    int places;
    precision(float v, int p = 2) : value(v), places(p) {}
};

// --- The Formatter (Stack-based) ---
template<size_t N>
class formatter {
private:
    char m_data[N];
    size_t m_pos = 0;

public:
    formatter() { m_data[0] = '\0'; }

    void clear() { m_pos = 0; m_data[0] = '\0'; }
    const char* c_str() const { return m_data; }
    size_t length() const { return m_pos; }

    // Overload for Strings / string_view
    formatter& operator<<(const char* s) {
        while (*s && m_pos < N - 1) m_data[m_pos++] = *s++;
        m_data[m_pos] = '\0';
        return *this;
    }

    formatter& operator<<(string_view sv) {
        for (size_t i = 0; i < sv.length() && m_pos < N - 1; ++i) {
            m_data[m_pos++] = sv[i];
        }
        m_data[m_pos] = '\0';
        return *this;
    }

    // Overload for Integers
    formatter& operator<<(int val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            if (m_pos < N - 1) m_data[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned int)val;
    }

    formatter& operator<<(unsigned int val) {
        if (val == 0) return *this << "0";
        char temp[12];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        while (i > 0 && m_pos < N - 1) m_data[m_pos++] = temp[--i];
        m_data[m_pos] = '\0';
        return *this;
    }

    // Overload for long
    formatter& operator<<(long val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            if (m_pos < N - 1) m_data[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned long)val;
    }

    // Overload for unsigned long
    formatter& operator<<(unsigned long val) {
        if (val == 0) return *this << "0";
        char temp[24];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        while (i > 0 && m_pos < N - 1) m_data[m_pos++] = temp[--i];
        m_data[m_pos] = '\0';
        return *this;
    }

    // Overload for long long
    formatter& operator<<(long long val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            if (m_pos < N - 1) m_data[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned long long)val;
    }

    // Overload for unsigned long long
    formatter& operator<<(unsigned long long val) {
        if (val == 0) return *this << "0";
        char temp[24];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        while (i > 0 && m_pos < N - 1) m_data[m_pos++] = temp[--i];
        m_data[m_pos] = '\0';
        return *this;
    }

    // Overload for Hex
    formatter& operator<<(hex h) {
        *this << "0x";
        if (h.value == 0) return *this << "0";
        char temp[10];
        int i = 0;
        while (h.value > 0) {
            int d = h.value % 16;
            temp[i++] = (d < 10) ? (d + '0') : (d - 10 + 'a');
            h.value /= 16;
        }
        while (i > 0 && m_pos < N - 1) m_data[m_pos++] = temp[--i];
        m_data[m_pos] = '\0';
        return *this;
    }

    // Overload for float/double (defaults to 2 decimal places)
    formatter& operator<<(float val) {
        return *this << precision(val);
    }

    formatter& operator<<(double val) {
        return *this << precision((float)val);
    }

    // Overload for Floats (via precision wrapper)
    formatter& operator<<(precision p) {
        int i = (int)p.value;
        *this << i << ".";
        float frac = p.value - (float)i;
        if (frac < 0) frac = -frac;
        
        // Multiplier based on precision
        int mult = 1;
        for(int j=0; j < p.places; j++) mult *= 10;
        
        int ifrac = (int)(frac * mult + 0.5f);
        // Handle leading zeros in fraction (e.g. 1.05)
        int temp = ifrac;
        int digits = 0;
        if (temp == 0) digits = 1;
        while(temp > 0) { temp /= 10; digits++; }
        for(int j=0; j < p.places - digits; j++) *this << "0";
        
        return *this << ifrac;
    }

    // Check if buffer is full (for overflow detection)
    bool is_full() const { return m_pos >= N - 1; }
    size_t capacity() const { return N; }
};

// --- Dynamic Formatter (Heap-based, grows as needed) ---
class dynamic_formatter {
private:
    char* m_data = nullptr;
    size_t m_pos = 0;
    size_t m_capacity = 0;

    void ensure_capacity(size_t needed) {
        if (m_pos + needed < m_capacity) return;
        
        size_t new_cap = m_capacity == 0 ? 256 : m_capacity * 2;
        while (new_cap < m_pos + needed + 1) new_cap *= 2;
        
        char* new_data = (char*)webcc::malloc(new_cap);
        if (m_data) {
            for (size_t i = 0; i < m_pos; ++i) new_data[i] = m_data[i];
            webcc::free(m_data);
        }
        m_data = new_data;
        m_capacity = new_cap;
    }

public:
    dynamic_formatter() = default;
    ~dynamic_formatter() { if (m_data) webcc::free(m_data); }
    
    // No copy
    dynamic_formatter(const dynamic_formatter&) = delete;
    dynamic_formatter& operator=(const dynamic_formatter&) = delete;
    
    // Move OK
    dynamic_formatter(dynamic_formatter&& other) noexcept 
        : m_data(other.m_data), m_pos(other.m_pos), m_capacity(other.m_capacity) {
        other.m_data = nullptr;
        other.m_pos = 0;
        other.m_capacity = 0;
    }

    void clear() { m_pos = 0; if (m_data) m_data[0] = '\0'; }
    const char* c_str() const { return m_data ? m_data : ""; }
    size_t length() const { return m_pos; }

    // Release ownership of the buffer (caller takes ownership, must free with webcc::free)
    char* release() {
        char* ptr = m_data;
        m_data = nullptr;
        m_pos = 0;
        m_capacity = 0;
        return ptr;
    }

    dynamic_formatter& operator<<(const char* s) {
        if (!s) return *this;
        size_t len = 0;
        while (s[len]) len++;
        ensure_capacity(len);
        while (*s) m_data[m_pos++] = *s++;
        m_data[m_pos] = '\0';
        return *this;
    }

    dynamic_formatter& operator<<(string_view sv) {
        ensure_capacity(sv.length());
        for (size_t i = 0; i < sv.length(); ++i) {
            m_data[m_pos++] = sv[i];
        }
        m_data[m_pos] = '\0';
        return *this;
    }

    dynamic_formatter& operator<<(int val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            ensure_capacity(1);
            m_data[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned int)val;
    }

    dynamic_formatter& operator<<(unsigned int val) {
        if (val == 0) return *this << "0";
        char temp[12];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        ensure_capacity(i);
        while (i > 0) m_data[m_pos++] = temp[--i];
        m_data[m_pos] = '\0';
        return *this;
    }

    dynamic_formatter& operator<<(float val) {
        int i = (int)val;
        *this << i << ".";
        float frac = val - (float)i;
        if (frac < 0) frac = -frac;
        int ifrac = (int)(frac * 100 + 0.5f);
        if (ifrac < 10) *this << "0";
        return *this << ifrac;
    }
};

// --- Hybrid Formatter (Stack first, heap if overflow) ---
// Best of both worlds: fast for small strings, handles any size
template<size_t StackSize = 1024>
class hybrid_formatter {
private:
    char m_stack[StackSize];
    char* m_heap = nullptr;
    size_t m_pos = 0;
    size_t m_capacity = StackSize;
    bool m_on_heap = false;

    char* buffer() { return m_on_heap ? m_heap : m_stack; }
    const char* buffer() const { return m_on_heap ? m_heap : m_stack; }

    void ensure_capacity(size_t needed) {
        if (m_pos + needed < m_capacity) return;
        
        // Need to grow - switch to heap or grow heap
        size_t new_cap = m_capacity * 2;
        while (new_cap < m_pos + needed + 1) new_cap *= 2;
        
        char* new_data = (char*)webcc::malloc(new_cap);
        
        // Copy existing data
        char* src = buffer();
        for (size_t i = 0; i < m_pos; ++i) new_data[i] = src[i];
        
        // Free old heap if we had one
        if (m_on_heap && m_heap) webcc::free(m_heap);
        
        m_heap = new_data;
        m_capacity = new_cap;
        m_on_heap = true;
    }

public:
    hybrid_formatter() { m_stack[0] = '\0'; }
    ~hybrid_formatter() { if (m_on_heap && m_heap) webcc::free(m_heap); }
    
    // No copy
    hybrid_formatter(const hybrid_formatter&) = delete;
    hybrid_formatter& operator=(const hybrid_formatter&) = delete;

    const char* c_str() const { return buffer(); }
    size_t length() const { return m_pos; }
    bool on_heap() const { return m_on_heap; }

    // Release heap buffer (only valid if on_heap() is true)
    // Returns nullptr if still on stack
    char* release() {
        if (!m_on_heap) return nullptr;
        char* ptr = m_heap;
        m_heap = nullptr;
        m_pos = 0;
        m_capacity = StackSize;
        m_on_heap = false;
        return ptr;
    }

    hybrid_formatter& operator<<(const char* s) {
        if (!s) return *this;
        size_t len = 0;
        while (s[len]) len++;
        ensure_capacity(len);
        char* buf = buffer();
        while (*s) buf[m_pos++] = *s++;
        buf[m_pos] = '\0';
        return *this;
    }

    hybrid_formatter& operator<<(string_view sv) {
        ensure_capacity(sv.length());
        char* buf = buffer();
        for (size_t i = 0; i < sv.length(); ++i) {
            buf[m_pos++] = sv[i];
        }
        buf[m_pos] = '\0';
        return *this;
    }

    hybrid_formatter& operator<<(int val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            ensure_capacity(1);
            buffer()[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned int)val;
    }

    hybrid_formatter& operator<<(unsigned int val) {
        if (val == 0) return *this << "0";
        char temp[12];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        ensure_capacity(i);
        char* buf = buffer();
        while (i > 0) buf[m_pos++] = temp[--i];
        buf[m_pos] = '\0';
        return *this;
    }

    hybrid_formatter& operator<<(long val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            ensure_capacity(1);
            buffer()[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned long)val;
    }

    hybrid_formatter& operator<<(unsigned long val) {
        if (val == 0) return *this << "0";
        char temp[24];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        ensure_capacity(i);
        char* buf = buffer();
        while (i > 0) buf[m_pos++] = temp[--i];
        buf[m_pos] = '\0';
        return *this;
    }

    hybrid_formatter& operator<<(long long val) {
        if (val == 0) return *this << "0";
        if (val < 0) {
            ensure_capacity(1);
            buffer()[m_pos++] = '-';
            val = -val;
        }
        return *this << (unsigned long long)val;
    }

    hybrid_formatter& operator<<(unsigned long long val) {
        if (val == 0) return *this << "0";
        char temp[24];
        int i = 0;
        while (val > 0) {
            temp[i++] = (val % 10) + '0';
            val /= 10;
        }
        ensure_capacity(i);
        char* buf = buffer();
        while (i > 0) buf[m_pos++] = temp[--i];
        buf[m_pos] = '\0';
        return *this;
    }

    hybrid_formatter& operator<<(hex h) {
        *this << "0x";
        if (h.value == 0) return *this << "0";
        char temp[10];
        int i = 0;
        unsigned int v = h.value;
        while (v > 0) {
            int d = v % 16;
            temp[i++] = (d < 10) ? (d + '0') : (d - 10 + 'a');
            v /= 16;
        }
        ensure_capacity(i);
        char* buf = buffer();
        while (i > 0) buf[m_pos++] = temp[--i];
        buf[m_pos] = '\0';
        return *this;
    }

    hybrid_formatter& operator<<(float val) {
        return *this << precision(val);
    }

    hybrid_formatter& operator<<(double val) {
        return *this << precision((float)val);
    }

    hybrid_formatter& operator<<(precision p) {
        int i = (int)p.value;
        *this << i << ".";
        float frac = p.value - (float)i;
        if (frac < 0) frac = -frac;
        
        int mult = 1;
        for(int j=0; j < p.places; j++) mult *= 10;
        
        int ifrac = (int)(frac * mult + 0.5f);
        int temp = ifrac;
        int digits = 0;
        if (temp == 0) digits = 1;
        while(temp > 0) { temp /= 10; digits++; }
        for(int j=0; j < p.places - digits; j++) *this << "0";
        
        return *this << ifrac;
    }
};

} // namespace webcc