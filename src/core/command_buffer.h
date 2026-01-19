#pragma once

#include <stdint.h>
#include <stddef.h>

namespace webcc {

struct CommandBuffer {
    // Append a 32-bit integer (aligned)
    static void push_u32(uint32_t v);
    static void push_i32(int32_t v);
    static void push_float(float v);
    static void push_double(double v);

    // Append a string (aligned)
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
