// This file provides minimal implementations of standard C library functions.
// Since we compile with -nostdlib, these are not available by default.
// Compilers (especially older versions or at certain optimization levels) may
// implicitly generate calls to these functions (e.g. for struct copying or initialization).
// Providing them here ensures the WASM module is self-contained and avoids LinkErrors.

#include <stddef.h>
#include <stdint.h>

extern "C"
{

    void *memcpy(void *dest, const void *src, size_t n)
    {
        uint8_t *d = static_cast<uint8_t *>(dest);
        const uint8_t *s = static_cast<const uint8_t *>(src);
        while (n--)
            *d++ = *s++;
        return dest;
    }

    void *memset(void *dest, int c, size_t n)
    {
        uint8_t *d = static_cast<uint8_t *>(dest);
        while (n--)
            *d++ = static_cast<uint8_t>(c);
        return dest;
    }

    void *memmove(void *dest, const void *src, size_t n)
    {
        uint8_t *d = static_cast<uint8_t *>(dest);
        const uint8_t *s = static_cast<const uint8_t *>(src);
        if (d < s)
        {
            while (n--)
                *d++ = *s++;
        }
        else
        {
            d += n;
            s += n;
            while (n--)
                *--d = *--s;
        }
        return dest;
    }
}
