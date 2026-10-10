// This file provides minimal implementations of standard C library functions.
// Since we compile with -nostdlib, these are not available by default.
// Compilers (especially older versions or at certain optimization levels) may
// implicitly generate calls to these functions (e.g. for struct copying or initialization).
// Providing them here ensures the WASM module is self-contained and avoids LinkErrors.

#include <stddef.h>
#include <stdint.h>
#include "webcc/core/allocator.h"

extern "C"
{
    // An unoptimized build (-O0) keeps calls the optimizer would drop: the stub behind a pure
    // virtual method, and the registration of static destructors. A pure virtual call is a
    // bug, so it traps; destructors never run on a page that lives until it is closed
    void __cxa_pure_virtual() { __builtin_trap(); }
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }

    size_t strlen(const char *s)
    {
        const char *p = s;
        while (*p)
            ++p;
        return p - s;
    }

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

// Global C++ allocation operators must be outside extern "C" 
// so they use C++ mangling (e.g., _Znwm, _ZdlPv) which the linker expects.
void* operator new(size_t size)
{
    return webcc::malloc(size);
}

void operator delete(void* p) noexcept
{
    webcc::free(p);
}

void operator delete(void* p, size_t) noexcept
{
    webcc::free(p);
}

void* operator new[](size_t size)
{
    return webcc::malloc(size);
}

void operator delete[](void* p) noexcept
{
    webcc::free(p);
}

void operator delete[](void* p, size_t) noexcept
{
    webcc::free(p);
}
