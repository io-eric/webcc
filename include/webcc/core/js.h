// Inline JavaScript escape hatch: the WEBCC_JS macro.
//
// WEBCC_JS lets you define a named function whose body is raw JavaScript, for
// the rare cases the schema does not cover yet. It is the analog of Emscripten's
// EM_JS, built the WebCC way: the function's whole source travels as a wasm
// import name, so detection and tree-shaking come for free (an unused function
// leaves no import and costs nothing), and dispatch is just the wasm import
// resolution.
//
// You declare it once (a header is fine) and call it like any C++ function.
// Arguments are passed by name through the normal wasm ABI, so the JS body
// refers to them by name (no $0/$1):
//
//     WEBCC_JS(void, set_title, (const char* title), {
//         document.title = title;
//     });
//     WEBCC_JS(int, js_add, (int a, int b), { return a + b; });
//
//     set_title("Hello");          // called like a normal C++ function
//     int s = js_add(2, 3);
//
// Calls are SYNCHRONOUS and flush() the command buffer first (exactly like
// webcc's other return-value calls), so the body observes the effects of any
// preceding batched commands.
//
// SUPPORTED TYPES (v1):
//   * Parameters: numeric (int/unsigned/float/double and integer handles) arrive
//     as JS numbers; `const char*` is auto-decoded to a JS string (read
//     NUL-terminated, so string literals and webcc::string work).
//   * Return: void / int / float / double. For a string *return*, write the
//     result back yourself (e.g. via the scratch buffer) for now.
//   * Use block comments inside the body, not //, since the source is collapsed
//     onto one line.
//
// BACK FROM JAVASCRIPT: a body may call push_event_js_MESSAGE(channel, text),
// right away or later (from a promise, a listener), and the program reads it
// with poll_event as webcc::js::MessageEvent {channel, data}, like any event;
// the program is woken for it. channel is any number the caller picks to tell
// messages apart:
//
//     WEBCC_JS(void, watch_online, (int channel), {
//         addEventListener('online', () => push_event_js_MESSAGE(channel, 'on'));
//     });
#pragma once

#include <stdint.h>

namespace webcc
{
    // Declared in webcc.h; re-declared here so this header is self-contained.
    void flush();
}

// `name(params){body}` is carried verbatim as the import name (the source itself
// is the channel). A forwarding wrapper adds the same auto-flush() that webcc's
// other return-value calls perform, and keeps the call site looking like a plain
// C++ function.
#define WEBCC_JS(ret, name, params, ...)                                               \
    extern "C" __attribute__((import_module("wjs_fn"),                                  \
                              import_name(#name #params #__VA_ARGS__)))                 \
    ret __webcc_js_imp_##name params;                                                   \
    template <class... __WjsArgs>                                                       \
    static inline ret name(__WjsArgs... __wjs_args)                                     \
    {                                                                                   \
        ::webcc::flush();                                                               \
        return __webcc_js_imp_##name(__wjs_args...);                                    \
    }                                                                                   \
    static_assert(true, "require trailing semicolon")
