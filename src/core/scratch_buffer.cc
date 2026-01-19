#include "scratch_buffer.h"

namespace webcc
{
    // 4KB should be enough for most return values (URLs, attributes, JSON chunks)
    constexpr size_t SCRATCH_BUFFER_SIZE = 4096; 
    
    alignas(8) static uint8_t g_scratch_buffer[SCRATCH_BUFFER_SIZE];

    extern "C" uint8_t *webcc_scratch_buffer_ptr()
    {
        return g_scratch_buffer;
    }

    extern "C" uint32_t webcc_scratch_buffer_capacity()
    {
        return SCRATCH_BUFFER_SIZE;
    }

    const uint8_t *scratch_buffer_data()
    {
        return g_scratch_buffer;
    }
}
