// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::dom {
    enum OpCode {
        OP_GET_BODY = 0x1,
        OP_CREATE_ELEMENT = 0x2,
        OP_SET_ATTRIBUTE = 0x3,
        OP_GET_ATTRIBUTE = 0x4,
        OP_APPEND_CHILD = 0x5,
        OP_REMOVE_ELEMENT = 0x6,
        OP_SET_INNER_HTML = 0x7,
        OP_SET_INNER_TEXT = 0x8,
        OP_ADD_CLASS = 0x9,
        OP_REMOVE_CLASS = 0xa
    };

    extern "C" int32_t webcc_dom_get_body();
    inline int32_t get_body(){
        ::webcc::flush();
        return webcc_dom_get_body();
    }

    extern "C" int32_t webcc_dom_create_element(const char* tag, uint32_t tag_len);
    inline int32_t create_element(const char* tag){
        ::webcc::flush();
        return webcc_dom_create_element(tag, webcc::strlen(tag));
    }

    inline void set_attribute(int32_t handle, const char* name, const char* value){
        push_command((uint8_t)OP_SET_ATTRIBUTE);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(name, strlen(name));
        webcc::CommandBuffer::push_string(value, strlen(value));
    }

    inline void get_attribute(int32_t handle, const char* name){
        push_command((uint8_t)OP_GET_ATTRIBUTE);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(name, strlen(name));
    }

    inline void append_child(int32_t parent_handle, int32_t child_handle){
        push_command((uint8_t)OP_APPEND_CHILD);
        push_data<int32_t>(parent_handle);
        push_data<int32_t>(child_handle);
    }

    inline void remove_element(int32_t handle){
        push_command((uint8_t)OP_REMOVE_ELEMENT);
        push_data<int32_t>(handle);
    }

    inline void set_inner_html(int32_t handle, const char* html){
        push_command((uint8_t)OP_SET_INNER_HTML);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(html, strlen(html));
    }

    inline void set_inner_text(int32_t handle, const char* text){
        push_command((uint8_t)OP_SET_INNER_TEXT);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(text, strlen(text));
    }

    inline void add_class(int32_t handle, const char* cls){
        push_command((uint8_t)OP_ADD_CLASS);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(cls, strlen(cls));
    }

    inline void remove_class(int32_t handle, const char* cls){
        push_command((uint8_t)OP_REMOVE_CLASS);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(cls, strlen(cls));
    }

} // namespace webcc::dom

