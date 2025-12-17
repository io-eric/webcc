// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::input {
    enum OpCode {
        OP_INIT_KEYBOARD = 0x2c,
        OP_INIT_MOUSE = 0x2d,
        OP_REQUEST_POINTER_LOCK = 0x2e,
        OP_EXIT_POINTER_LOCK = 0x2f
    };

    enum EventType {
        EVENT_KEY_DOWN = 0x1,
        EVENT_KEY_UP = 0x2,
        EVENT_MOUSE_DOWN = 0x3,
        EVENT_MOUSE_UP = 0x4,
        EVENT_MOUSE_MOVE = 0x5
    };

    enum EventMask {
        MASK_KEY_DOWN = 1 << 0,
        MASK_KEY_UP = 1 << 1,
        MASK_MOUSE_DOWN = 1 << 2,
        MASK_MOUSE_UP = 1 << 3,
        MASK_MOUSE_MOVE = 1 << 4
    };

    inline void init_keyboard(){
        push_command((uint32_t)OP_INIT_KEYBOARD);
    }

    inline void init_mouse(int32_t handle){
        push_command((uint32_t)OP_INIT_MOUSE);
        push_data<int32_t>(handle);
    }

    inline void request_pointer_lock(int32_t handle){
        push_command((uint32_t)OP_REQUEST_POINTER_LOCK);
        push_data<int32_t>(handle);
    }

    inline void exit_pointer_lock(){
        push_command((uint32_t)OP_EXIT_POINTER_LOCK);
    }

} // namespace webcc::input

