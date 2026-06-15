// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc.h"
#include "webcc/core/handle.h"
#include "webcc/core/handles.h"
#include "webcc/core/string_view.h"
#include "webcc/core/string.h"
namespace webcc::canvas {
    enum OpCode {
        OP_CREATE_CANVAS = 0x1e,
        OP_GET_CONTEXT_2D = 0x1f,
        OP_GET_CONTEXT_WEBGL = 0x20,
        OP_GET_CONTEXT_WEBGPU = 0x21,
        OP_SET_SIZE = 0x22,
        OP_SET_FILL_STYLE = 0x23,
        OP_SET_FILL_STYLE_STR = 0x24,
        OP_FILL_RECT = 0x25,
        OP_CLEAR_RECT = 0x26,
        OP_STROKE_RECT = 0x27,
        OP_SET_STROKE_STYLE = 0x28,
        OP_SET_STROKE_STYLE_STR = 0x29,
        OP_SET_LINE_WIDTH = 0x2a,
        OP_BEGIN_PATH = 0x2b,
        OP_CLOSE_PATH = 0x2c,
        OP_MOVE_TO = 0x2d,
        OP_LINE_TO = 0x2e,
        OP_STROKE = 0x2f,
        OP_FILL = 0x30,
        OP_ARC = 0x31,
        OP_FILL_TEXT = 0x32,
        OP_FILL_TEXT_F = 0x33,
        OP_FILL_TEXT_I = 0x34,
        OP_SET_FONT = 0x35,
        OP_SET_TEXT_ALIGN = 0x36,
        OP_DRAW_IMAGE = 0x37,
        OP_TRANSLATE = 0x38,
        OP_ROTATE = 0x39,
        OP_SCALE = 0x3a,
        OP_SAVE = 0x3b,
        OP_RESTORE = 0x3c,
        OP_LOG_CANVAS_INFO = 0x3d,
        OP_SET_GLOBAL_ALPHA = 0x3e,
        OP_SET_LINE_CAP = 0x3f,
        OP_SET_LINE_JOIN = 0x40,
        OP_SET_SHADOW = 0x41,
        OP_BEZIER_CURVE_TO = 0x42,
        OP_QUADRATIC_CURVE_TO = 0x43,
        OP_RECT = 0x44,
        OP_CLIP = 0x45,
        OP_STROKE_TEXT = 0x46,
        OP_SET_TEXT_BASELINE = 0x47,
        OP_SET_GLOBAL_COMPOSITE_OPERATION = 0x48,
        OP_DRAW_IMAGE_SCALED = 0x49,
        OP_DRAW_IMAGE_FULL = 0x4a,
        OP_RESET_TRANSFORM = 0x4b,
        OP_ELLIPSE = 0x4c,
        OP_ARC_TO = 0x4d,
        OP_SET_TRANSFORM = 0x4e,
        OP_TRANSFORM = 0x4f,
        OP_SET_MITER_LIMIT = 0x50,
        OP_SET_IMAGE_SMOOTHING_ENABLED = 0x51,
        OP_MEASURE_TEXT_WIDTH = 0x52,
    };

    extern "C" int32_t webcc_canvas_create_canvas(const char* dom_id, uint32_t dom_id_len, double width, double height);
    inline webcc::Canvas create_canvas(webcc::string_view dom_id, double width, double height){
        ::webcc::flush();
        return webcc::Canvas(webcc_canvas_create_canvas(dom_id.data(), dom_id.length(), width, height));
    }

    extern "C" int32_t webcc_canvas_get_context_2d(int32_t canvas_handle);
    inline webcc::CanvasContext2D get_context_2d(webcc::Canvas canvas_handle){
        ::webcc::flush();
        return webcc::CanvasContext2D(webcc_canvas_get_context_2d((int32_t)canvas_handle));
    }

