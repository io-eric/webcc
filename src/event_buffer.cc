#include "event_buffer.h"

namespace webcc
{

    constexpr size_t EVENT_BUFFER_SIZE = 1024 * 1024; // 1MB
    static uint8_t g_event_buffer[EVENT_BUFFER_SIZE];
    static uint32_t g_event_offset = 0;

    extern "C" uint8_t *webcc_event_buffer_ptr()
    {
        return g_event_buffer;
    }

    extern "C" uint32_t *webcc_event_offset_ptr()
    {
        return &g_event_offset;
    }

    void reset_event_buffer()
    {
        g_event_offset = 0;
    }

    const uint8_t *event_buffer_data()
    {
        return g_event_buffer;
    }

    uint32_t event_buffer_size()
    {
        return g_event_offset;
    }

}
