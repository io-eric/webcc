// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::storage {
    enum OpCode {
        OP_SET_ITEM = 0x38,
        OP_REMOVE_ITEM = 0x39,
        OP_CLEAR = 0x3a
    };

    inline void set_item(const char* key, const char* value){
        push_command((uint32_t)OP_SET_ITEM);
        webcc::CommandBuffer::push_string(key, strlen(key));
        webcc::CommandBuffer::push_string(value, strlen(value));
    }

    inline void remove_item(const char* key){
        push_command((uint32_t)OP_REMOVE_ITEM);
        webcc::CommandBuffer::push_string(key, strlen(key));
    }

    inline void clear(){
        push_command((uint32_t)OP_CLEAR);
    }

} // namespace webcc::storage