    extern "C" int32_t webcc_canvas_get_context_webgl(int32_t canvas_handle);
    inline webcc::WebGLContext get_context_webgl(webcc::Canvas canvas_handle){
        ::webcc::flush();
        return webcc::WebGLContext(webcc_canvas_get_context_webgl((int32_t)canvas_handle));
    }

    extern "C" int32_t webcc_canvas_get_context_webgpu(int32_t canvas_handle);
    inline webcc::WGPUContext get_context_webgpu(webcc::Canvas canvas_handle){
        ::webcc::flush();
        return webcc::WGPUContext(webcc_canvas_get_context_webgpu((int32_t)canvas_handle));
    }

    extern "C" void webcc_use_canvas_set_size();
    inline void set_size(webcc::Canvas handle, double width, double height){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_size; (void)webcc_mark;
        push_command((uint32_t)OP_SET_SIZE);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(width);
        push_data<double>(height);
    }

    extern "C" void webcc_use_canvas_set_fill_style();
    inline void set_fill_style(webcc::CanvasContext2D handle, uint8_t r, uint8_t g, uint8_t b){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_fill_style; (void)webcc_mark;
        push_command((uint32_t)OP_SET_FILL_STYLE);
        push_data<int32_t>((int32_t)handle);
        push_data<uint32_t>((uint32_t)r);
        push_data<uint32_t>((uint32_t)g);
        push_data<uint32_t>((uint32_t)b);
    }

    extern "C" void webcc_use_canvas_set_fill_style_str();
    inline void set_fill_style_str(webcc::CanvasContext2D handle, webcc::string_view color){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_fill_style_str; (void)webcc_mark;
        push_command((uint32_t)OP_SET_FILL_STYLE_STR);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(color.data(), color.length());
    }

    extern "C" void webcc_use_canvas_fill_rect();
    inline void fill_rect(webcc::CanvasContext2D handle, double x, double y, double w, double h){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_fill_rect; (void)webcc_mark;
        push_command((uint32_t)OP_FILL_RECT);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(w);
        push_data<double>(h);
    }

    extern "C" void webcc_use_canvas_clear_rect();
    inline void clear_rect(webcc::CanvasContext2D handle, double x, double y, double w, double h){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_clear_rect; (void)webcc_mark;
        push_command((uint32_t)OP_CLEAR_RECT);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(w);
        push_data<double>(h);
    }

    extern "C" void webcc_use_canvas_stroke_rect();
    inline void stroke_rect(webcc::CanvasContext2D handle, double x, double y, double w, double h){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_stroke_rect; (void)webcc_mark;
        push_command((uint32_t)OP_STROKE_RECT);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(w);
        push_data<double>(h);
    }

    extern "C" void webcc_use_canvas_set_stroke_style();
    inline void set_stroke_style(webcc::CanvasContext2D handle, uint8_t r, uint8_t g, uint8_t b){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_stroke_style; (void)webcc_mark;
        push_command((uint32_t)OP_SET_STROKE_STYLE);
        push_data<int32_t>((int32_t)handle);
        push_data<uint32_t>((uint32_t)r);
        push_data<uint32_t>((uint32_t)g);
        push_data<uint32_t>((uint32_t)b);
    }

    extern "C" void webcc_use_canvas_set_stroke_style_str();
    inline void set_stroke_style_str(webcc::CanvasContext2D handle, webcc::string_view color){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_stroke_style_str; (void)webcc_mark;
        push_command((uint32_t)OP_SET_STROKE_STYLE_STR);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(color.data(), color.length());
    }

    extern "C" void webcc_use_canvas_set_line_width();
    inline void set_line_width(webcc::CanvasContext2D handle, double width){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_line_width; (void)webcc_mark;
        push_command((uint32_t)OP_SET_LINE_WIDTH);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(width);
    }

