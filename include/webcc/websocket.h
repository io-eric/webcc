// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::websocket {
    enum OpCode {
        OP_CREATE = 0x42,
        OP_SEND = 0x43,
        OP_CLOSE = 0x44
    };

    enum EventType {
        EVENT_MESSAGE = 0x6,
        EVENT_OPEN = 0x7,
        EVENT_CLOSE = 0x8,
        EVENT_ERROR = 0x9
    };

    enum EventMask {
        MASK_MESSAGE = 1 << 0,
        MASK_OPEN = 1 << 1,
        MASK_CLOSE = 1 << 2,
        MASK_ERROR = 1 << 3
    };

    extern "C" int32_t webcc_websocket_create(const char* url, uint32_t url_len, uint32_t events);
    inline int32_t create(const char* url, uint32_t events){
        ::webcc::flush();
        return webcc_websocket_create(url, webcc::strlen(url), events);
    }

    inline void send(int32_t handle, const char* msg){
        push_command((uint8_t)OP_SEND);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(msg, strlen(msg));
    }

    inline void close(int32_t handle){
        push_command((uint8_t)OP_CLOSE);
        push_data<int32_t>(handle);
    }

} // namespace webcc::websocket

