// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::system {
    enum OpCode {
        OP_LOG = 0x30,
        OP_WARN = 0x31,
        OP_ERROR = 0x32,
        OP_SET_MAIN_LOOP = 0x33,
        OP_SET_TITLE = 0x34,
        OP_RELOAD = 0x35,
        OP_OPEN_URL = 0x36,
        OP_REQUEST_FULLSCREEN = 0x37
    };

    inline void log(const char* msg){
        push_command((uint8_t)OP_LOG);
        webcc::CommandBuffer::push_string(msg, strlen(msg));
    }

    inline void warn(const char* msg){
        push_command((uint8_t)OP_WARN);
        webcc::CommandBuffer::push_string(msg, strlen(msg));
    }

    inline void error(const char* msg){
        push_command((uint8_t)OP_ERROR);
        webcc::CommandBuffer::push_string(msg, strlen(msg));
    }

    inline void set_main_loop(const char* func_name){
        push_command((uint8_t)OP_SET_MAIN_LOOP);
        webcc::CommandBuffer::push_string(func_name, strlen(func_name));
    }

    inline void set_title(const char* title){
        push_command((uint8_t)OP_SET_TITLE);
        webcc::CommandBuffer::push_string(title, strlen(title));
    }

    inline void reload(){
        push_command((uint8_t)OP_RELOAD);
    }

    inline void open_url(const char* url){
        push_command((uint8_t)OP_OPEN_URL);
        webcc::CommandBuffer::push_string(url, strlen(url));
    }

    inline void request_fullscreen(int32_t handle){
        push_command((uint8_t)OP_REQUEST_FULLSCREEN);
        push_data<int32_t>(handle);
    }

} // namespace webcc::system