    extern "C" void webcc_use_canvas_begin_path();
    inline void begin_path(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_begin_path; (void)webcc_mark;
        push_command((uint32_t)OP_BEGIN_PATH);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_close_path();
    inline void close_path(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_close_path; (void)webcc_mark;
        push_command((uint32_t)OP_CLOSE_PATH);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_move_to();
    inline void move_to(webcc::CanvasContext2D handle, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_move_to; (void)webcc_mark;
        push_command((uint32_t)OP_MOVE_TO);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_line_to();
    inline void line_to(webcc::CanvasContext2D handle, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_line_to; (void)webcc_mark;
        push_command((uint32_t)OP_LINE_TO);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_stroke();
    inline void stroke(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_stroke; (void)webcc_mark;
        push_command((uint32_t)OP_STROKE);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_fill();
    inline void fill(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_fill; (void)webcc_mark;
        push_command((uint32_t)OP_FILL);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_arc();
    inline void arc(webcc::CanvasContext2D handle, double x, double y, double radius, double start_angle, double end_angle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_arc; (void)webcc_mark;
        push_command((uint32_t)OP_ARC);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(radius);
        push_data<double>(start_angle);
        push_data<double>(end_angle);
    }

    extern "C" void webcc_use_canvas_fill_text();
    inline void fill_text(webcc::CanvasContext2D handle, webcc::string_view text, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_fill_text; (void)webcc_mark;
        push_command((uint32_t)OP_FILL_TEXT);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(text.data(), text.length());
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_fill_text_f();
    inline void fill_text_f(webcc::CanvasContext2D handle, webcc::string_view fmt, double val, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_fill_text_f; (void)webcc_mark;
        push_command((uint32_t)OP_FILL_TEXT_F);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(fmt.data(), fmt.length());
        push_data<double>(val);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_fill_text_i();
    inline void fill_text_i(webcc::CanvasContext2D handle, webcc::string_view fmt, int32_t val, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_fill_text_i; (void)webcc_mark;
        push_command((uint32_t)OP_FILL_TEXT_I);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(fmt.data(), fmt.length());
        push_data<int32_t>(val);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_set_font();
    inline void set_font(webcc::CanvasContext2D handle, webcc::string_view font){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_font; (void)webcc_mark;
        push_command((uint32_t)OP_SET_FONT);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(font.data(), font.length());
    }

    extern "C" void webcc_use_canvas_set_text_align();
    inline void set_text_align(webcc::CanvasContext2D handle, webcc::string_view align){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_text_align; (void)webcc_mark;
        push_command((uint32_t)OP_SET_TEXT_ALIGN);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(align.data(), align.length());
    }

    extern "C" void webcc_use_canvas_draw_image();
    inline void draw_image(webcc::CanvasContext2D handle, webcc::Image img_handle, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_draw_image; (void)webcc_mark;
        push_command((uint32_t)OP_DRAW_IMAGE);
        push_data<int32_t>((int32_t)handle);
        push_data<int32_t>((int32_t)img_handle);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_translate();
    inline void translate(webcc::CanvasContext2D handle, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_translate; (void)webcc_mark;
        push_command((uint32_t)OP_TRANSLATE);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_rotate();
    inline void rotate(webcc::CanvasContext2D handle, double angle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_rotate; (void)webcc_mark;
        push_command((uint32_t)OP_ROTATE);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(angle);
    }

    extern "C" void webcc_use_canvas_scale();
    inline void scale(webcc::CanvasContext2D handle, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_scale; (void)webcc_mark;
        push_command((uint32_t)OP_SCALE);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_save();
    inline void save(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_save; (void)webcc_mark;
        push_command((uint32_t)OP_SAVE);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_restore();
    inline void restore(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_restore; (void)webcc_mark;
        push_command((uint32_t)OP_RESTORE);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_log_canvas_info();
    inline void log_canvas_info(webcc::Canvas handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_log_canvas_info; (void)webcc_mark;
        push_command((uint32_t)OP_LOG_CANVAS_INFO);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_set_global_alpha();
    inline void set_global_alpha(webcc::CanvasContext2D handle, double alpha){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_global_alpha; (void)webcc_mark;
        push_command((uint32_t)OP_SET_GLOBAL_ALPHA);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(alpha);
    }

    extern "C" void webcc_use_canvas_set_line_cap();
    inline void set_line_cap(webcc::CanvasContext2D handle, webcc::string_view cap){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_line_cap; (void)webcc_mark;
        push_command((uint32_t)OP_SET_LINE_CAP);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(cap.data(), cap.length());
    }

    extern "C" void webcc_use_canvas_set_line_join();
    inline void set_line_join(webcc::CanvasContext2D handle, webcc::string_view join){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_line_join; (void)webcc_mark;
        push_command((uint32_t)OP_SET_LINE_JOIN);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(join.data(), join.length());
    }

    extern "C" void webcc_use_canvas_set_shadow();
    inline void set_shadow(webcc::CanvasContext2D handle, double blur, double off_x, double off_y, webcc::string_view color){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_shadow; (void)webcc_mark;
        push_command((uint32_t)OP_SET_SHADOW);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(blur);
        push_data<double>(off_x);
        push_data<double>(off_y);
        webcc::CommandBuffer::push_string(color.data(), color.length());
    }

    extern "C" void webcc_use_canvas_bezier_curve_to();
    inline void bezier_curve_to(webcc::CanvasContext2D handle, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_bezier_curve_to; (void)webcc_mark;
        push_command((uint32_t)OP_BEZIER_CURVE_TO);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(cp1x);
        push_data<double>(cp1y);
        push_data<double>(cp2x);
        push_data<double>(cp2y);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_quadratic_curve_to();
    inline void quadratic_curve_to(webcc::CanvasContext2D handle, double cpx, double cpy, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_quadratic_curve_to; (void)webcc_mark;
        push_command((uint32_t)OP_QUADRATIC_CURVE_TO);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(cpx);
        push_data<double>(cpy);
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_rect();
    inline void rect(webcc::CanvasContext2D handle, double x, double y, double w, double h){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_rect; (void)webcc_mark;
        push_command((uint32_t)OP_RECT);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(w);
        push_data<double>(h);
    }

    extern "C" void webcc_use_canvas_clip();
    inline void clip(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_clip; (void)webcc_mark;
        push_command((uint32_t)OP_CLIP);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_stroke_text();
    inline void stroke_text(webcc::CanvasContext2D handle, webcc::string_view text, double x, double y){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_stroke_text; (void)webcc_mark;
        push_command((uint32_t)OP_STROKE_TEXT);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(text.data(), text.length());
        push_data<double>(x);
        push_data<double>(y);
    }

    extern "C" void webcc_use_canvas_set_text_baseline();
    inline void set_text_baseline(webcc::CanvasContext2D handle, webcc::string_view baseline){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_text_baseline; (void)webcc_mark;
        push_command((uint32_t)OP_SET_TEXT_BASELINE);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(baseline.data(), baseline.length());
    }

    extern "C" void webcc_use_canvas_set_global_composite_operation();
    inline void set_global_composite_operation(webcc::CanvasContext2D handle, webcc::string_view op){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_global_composite_operation; (void)webcc_mark;
        push_command((uint32_t)OP_SET_GLOBAL_COMPOSITE_OPERATION);
        push_data<int32_t>((int32_t)handle);
        webcc::CommandBuffer::push_string(op.data(), op.length());
    }

    extern "C" void webcc_use_canvas_draw_image_scaled();
    inline void draw_image_scaled(webcc::CanvasContext2D handle, webcc::Image img_handle, double x, double y, double w, double h){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_draw_image_scaled; (void)webcc_mark;
        push_command((uint32_t)OP_DRAW_IMAGE_SCALED);
        push_data<int32_t>((int32_t)handle);
        push_data<int32_t>((int32_t)img_handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(w);
        push_data<double>(h);
    }

    extern "C" void webcc_use_canvas_draw_image_full();
    inline void draw_image_full(webcc::CanvasContext2D handle, webcc::Image img_handle, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_draw_image_full; (void)webcc_mark;
        push_command((uint32_t)OP_DRAW_IMAGE_FULL);
        push_data<int32_t>((int32_t)handle);
        push_data<int32_t>((int32_t)img_handle);
        push_data<double>(sx);
        push_data<double>(sy);
        push_data<double>(sw);
        push_data<double>(sh);
        push_data<double>(dx);
        push_data<double>(dy);
        push_data<double>(dw);
        push_data<double>(dh);
    }

    extern "C" void webcc_use_canvas_reset_transform();
    inline void reset_transform(webcc::CanvasContext2D handle){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_reset_transform; (void)webcc_mark;
        push_command((uint32_t)OP_RESET_TRANSFORM);
        push_data<int32_t>((int32_t)handle);
    }

    extern "C" void webcc_use_canvas_ellipse();
    inline void ellipse(webcc::CanvasContext2D handle, double x, double y, double radius_x, double radius_y, double rotation, double start_angle, double end_angle, uint8_t counter_clockwise){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_ellipse; (void)webcc_mark;
        push_command((uint32_t)OP_ELLIPSE);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x);
        push_data<double>(y);
        push_data<double>(radius_x);
        push_data<double>(radius_y);
        push_data<double>(rotation);
        push_data<double>(start_angle);
        push_data<double>(end_angle);
        push_data<uint32_t>((uint32_t)counter_clockwise);
    }

    extern "C" void webcc_use_canvas_arc_to();
    inline void arc_to(webcc::CanvasContext2D handle, double x1, double y1, double x2, double y2, double radius){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_arc_to; (void)webcc_mark;
        push_command((uint32_t)OP_ARC_TO);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(x1);
        push_data<double>(y1);
        push_data<double>(x2);
        push_data<double>(y2);
        push_data<double>(radius);
    }

    extern "C" void webcc_use_canvas_set_transform();
    inline void set_transform(webcc::CanvasContext2D handle, double a, double b, double c, double d, double e, double f){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_transform; (void)webcc_mark;
        push_command((uint32_t)OP_SET_TRANSFORM);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(a);
        push_data<double>(b);
        push_data<double>(c);
        push_data<double>(d);
        push_data<double>(e);
        push_data<double>(f);
    }

    extern "C" void webcc_use_canvas_transform();
    inline void transform(webcc::CanvasContext2D handle, double a, double b, double c, double d, double e, double f){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_transform; (void)webcc_mark;
        push_command((uint32_t)OP_TRANSFORM);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(a);
        push_data<double>(b);
        push_data<double>(c);
        push_data<double>(d);
        push_data<double>(e);
        push_data<double>(f);
    }

    extern "C" void webcc_use_canvas_set_miter_limit();
    inline void set_miter_limit(webcc::CanvasContext2D handle, double limit){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_miter_limit; (void)webcc_mark;
        push_command((uint32_t)OP_SET_MITER_LIMIT);
        push_data<int32_t>((int32_t)handle);
        push_data<double>(limit);
    }

    extern "C" void webcc_use_canvas_set_image_smoothing_enabled();
    inline void set_image_smoothing_enabled(webcc::CanvasContext2D handle, uint8_t enabled){
        static const void* webcc_mark __attribute__((used)) = (const void*)&webcc_use_canvas_set_image_smoothing_enabled; (void)webcc_mark;
        push_command((uint32_t)OP_SET_IMAGE_SMOOTHING_ENABLED);
        push_data<int32_t>((int32_t)handle);
        push_data<uint32_t>((uint32_t)enabled);
    }

    extern "C" double webcc_canvas_measure_text_width(int32_t handle, const char* text, uint32_t text_len);
    inline double measure_text_width(webcc::CanvasContext2D handle, webcc::string_view text){
        ::webcc::flush();
        return webcc_canvas_measure_text_width((int32_t)handle, text.data(), text.length());
    }

} // namespace webcc::canvas
