#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../../src/core/command_buffer.h"
#include "../../src/core/event_buffer.h"
#include "../../src/core/scratch_buffer.h"
#include "core/optional.h"
#include "core/string.h"
#include "core/vector.h"
#include "core/js.h"

// Copies a string/bytes result too large for the scratch buffer into dst
extern "C" void webcc_js_read_result(void* dst);

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

    template<>
    inline void push_data<double>(double value){
        CommandBuffer::push_double(value);
    }

    // Result of a string-returning command, `len` as returned by the import
    inline string take_string_result(uint32_t len){
        if (len <= SCRATCH_BUFFER_SIZE)
            return string((const char*)scratch_buffer_data(), len);
        char* buf = (char*)webcc::malloc(len + 1);
        if (!buf) return string();
        webcc_js_read_result(buf);
        buf[len] = '\0';
        return string::adopt(buf, len);
    }

    // Result of a bytes-returning command, `len` as returned by the import
    inline vector<uint8_t> take_bytes_result(uint32_t len){
        vector<uint8_t> out;
        out.resize(len);
        if (out.size() != len) return vector<uint8_t>();
        if (len <= SCRATCH_BUFFER_SIZE)
            __builtin_memcpy(out.data(), scratch_buffer_data(), len);
        else
            webcc_js_read_result(out.data());
        return out;
    }

    inline void push_command(uint32_t opcode){
        CommandBuffer::push_command(opcode);
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

    // =========================================================================
    // Deferred DOM element creation (for batched DOM creation)
    // C++ assigns handles from a high starting number to avoid collision with
    // JS-assigned handles. 
    // =========================================================================
    
    inline int32_t& deferred_handle_counter() {
        static int32_t counter = 0x100000;  // Start high to avoid JS collision
        return counter;
    }

    inline int32_t next_deferred_handle() {
        return deferred_handle_counter()++;
    }

    // Reserves n consecutive handles and returns the first one.
    // Saves reloading the counter for every node when creating many at once.
    inline int32_t reserve_deferred_handles(int32_t n) {
        int32_t& counter = deferred_handle_counter();
        int32_t first = counter;
        counter += n;
        return first;
    }
}