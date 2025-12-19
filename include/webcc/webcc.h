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

    inline bool poll_event(uint8_t& opcode, const uint8_t** data_ptr, uint32_t& data_len) {
        return next_event(opcode, data_ptr, data_len);
    }
}
