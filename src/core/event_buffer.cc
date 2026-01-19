#include "event_buffer.h"

namespace webcc
{

    constexpr size_t EVENT_BUFFER_SIZE = 1024 * 1024; // 1MB
    // Align to 8 bytes so that JS Float64Array can access it directly
    alignas(8) static uint8_t g_event_buffer[EVENT_BUFFER_SIZE];
    static uint32_t g_event_offset = 0;
    static uint32_t g_read_offset = 0;

    extern "C" uint8_t *webcc_event_buffer_ptr()
    {
        return g_event_buffer;
    }

    extern "C" uint32_t *webcc_event_offset_ptr()
    {
        return &g_event_offset;
    }

    extern "C" uint32_t webcc_event_buffer_capacity()
    {
        return EVENT_BUFFER_SIZE;
    }

    void reset_event_buffer()
    {
        g_event_offset = 0;
        g_read_offset = 0;
    }

    const uint8_t *event_buffer_data()
    {
        return g_event_buffer;
    }

    uint32_t event_buffer_size()
    {
        return g_event_offset;
    }

    bool next_event(uint8_t& opcode, const uint8_t** data_ptr, uint32_t& data_len) {
        uint32_t size = g_event_offset;
        if (g_read_offset >= size) {
            reset_event_buffer();
            return false;
        }
        
        // Format: [Opcode:1][Pad:1][Size:2][Data...]
        if (g_read_offset + 4 > size) {
             // Malformed or incomplete? Reset.
            reset_event_buffer();
            return false;
        }

        opcode = g_event_buffer[g_read_offset];
        uint16_t event_len = (uint16_t)g_event_buffer[g_read_offset + 2] | ((uint16_t)g_event_buffer[g_read_offset + 3] << 8);
        
        if (g_read_offset + event_len > size) {
             // Incomplete event?
            reset_event_buffer();
            return false;
        }

        *data_ptr = g_event_buffer + g_read_offset + 4;
        data_len = event_len - 4;

        g_read_offset += event_len;
        return true;
    }

}
