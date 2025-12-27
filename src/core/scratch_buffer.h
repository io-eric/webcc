#pragma once
#include <stdint.h>
#include <stddef.h>

namespace webcc
{
    // The Scratch Buffer is a shared memory region used for passing temporary data
    // from JavaScript back to C++ synchronously.
    //
    // USE CASE:
    // When a C++ function calls a JS function that returns a string (e.g. get_attribute),
    // JS cannot return the string directly (WASM only supports numbers). Instead:
    // 1. JS writes the string data into this scratch buffer.
    // 2. JS returns the length of the string.
    // 3. C++ immediately reads the data from the scratch buffer and copies it.
    //
    // This avoids dynamic memory allocation (malloc/free) for temporary return values.
    //
    // WARNING: This buffer is ephemeral. Data is only valid until the next JS call
    // that uses the scratch buffer.

    // Accessors for JS
    extern "C" uint8_t *webcc_scratch_buffer_ptr();
    extern "C" uint32_t webcc_scratch_buffer_capacity();

    // C++ API
    const uint8_t *scratch_buffer_data();
}
