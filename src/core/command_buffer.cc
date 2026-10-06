#include "command_buffer.h"
#include "webcc/core/allocator.h"

#ifdef __wasm__
// Import for WASM
extern "C" void webcc_js_flush(uintptr_t ptr, size_t size);
#else
// Stub for native build (webcc tool); weak so tests can observe flushes
extern "C" __attribute__((weak)) void webcc_js_flush(uintptr_t ptr, size_t size) {}
#endif

namespace webcc {

namespace {
    constexpr size_t STATIC_BUFFER_SIZE = 1024 * 1024; // 1MB
    alignas(8) static uint8_t g_static_buffer[STATIC_BUFFER_SIZE];
    static uint8_t* g_buffer = g_static_buffer;
    static size_t g_capacity = STATIC_BUFFER_SIZE;
    static size_t g_start = 0;     // first pending byte (0 or 4, see make_room)
    static size_t g_offset = 0;
    static size_t g_cmd_start = 0; // start of the command being written
    static bool g_dropped = false; // current command did not fit, drop it

    // Slow path: make space for `n` more bytes while a command is being written.
    __attribute__((noinline)) bool make_room(size_t n) {
        if (g_dropped) return false;

        // Flush the complete commands and move the partial one to the front.
        // Keep its offset mod 8 so double alignment stays valid.
        if (g_cmd_start > g_start) {
            webcc_js_flush(reinterpret_cast<uintptr_t>(g_buffer + g_start), g_cmd_start - g_start);
            size_t partial = g_offset - g_cmd_start;
            g_start = g_cmd_start % 8;
            __builtin_memmove(g_buffer + g_start, g_buffer + g_cmd_start, partial);
            g_cmd_start = g_start;
            g_offset = g_start + partial;
        }
        if (g_offset + n <= g_capacity) return true;

        // A single command larger than the buffer: grow onto the heap.
        size_t cap = g_capacity;
        while (cap < g_offset + n) cap *= 2;
        uint8_t* grown = static_cast<uint8_t*>(webcc::malloc(cap));
        if (!grown) {
            g_dropped = true;
            return false;
        }
        __builtin_memcpy(grown, g_buffer, g_offset);
        if (g_buffer != g_static_buffer) webcc::free(g_buffer);
        g_buffer = grown;
        g_capacity = cap;
        return true;
    }

    // Claim `n` bytes at the write position, nullptr if the command is dropped
    uint8_t* take(size_t n) {
        if (g_offset + n > g_capacity && !make_room(n)) return nullptr;
        uint8_t* p = g_buffer + g_offset;
        g_offset += n;
        return p;
    }

    // Forget a command that could not be buffered
    inline void discard_dropped() {
        if (g_dropped) {
            g_offset = g_cmd_start;
            g_dropped = false;
        }
    }
}

void CommandBuffer::push_command(uint32_t opcode) {
    discard_dropped();
    g_cmd_start = g_offset;
    push_u32(opcode);
}

// Values are stored little-endian (wasm byte order)
void CommandBuffer::push_u32(uint32_t v) {
    if (uint8_t* p = take(4)) __builtin_memcpy(p, &v, 4);
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
    size_t pad = g_offset % 8 ? 4 : 0;
    if (uint8_t* p = take(pad + 8)) {
        __builtin_memset(p, 0, pad);
        __builtin_memcpy(p + pad, &v, 8);
    }
}

void CommandBuffer::push_string(const char* str, size_t len) {
    // Length, data, padding to 4 bytes
    size_t padded = (len + 3) & ~(size_t)3;
    if (uint8_t* p = take(4 + padded)) {
        uint32_t l = (uint32_t)len;
        __builtin_memcpy(p, &l, 4);
        __builtin_memcpy(p + 4, str, len);
        __builtin_memset(p + 4 + len, 0, padded - len);
    }
}

const uint8_t* CommandBuffer::data(){
    discard_dropped();
    return g_buffer + g_start;
}

size_t CommandBuffer::size(){
    discard_dropped();
    return g_offset - g_start;
}

void CommandBuffer::reset(){
    if (g_buffer != g_static_buffer) {
        webcc::free(g_buffer);
        g_buffer = g_static_buffer;
        g_capacity = STATIC_BUFFER_SIZE;
    }
    g_start = 0;
    g_offset = 0;
    g_cmd_start = 0;
    g_dropped = false;
}

} // namespace webcc

namespace webcc {
    void flush() {
        size_t s = CommandBuffer::size();
        if (s == 0) return;
        webcc_js_flush(reinterpret_cast<uintptr_t>(CommandBuffer::data()), s);
        CommandBuffer::reset();
    }
}
