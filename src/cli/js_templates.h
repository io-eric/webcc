#pragma once
#include <string>

namespace webcc
{

    // JS code to initialize the WebAssembly module and set up the environment.
    const std::string JS_INIT_HEAD = R"(
const supportsStreaming = () => {
    try {
        if (typeof WebAssembly === 'undefined') return false;
        if (typeof WebAssembly.instantiateStreaming !== 'function') return false;
        
        return !/^((?!chrome|android).)*safari/i.test(navigator.userAgent) ||
               parseInt(navigator.userAgent.match(/version\/(\d+)/i)?.[1] || 0) >= 15;
    } catch { return false; }
};

const run = async () => {
    const imports = {
        env: {
            // C++ calls this function to tell JS "I wrote commands, please execute them"
            webcc_js_flush: (ptr, size) => flush(ptr, size),
            // C++ runtime
            __cxa_atexit: () => 0,
            __cxa_thread_atexit: () => 0,
            __cxa_finalize: () => {}
)";

    // JS code to finalize WASM instantiation.
    // Note: The exports destructuring is now generated dynamically based on what's needed.
    const std::string JS_INIT_TAIL = R"(
        }
    };

    let mod;
    if (supportsStreaming()) {
        mod = await WebAssembly.instantiateStreaming(fetch('app.wasm'), imports);
    } else {
        const response = await fetch('app.wasm');
        const bytes = await response.arrayBuffer();
        mod = await WebAssembly.instantiate(bytes, imports);
    }
)";

    // JS code for the 'flush' function, which processes commands from C++.
    const std::string JS_FLUSH_HEAD = R"(
    // Reusable text decoder to avoid garbage collection overhead
    const decoder = new TextDecoder();
    let u8 = new Uint8Array(memory.buffer);
    let i32 = new Int32Array(memory.buffer);
    let f32 = new Float32Array(memory.buffer);

    function flush(ptr, size) {
        if (size === 0) return;

        if (u8.buffer !== memory.buffer) {
            u8 = new Uint8Array(memory.buffer);
            i32 = new Int32Array(memory.buffer);
            f32 = new Float32Array(memory.buffer);
        }

        let pos = ptr;
        const end = ptr + size;

        // Loop through the buffer
        while (pos < end) {
            if (pos + 4 > end) {
                console.error("WebCC: Unexpected end of buffer reading opcode");
                break;
            }
            const opcode = i32[pos >> 2];
            pos += 4;

            switch (opcode) {
)";

    // The constant "footer" for the generated JS file.
    const std::string JS_TAIL = R"(
                default:
                    console.error("Unknown opcode:", opcode);
                    return;
            }
        }
    }

    // Run the C++ main function
    if (main) main();
};
run();
)";

} // namespace webcc
