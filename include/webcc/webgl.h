// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::webgl {
    enum OpCode {
        OP_CREATE_CONTEXT = 0x46,
        OP_VIEWPORT = 0x47,
        OP_CLEAR_COLOR = 0x48,
        OP_CLEAR = 0x49,
        OP_CREATE_SHADER = 0x4a,
        OP_CREATE_PROGRAM = 0x4b,
        OP_ATTACH_SHADER = 0x4c,
        OP_LINK_PROGRAM = 0x4d,
        OP_BIND_ATTRIB_LOCATION = 0x4e,
        OP_USE_PROGRAM = 0x4f,
        OP_CREATE_BUFFER = 0x50,
        OP_BIND_BUFFER = 0x51,
        OP_BUFFER_DATA = 0x52,
        OP_ENABLE_VERTEX_ATTRIB_ARRAY = 0x53,
        OP_ENABLE = 0x54,
        OP_GET_UNIFORM_LOCATION = 0x55,
        OP_UNIFORM_1F = 0x56,
        OP_VERTEX_ATTRIB_POINTER = 0x57,
        OP_DRAW_ARRAYS = 0x58
    };

    extern "C" int32_t webcc_webgl_create_context(int32_t canvas_handle);
    inline int32_t create_context(int32_t canvas_handle){
        ::webcc::flush();
        return webcc_webgl_create_context(canvas_handle);
    }

    inline void viewport(int32_t ctx_handle, int32_t x, int32_t y, int32_t width, int32_t height){
        push_command((uint32_t)OP_VIEWPORT);
        push_data<int32_t>(ctx_handle);
        push_data<int32_t>(x);
        push_data<int32_t>(y);
        push_data<int32_t>(width);
        push_data<int32_t>(height);
    }

    inline void clear_color(int32_t ctx_handle, float r, float g, float b, float a){
        push_command((uint32_t)OP_CLEAR_COLOR);
        push_data<int32_t>(ctx_handle);
        push_data<float>(r);
        push_data<float>(g);
        push_data<float>(b);
        push_data<float>(a);
    }

    inline void clear(int32_t ctx_handle, uint32_t mask){
        push_command((uint32_t)OP_CLEAR);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(mask);
    }

    extern "C" int32_t webcc_webgl_create_shader(int32_t ctx_handle, uint32_t type, const char* source, uint32_t source_len);
    inline int32_t create_shader(int32_t ctx_handle, uint32_t type, const char* source){
        ::webcc::flush();
        return webcc_webgl_create_shader(ctx_handle, type, source, webcc::strlen(source));
    }

    extern "C" int32_t webcc_webgl_create_program(int32_t ctx_handle);
    inline int32_t create_program(int32_t ctx_handle){
        ::webcc::flush();
        return webcc_webgl_create_program(ctx_handle);
    }

    inline void attach_shader(int32_t ctx_handle, int32_t prog_handle, int32_t shader_handle){
        push_command((uint32_t)OP_ATTACH_SHADER);
        push_data<int32_t>(ctx_handle);
        push_data<int32_t>(prog_handle);
        push_data<int32_t>(shader_handle);
    }

    inline void link_program(int32_t ctx_handle, int32_t prog_handle){
        push_command((uint32_t)OP_LINK_PROGRAM);
        push_data<int32_t>(ctx_handle);
        push_data<int32_t>(prog_handle);
    }

    inline void bind_attrib_location(int32_t ctx_handle, int32_t prog_handle, uint32_t index, const char* name){
        push_command((uint32_t)OP_BIND_ATTRIB_LOCATION);
        push_data<int32_t>(ctx_handle);
        push_data<int32_t>(prog_handle);
        push_data<uint32_t>(index);
        webcc::CommandBuffer::push_string(name, strlen(name));
    }

    inline void use_program(int32_t ctx_handle, int32_t prog_handle){
        push_command((uint32_t)OP_USE_PROGRAM);
        push_data<int32_t>(ctx_handle);
        push_data<int32_t>(prog_handle);
    }

    extern "C" int32_t webcc_webgl_create_buffer(int32_t ctx_handle);
    inline int32_t create_buffer(int32_t ctx_handle){
        ::webcc::flush();
        return webcc_webgl_create_buffer(ctx_handle);
    }

    inline void bind_buffer(int32_t ctx_handle, uint32_t target, int32_t buf_handle){
        push_command((uint32_t)OP_BIND_BUFFER);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(target);
        push_data<int32_t>(buf_handle);
    }

    inline void buffer_data(int32_t ctx_handle, uint32_t target, uint32_t data_ptr, uint32_t data_len, uint32_t usage){
        push_command((uint32_t)OP_BUFFER_DATA);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(target);
        push_data<uint32_t>(data_ptr);
        push_data<uint32_t>(data_len);
        push_data<uint32_t>(usage);
    }

    inline void enable_vertex_attrib_array(int32_t ctx_handle, uint32_t index){
        push_command((uint32_t)OP_ENABLE_VERTEX_ATTRIB_ARRAY);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(index);
    }

    inline void enable(int32_t ctx_handle, uint32_t cap){
        push_command((uint32_t)OP_ENABLE);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(cap);
    }

    extern "C" int32_t webcc_webgl_get_uniform_location(int32_t ctx_handle, int32_t prog_handle, const char* name, uint32_t name_len);
    inline int32_t get_uniform_location(int32_t ctx_handle, int32_t prog_handle, const char* name){
        ::webcc::flush();
        return webcc_webgl_get_uniform_location(ctx_handle, prog_handle, name, webcc::strlen(name));
    }

    inline void uniform_1f(int32_t ctx_handle, int32_t loc_handle, float val){
        push_command((uint32_t)OP_UNIFORM_1F);
        push_data<int32_t>(ctx_handle);
        push_data<int32_t>(loc_handle);
        push_data<float>(val);
    }

    inline void vertex_attrib_pointer(int32_t ctx_handle, uint32_t index, int32_t size, uint32_t type, uint8_t normalized, int32_t stride, int32_t offset){
        push_command((uint32_t)OP_VERTEX_ATTRIB_POINTER);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(index);
        push_data<int32_t>(size);
        push_data<uint32_t>(type);
        push_data<uint32_t>((uint32_t)normalized);
        push_data<int32_t>(stride);
        push_data<int32_t>(offset);
    }

    inline void draw_arrays(int32_t ctx_handle, uint32_t mode, int32_t first, int32_t count){
        push_command((uint32_t)OP_DRAW_ARRAYS);
        push_data<int32_t>(ctx_handle);
        push_data<uint32_t>(mode);
        push_data<int32_t>(first);
        push_data<int32_t>(count);
    }

} // namespace webcc::webgl

