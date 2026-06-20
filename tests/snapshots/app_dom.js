
const supportsStreaming = () => {
    try {
        if (typeof WebAssembly === 'undefined') return false;
        if (typeof WebAssembly.instantiateStreaming !== 'function') return false;
        
        return !/^((?!chrome|android).)*safari/i.test(navigator.userAgent) ||
               parseInt(navigator.userAgent.match(/version\/(\d+)/i)?.[1] || 0) >= 15;
    } catch { return false; }
};

const run = async () => {
    const scriptSrc = document.currentScript && document.currentScript.src;
    const assetBase = new URL('.', scriptSrc || window.location.href);
    const wasmUrl = new URL('app.wasm', assetBase);

    const _wm = () => {}; // shared no-op for void-command feature markers (never called)
    const imports = {
        env: {
            // C++ calls this function to tell JS "I wrote commands, please execute them"
            webcc_js_flush: (ptr, size) => flush(ptr, size),
            // C++ runtime
            __cxa_atexit: () => 0,
            __cxa_thread_atexit: () => 0,
            __cxa_finalize: () => {}
,
            webcc_dom_get_body: () => {
                if(!elements[0]) elements[0] = document.body; return 0;
            }
,
            webcc_dom_create_element: (tag_ptr, tag_len) => {
                const tag = decoder.decode(new Uint8Array(memory.buffer, tag_ptr, tag_len));
                const handle = (window.webcc_next_id = (window.webcc_next_id || 0) + 1); const el = document.createElement(tag); elements[handle] = el; return handle;
            }

        },
        w: new Proxy({}, { get: () => _wm })
    };

    let mod;
    if (supportsStreaming()) {
        mod = await WebAssembly.instantiateStreaming(fetch(wasmUrl), imports);
    } else {
        const response = await fetch(wasmUrl);
        const bytes = await response.arrayBuffer();
        mod = await WebAssembly.instantiate(bytes, imports);
    }
    const { memory, main, __indirect_function_table: table, webcc_event_buffer_ptr, webcc_event_offset_ptr, webcc_event_buffer_capacity, webcc_scratch_buffer_ptr } = mod.instance.exports;

    const event_buffer_ptr_val = webcc_event_buffer_ptr();
    const event_offset_ptr_val = webcc_event_offset_ptr();
    const scratch_buffer_ptr_val = webcc_scratch_buffer_ptr();
    let event_offset_view = new Uint32Array(memory.buffer, event_offset_ptr_val, 1);
    let event_u8 = new Uint8Array(memory.buffer, event_buffer_ptr_val);
    let event_i32 = new Int32Array(memory.buffer, event_buffer_ptr_val);
    let event_f32 = new Float32Array(memory.buffer, event_buffer_ptr_val);
    let event_f64 = new Float64Array(memory.buffer, event_buffer_ptr_val);
    const text_encoder = new TextEncoder();
    const EVENT_BUFFER_SIZE = webcc_event_buffer_capacity();

    // Global update function reference for immediate discrete event processing
    let _updateFn = null;
    let _updatePending = false;
    function _triggerDiscreteUpdate() {
        if (_updateFn && !_updatePending) {
            _updatePending = true;
            queueMicrotask(() => { _updatePending = false; _updateFn(performance.now()); });
        }
    }

    const elements = []; elements[0] = document.body;

    // Reusable text decoder to avoid garbage collection overhead
    const decoder = new TextDecoder();
    let u8 = new Uint8Array(memory.buffer);
    let i32 = new Int32Array(memory.buffer);
    let f32 = new Float32Array(memory.buffer);
    let f64 = new Float64Array(memory.buffer);

    function flush(ptr, size) {
        if (size === 0) return;

        if (u8.buffer !== memory.buffer) {
            u8 = new Uint8Array(memory.buffer);
            i32 = new Int32Array(memory.buffer);
            f32 = new Float32Array(memory.buffer);
            f64 = new Float64Array(memory.buffer);
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
                case 15: {
                    if (pos + 4 > end) { console.error('WebCC: OOB parent_handle'); break; }
                    const parent_handle = i32[pos >> 2]; pos += 4;
                    if (pos + 4 > end) { console.error('WebCC: OOB child_handle'); break; }
                    const child_handle = i32[pos >> 2]; pos += 4;
                    { const parent = elements[parent_handle]; const child = elements[child_handle]; if(!parent || !child){ console.warn('append_child: unknown handles', parent_handle, child_handle); continue; } parent.appendChild(child); }
                    break;
                }
                case 20: {
                    if (pos + 4 > end) { console.error('WebCC: OOB handle'); break; }
                    const handle = i32[pos >> 2]; pos += 4;
                    if (pos + 4 > end) { console.error('WebCC: OOB text_len'); break; }
                    const text_len = i32[pos >> 2]; pos += 4;
                    const text_padded = (text_len + 3) & ~3;
                    if (pos + text_padded > end) { console.error('WebCC: OOB text_data'); break; }
                    const text = decoder.decode(u8.subarray(pos, pos + text_len)); pos += text_padded;
                    { const el = elements[handle]; if(el) el.innerText = text; }
                    break;
                }

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
