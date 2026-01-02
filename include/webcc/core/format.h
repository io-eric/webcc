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
};

} // namespace webcc