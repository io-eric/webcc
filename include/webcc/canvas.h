// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"

namespace webcc::canvas {
    enum OpCode {
        OP_CREATE_CANVAS = 0xb,
        OP_SET_SIZE = 0xc,
        OP_SET_FILL_STYLE = 0xd,
        OP_SET_FILL_STYLE_STR = 0xe,
        OP_FILL_RECT = 0xf,
        OP_CLEAR_RECT = 0x10,
        OP_STROKE_RECT = 0x11,
        OP_SET_STROKE_STYLE = 0x12,
        OP_SET_STROKE_STYLE_STR = 0x13,
        OP_SET_LINE_WIDTH = 0x14,
        OP_BEGIN_PATH = 0x15,
        OP_CLOSE_PATH = 0x16,
        OP_MOVE_TO = 0x17,
        OP_LINE_TO = 0x18,
        OP_STROKE = 0x19,
        OP_FILL = 0x1a,
        OP_ARC = 0x1b,
        OP_FILL_TEXT = 0x1c,
        OP_FILL_TEXT_F = 0x1d,
        OP_FILL_TEXT_I = 0x1e,
        OP_SET_FONT = 0x1f,
        OP_SET_TEXT_ALIGN = 0x20,
        OP_DRAW_IMAGE = 0x21,
        OP_TRANSLATE = 0x22,
        OP_ROTATE = 0x23,
        OP_SCALE = 0x24,
        OP_SAVE = 0x25,
        OP_RESTORE = 0x26,
        OP_LOG_CANVAS_INFO = 0x27,
        OP_SET_GLOBAL_ALPHA = 0x28,
        OP_SET_LINE_CAP = 0x29,
        OP_SET_LINE_JOIN = 0x2a,
        OP_SET_SHADOW = 0x2b
    };

    extern "C" int32_t webcc_canvas_create_canvas(const char* dom_id, uint32_t dom_id_len, float width, float height);
    inline int32_t create_canvas(const char* dom_id, float width, float height){
        ::webcc::flush();
        return webcc_canvas_create_canvas(dom_id, webcc::strlen(dom_id), width, height);
    }

    inline void set_size(int32_t handle, float width, float height){
        push_command((uint8_t)OP_SET_SIZE);
        push_data<int32_t>(handle);
        push_data<float>(width);
        push_data<float>(height);
    }

    inline void set_fill_style(int32_t handle, uint8_t r, uint8_t g, uint8_t b){
        push_command((uint8_t)OP_SET_FILL_STYLE);
        push_data<int32_t>(handle);
        push_data<uint8_t>(r);
        push_data<uint8_t>(g);
        push_data<uint8_t>(b);
    }

    inline void set_fill_style_str(int32_t handle, const char* color){
        push_command((uint8_t)OP_SET_FILL_STYLE_STR);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(color, strlen(color));
    }

    inline void fill_rect(int32_t handle, float x, float y, float w, float h){
        push_command((uint8_t)OP_FILL_RECT);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
        push_data<float>(w);
        push_data<float>(h);
    }

    inline void clear_rect(int32_t handle, float x, float y, float w, float h){
        push_command((uint8_t)OP_CLEAR_RECT);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
        push_data<float>(w);
        push_data<float>(h);
    }

    inline void stroke_rect(int32_t handle, float x, float y, float w, float h){
        push_command((uint8_t)OP_STROKE_RECT);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
        push_data<float>(w);
        push_data<float>(h);
    }

    inline void set_stroke_style(int32_t handle, uint8_t r, uint8_t g, uint8_t b){
        push_command((uint8_t)OP_SET_STROKE_STYLE);
        push_data<int32_t>(handle);
        push_data<uint8_t>(r);
        push_data<uint8_t>(g);
        push_data<uint8_t>(b);
    }

    inline void set_stroke_style_str(int32_t handle, const char* color){
        push_command((uint8_t)OP_SET_STROKE_STYLE_STR);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(color, strlen(color));
    }

    inline void set_line_width(int32_t handle, float width){
        push_command((uint8_t)OP_SET_LINE_WIDTH);
        push_data<int32_t>(handle);
        push_data<float>(width);
    }

