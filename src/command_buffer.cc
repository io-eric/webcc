#include "command_buffer.h"

namespace webcc {

namespace {
    constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024; // 1MB
    static uint8_t g_buffer[MAX_BUFFER_SIZE];
    static size_t g_offset = 0;

    struct StringCacheEntry {
        const char* ptr;
        size_t len;
        uint16_t id;
    };
    static StringCacheEntry g_string_cache[512];
    static uint16_t g_string_count = 0;
}

void CommandBuffer::push_byte(uint8_t b){
    if (g_offset < MAX_BUFFER_SIZE) {
        g_buffer[g_offset++] = b;
    }
}

void CommandBuffer::push_bytes(const uint8_t* data, size_t len){
    if(!data || len==0) return;
    if (g_offset + len <= MAX_BUFFER_SIZE) {
        for(size_t i=0; i<len; ++i) {
            g_buffer[g_offset++] = data[i];
        }
    }
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
                push_byte(0); // Tag: Cached
                push_byte(g_string_cache[i].id & 0xFF);
                push_byte(g_string_cache[i].id >> 8);
                return;
            }
        }
    }

    // Not found
    push_byte(1); // Tag: New
    push_byte(len & 0xFF);
    push_byte(len >> 8);
    push_bytes((const uint8_t*)str, len);

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


