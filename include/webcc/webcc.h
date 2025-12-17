#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../../src/command_buffer.h"
#include "../../src/event_buffer.h"

namespace webcc{

    // Simple strlen for WASM
    static inline size_t strlen(const char* s) {
        size_t len = 0;
        while (s[len]) ++len;
        return len;
    }

    // The trigger. This calls a JS function (imported)
    void flush();

    // Internal helpers for command serialization
    template<typename T>
    inline void push_data(T value);

    template<>
    inline void push_data<uint32_t>(uint32_t value){
        CommandBuffer::push_u32(value);
    }

    template<>
    inline void push_data<int32_t>(int32_t value){
        CommandBuffer::push_i32(value);
    }

    template<>
    inline void push_data<float>(float value){
        CommandBuffer::push_float(value);
    }

    inline void push_command(uint32_t opcode){
        CommandBuffer::push_u32(opcode);
    }

    static uint32_t g_read_offset = 0;

    inline bool poll_event(uint8_t& opcode, const uint8_t** data_ptr, uint32_t& data_len) {
        uint32_t size = event_buffer_size();
        if (g_read_offset >= size) {
            reset_event_buffer();
            g_read_offset = 0;
            return false;
        }
        const uint8_t* buf = event_buffer_data();
        // Format: [Opcode:1][Size:2][Data...]
        if (g_read_offset + 3 > size) {
             // Malformed or incomplete? Reset.
            reset_event_buffer();
            g_read_offset = 0;
            return false;
        }

        opcode = buf[g_read_offset];
        uint16_t event_len = (uint16_t)buf[g_read_offset + 1] | ((uint16_t)buf[g_read_offset + 2] << 8);
        
        if (g_read_offset + event_len > size) {
             // Incomplete event?
            reset_event_buffer();
            g_read_offset = 0;
            return false;
        }

        *data_ptr = buf + g_read_offset + 3;
        data_len = event_len - 3;

        g_read_offset += event_len;
        return true;
    }
}
