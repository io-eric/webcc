#pragma once
#include "webcc.h"

namespace webcc::websocket {
    enum OpCode {
        OP_CREATE = 0x42,
        OP_SEND = 0x43,
        OP_CLOSE = 0x44
    };

    extern "C" int32_t webcc_websocket_create(const char* url, uint32_t url_len);
    inline int32_t create(const char* url){
        ::webcc::flush();
        return webcc_websocket_create(url, webcc::strlen(url));
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

