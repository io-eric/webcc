# Architecture

WebCC works by serializing API calls into a linear memory buffer (the **Command Buffer**). When you call a function like `webcc::canvas::fill_rect`, it writes a compact binary opcode and its arguments to this buffer.

When `webcc::flush()` is called, the buffer is passed to the JavaScript runtime, which decodes the commands and executes the corresponding Web APIs in a tight loop. This batching approach significantly reduces the overhead of crossing the WebAssembly/JavaScript boundary.

> **Note**: Functions that return a value (e.g., `create_element`) are implemented as **direct WASM imports** (synchronous calls). To ensure correct execution order, they automatically trigger a `flush()` before running, ensuring all pending buffered commands are executed first.

## Event System
WebCC uses a secondary shared memory buffer for sending events (like mouse clicks, key presses, or WebSocket messages) from JavaScript to C++.
- **Zero-Copy**: Events are written directly into WASM memory by the JS runtime.
- **Polling**: The C++ application polls this buffer (e.g., once per frame) to process pending events.

## Schema Generation
The toolchain generates `src/cli/webcc_schema.h` which embeds command definitions directly into the binary. This avoids the need to parse `schema.def` at runtime.

The build uses Ninja for incremental compilation:
- A bootstrap compiler is built first (without the schema)
- The bootstrap generates `webcc_schema.h` from `schema.def`
- The final compiler is built with the schema baked in
- Changes to `schema.def` automatically trigger a rebuild

## Compilation & Linking
WebCC acts as a wrapper around `clang++`. It:
1.  **Scans** your code to determine which Web APIs are used.
2.  **Generates** a tree-shaken `app.js` containing only the necessary JS glue code for the features you use.
3.  **Compiles** your C++ code to WebAssembly.
4.  **Caches** compiled object files in a `.webcc_cache` directory (located inside the output directory) to speed up subsequent builds.

## Resource Handles
To maximize performance, WebCC uses **typed integer handles** to reference resources (like DOM elements, Canvases, Audio objects, and WebGL programs).
- **Creation**: Functions like `create_element` return a typed handle (e.g., `webcc::DOMElement`), and `create_canvas` returns a `webcc::Canvas`.
- **Type Safety**: Handle types use C++ template inheritance to allow implicit conversions where appropriate (e.g., `Canvas` can be passed to functions expecting `DOMElement`).
- **Zero Overhead**: The type system is compile-time only, at runtime, handles are simple 32-bit integers with no additional cost.
- **Usage**: Subsequent commands use these integer handles, avoiding expensive string lookups or map queries on the JavaScript side during hot code paths (like rendering loops).

### Deferred Handles
Functions that return handles (like `create_element`) must synchronously call into JavaScript, triggering an automatic `flush()`. This can be expensive when creating many elements.

**Deferred handles** allow C++ to pre-assign a handle before the element is created:

```cpp
// Generate handle on C++ side (no JS call)
webcc::handle h = webcc::next_deferred_handle();

// Buffer the creation command
webcc::dom::create_element_deferred(h, "div");

// Use immediately in other buffered commands
webcc::dom::append_child(parent, webcc::DOMElement(h));

// Single flush creates everything
webcc::flush();
```

Deferred handles start at `0x100000` and increment upward, while JS-assigned handles use lower values, ensuring no collisions. This pattern is essential for high-performance DOM manipulation.

## C++ Standard Library Compatibility
WebCC provides a lightweight compatibility layer for common C++ Standard Library headers (located in `include/webcc/compat/`). 

Standard libraries provided by toolchains like Emscripten or even the default LLVM `libc++` can be quite large, often adding hundreds of kilobytes to the WASM binary. WebCC's compat layer provides minimal, header-only implementations of essential types like `std::vector`, `std::string`, and `std::iostream` (e.g., `std::cout`) that are:
- **Optimized for Size**: They avoid complex features like locales, exceptions, and heavy template nesting.
- **Zero-Dependency**: They build directly on top of WebCC's core types and allocators.
- **WASM-Friendly**: Designed to work efficiently within the WebAssembly environment.

**Note on Binary Size**: While these headers are significantly smaller than a full STL (often adding only a few KB instead of hundreds), they still introduce more overhead than using the raw `webcc::core` types directly. For the absolute smallest binaries, prefer `webcc::string_view` and direct `webcc::system::log` calls.

To use them, simply include the standard header name (e.g., `#include <vector>`) and ensure the `include/webcc/compat` directory is in your include path (which the `webcc` tool handles automatically).

## Easy Extensibility
The entire API surface is defined in a single configuration file: `schema.def`.
- **Format**: `NAMESPACE|TYPE|NAME|FUNC_NAME|ARG_TYPES|JS_ACTION`
- **Generation**: The `webcc` tool parses this file to automatically generate:
  1.  **C++ Headers**: Type-safe function prototypes (e.g., `webcc/canvas.h`).
  2.  **JavaScript Runtime**: The switch-case logic to execute commands in `app.js`.

To add a new Web API feature, simply add a line to `schema.def` and run `./build.sh` to regenerate the toolchain and headers.
