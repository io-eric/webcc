#pragma once
#include <stdint.h>
#include <stddef.h>

namespace webcc
{

    // Accessors for JS
    extern "C" uint8_t *webcc_event_buffer_ptr();
    extern "C" uint32_t *webcc_event_offset_ptr();

    // C++ API
    void reset_event_buffer();
    const uint8_t *event_buffer_data();
    uint32_t event_buffer_size();

}
