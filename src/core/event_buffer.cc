#include "event_buffer.h"

namespace webcc
{

    constexpr size_t EVENT_BUFFER_SIZE = 1024 * 1024; // 1MB
    // Event length is 24 bits, see next_event()
    static_assert(EVENT_BUFFER_SIZE <= (1u << 24), "event length field is 24 bits");
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
        
        // Format: [Opcode:1][SizeHi:1][SizeLo:2][Data...]
        // Size is 24 bits and includes the header
        if (g_read_offset + 4 > size) {
             // Malformed or incomplete? Reset.
            reset_event_buffer();
            return false;
        }

        opcode = g_event_buffer[g_read_offset];
        uint32_t event_len = (uint32_t)g_event_buffer[g_read_offset + 2] |
                             ((uint32_t)g_event_buffer[g_read_offset + 3] << 8) |
                             ((uint32_t)g_event_buffer[g_read_offset + 1] << 16);

        if (event_len < 4 || g_read_offset + event_len > size) {
             // Malformed or incomplete event?
            reset_event_buffer();
            return false;
        }

        *data_ptr = g_event_buffer + g_read_offset + 4;
        data_len = event_len - 4;

        g_read_offset += event_len;
        return true;
    }

}
