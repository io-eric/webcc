#include "command_buffer.h"

namespace webcc {

namespace {
    constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024; // 1MB
    alignas(8) static uint8_t g_buffer[MAX_BUFFER_SIZE];
    static size_t g_offset = 0;
}

void CommandBuffer::push_u32(uint32_t v) {
    if (g_offset + 4 <= MAX_BUFFER_SIZE) {
        g_buffer[g_offset++] = v & 0xFF;
        g_buffer[g_offset++] = (v >> 8) & 0xFF;
        g_buffer[g_offset++] = (v >> 16) & 0xFF;
        g_buffer[g_offset++] = (v >> 24) & 0xFF;
    }
}

void CommandBuffer::push_i32(int32_t v) {
    push_u32(static_cast<uint32_t>(v));
}

void CommandBuffer::push_float(float v) {
    uint32_t u;
    __builtin_memcpy(&u, &v, 4);
    push_u32(u);
}

void CommandBuffer::push_double(double v) {
    // Align to 8 bytes before writing double
    if (g_offset % 8 != 0) {
        size_t pad = 8 - (g_offset % 8);
        for(size_t k=0; k<pad; ++k) {
            if(g_offset < MAX_BUFFER_SIZE) g_buffer[g_offset++] = 0;
        }
    }
    uint64_t u;
    __builtin_memcpy(&u, &v, 8);
    // Push as two 32-bit values (little-endian)
    push_u32(u & 0xFFFFFFFF);
    push_u32(u >> 32);
}

void CommandBuffer::push_string(const char* str, size_t len) {
    push_u32((uint32_t)len);
    
    if (g_offset + len <= MAX_BUFFER_SIZE) {
        for(size_t k=0; k<len; ++k) g_buffer[g_offset++] = str[k];
    }
    
    // Pad to 4 bytes
    size_t pad = (4 - (len % 4)) % 4;
    for(size_t k=0; k<pad; ++k) {
        if(g_offset < MAX_BUFFER_SIZE) g_buffer[g_offset++] = 0;
    }
}

const uint8_t* CommandBuffer::data(){
    return g_buffer;
}

size_t CommandBuffer::size(){
    return g_offset;
}

void CommandBuffer::reset(){
    g_offset = 0;
}

} // namespace webcc

#ifdef __wasm__
// Import for WASM
extern "C" void webcc_js_flush(uintptr_t ptr, size_t size);
#else
// Stub for native build (webcc tool)
extern "C" void webcc_js_flush(uintptr_t ptr, size_t size) {}
#endif

namespace webcc {
    void flush() {
        size_t s = CommandBuffer::size();
        if (s == 0) return;
        webcc_js_flush(reinterpret_cast<uintptr_t>(CommandBuffer::data()), s);
        CommandBuffer::reset();
    }
}


