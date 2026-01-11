#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../../src/core/command_buffer.h"
#include "../../src/core/event_buffer.h"
#include "../../src/core/scratch_buffer.h"
#include "core/optional.h"

namespace webcc
{
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

    struct Event {
        uint8_t opcode;
        const uint8_t* data;
        uint32_t len;

        template <typename T>
        webcc::optional<T> as() const {
            if (opcode == T::OPCODE) {
                return T::parse(data, len);
            }
            return {};
        }
    };

    inline bool poll_event(Event& event) {
        return next_event(event.opcode, &event.data, event.len);
    }

    template <typename T>
    inline T parse_event(const uint8_t* data, uint32_t len) {
        return T::parse(data, len);
    }
}