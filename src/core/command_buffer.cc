#include "command_buffer.h"

namespace webcc {

namespace {
    constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024; // 1MB
    alignas(4) static uint8_t g_buffer[MAX_BUFFER_SIZE];
    static size_t g_offset = 0;

    struct StringCacheEntry {
        const char* ptr;
        size_t len;
        uint16_t id;
    };
    static StringCacheEntry g_string_cache[512];
    static uint16_t g_string_count = 0;
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

void CommandBuffer::push_string(const char* str, size_t len) {
    // Linear search
    for(uint16_t i=0; i<g_string_count; ++i) {
        if (g_string_cache[i].len == len) {
            bool match = true;
            for(size_t j=0; j<len; ++j) {
                if (g_string_cache[i].ptr[j] != str[j]) { match = false; break; }
            }
            if (match) {
                push_u32(0); // Tag: Cached
                push_u32(g_string_cache[i].id);
                return;
            }
        }
    }

    // Not found
    push_u32(1); // Tag: New
    push_u32((uint32_t)len);
    
    if (g_offset + len <= MAX_BUFFER_SIZE) {
        for(size_t k=0; k<len; ++k) g_buffer[g_offset++] = str[k];
    }
    
    // Pad to 4 bytes
    size_t pad = (4 - (len % 4)) % 4;
    for(size_t k=0; k<pad; ++k) {
        if(g_offset < MAX_BUFFER_SIZE) g_buffer[g_offset++] = 0;
    }

    // Add to cache
    if (g_string_count < 512) {
        g_string_cache[g_string_count].ptr = str;
        g_string_cache[g_string_count].len = len;
        g_string_cache[g_string_count].id = g_string_count;
        g_string_count++;
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
    g_string_count = 0;
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


