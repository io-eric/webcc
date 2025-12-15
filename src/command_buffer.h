#pragma once

#include <stdint.h>
#include <stddef.h>

namespace webcc {

struct CommandBuffer {
    // Append a single byte to the buffer
    static void push_byte(uint8_t b);

    // Append raw bytes to the buffer
    static void push_bytes(const uint8_t* data, size_t len);

    // Append a string with caching/interning support
    static void push_string(const char* str, size_t len);

    // Accessors used by the JS runtime (exported C symbols call these)
    // These access the "snapshot" buffer which is stable until
    // `reset()` is called. Writers append to the producer buffer and a
    // swap is performed when JS asks for the buffer ptr.
    static const uint8_t* data();
    static size_t size();
    static void reset();
};

} // namespace webcc