    inline void begin_path(int32_t handle){
        push_command((uint8_t)OP_BEGIN_PATH);
        push_data<int32_t>(handle);
    }

    inline void close_path(int32_t handle){
        push_command((uint8_t)OP_CLOSE_PATH);
        push_data<int32_t>(handle);
    }

    inline void move_to(int32_t handle, float x, float y){
        push_command((uint8_t)OP_MOVE_TO);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void line_to(int32_t handle, float x, float y){
        push_command((uint8_t)OP_LINE_TO);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void stroke(int32_t handle){
        push_command((uint8_t)OP_STROKE);
        push_data<int32_t>(handle);
    }

    inline void fill(int32_t handle){
        push_command((uint8_t)OP_FILL);
        push_data<int32_t>(handle);
    }

    inline void arc(int32_t handle, float x, float y, float radius, float start_angle, float end_angle){
        push_command((uint8_t)OP_ARC);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
        push_data<float>(radius);
        push_data<float>(start_angle);
        push_data<float>(end_angle);
    }

    inline void fill_text(int32_t handle, const char* text, float x, float y){
        push_command((uint8_t)OP_FILL_TEXT);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(text, strlen(text));
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void fill_text_f(int32_t handle, const char* fmt, float val, float x, float y){
        push_command((uint8_t)OP_FILL_TEXT_F);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(fmt, strlen(fmt));
        push_data<float>(val);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void fill_text_i(int32_t handle, const char* fmt, int32_t val, float x, float y){
        push_command((uint8_t)OP_FILL_TEXT_I);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(fmt, strlen(fmt));
        push_data<int32_t>(val);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void set_font(int32_t handle, const char* font){
        push_command((uint8_t)OP_SET_FONT);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(font, strlen(font));
    }

    inline void set_text_align(int32_t handle, const char* align){
        push_command((uint8_t)OP_SET_TEXT_ALIGN);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(align, strlen(align));
    }

    inline void draw_image(int32_t handle, int32_t img_handle, float x, float y){
        push_command((uint8_t)OP_DRAW_IMAGE);
        push_data<int32_t>(handle);
        push_data<int32_t>(img_handle);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void translate(int32_t handle, float x, float y){
        push_command((uint8_t)OP_TRANSLATE);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void rotate(int32_t handle, float angle){
        push_command((uint8_t)OP_ROTATE);
        push_data<int32_t>(handle);
        push_data<float>(angle);
    }

    inline void scale(int32_t handle, float x, float y){
        push_command((uint8_t)OP_SCALE);
        push_data<int32_t>(handle);
        push_data<float>(x);
        push_data<float>(y);
    }

    inline void save(int32_t handle){
        push_command((uint8_t)OP_SAVE);
        push_data<int32_t>(handle);
    }

    inline void restore(int32_t handle){
        push_command((uint8_t)OP_RESTORE);
        push_data<int32_t>(handle);
    }

    inline void log_canvas_info(int32_t handle){
        push_command((uint8_t)OP_LOG_CANVAS_INFO);
        push_data<int32_t>(handle);
    }

    inline void set_global_alpha(int32_t handle, float alpha){
        push_command((uint8_t)OP_SET_GLOBAL_ALPHA);
        push_data<int32_t>(handle);
        push_data<float>(alpha);
    }

    inline void set_line_cap(int32_t handle, const char* cap){
        push_command((uint8_t)OP_SET_LINE_CAP);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(cap, strlen(cap));
    }

    inline void set_line_join(int32_t handle, const char* join){
        push_command((uint8_t)OP_SET_LINE_JOIN);
        push_data<int32_t>(handle);
        webcc::CommandBuffer::push_string(join, strlen(join));
    }

    inline void set_shadow(int32_t handle, float blur, float off_x, float off_y, const char* color){
        push_command((uint8_t)OP_SET_SHADOW);
        push_data<int32_t>(handle);
        push_data<float>(blur);
        push_data<float>(off_x);
        push_data<float>(off_y);
        webcc::CommandBuffer::push_string(color, strlen(color));
    }

} // namespace webcc::canvas

