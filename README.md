<p align="center">
  <img src="docs/images/logo.png" alt="WebCC Logo" width="400">
</p>

# WebCC

**WebCC** is a lightweight, zero-dependency C++ framework for building WebAssembly applications. It provides a direct, high-performance bridge between C++ and HTML5 APIs (DOM, Canvas, WebGL, WebGPU, Audio, ...).

## Features

- Generates minimal WASM binaries and glue code.
- No heavy runtimes or external libraries required.
- Uses a binary command buffer to batch API calls, minimizing C++/JS boundary overhead.
- Supports DOM, Canvas 2D, WebGL, WebGPU, Audio, Input, WebSockets, and more.
- A single CLI tool handles code generation and compilation.
- Incremental compilation using `.webcc_cache` for faster rebuilds.

## Quick Start

Here is a complete example of creating a Canvas, handling mouse input, and running a game loop:

```cpp
#include "webcc/canvas.h"
#include "webcc/dom.h"
#include "webcc/system.h"
#include "webcc/input.h"

// Global state
int ctx = 0;
int mouse_x = 400.0f;
int mouse_y = 300.0f;

// Main loop function called every frame
void update(float time_ms) {
    // Poll events
    uint8_t opcode;
    const uint8_t* data;
    uint32_t len;
    while (webcc::poll_event(opcode, &data, len)) {
        if (opcode == webcc::input::EVENT_MOUSE_MOVE) {
            // Parse event data safely
            auto event = webcc::parse_event<webcc::input::MouseMoveEvent>(data, len);
            mouse_x = event.x;
            mouse_y = event.y;
        }
    }

    // Clear background (Blue)
    webcc::canvas::set_fill_style(ctx, 52, 152, 219);
    webcc::canvas::fill_rect(ctx, 0, 0, 800, 600);

    // Draw circle at mouse position (Yellow)
    webcc::canvas::begin_path(ctx);
    webcc::canvas::arc(ctx, mouse_x, mouse_y, 50, 0, 6.28318f);
    webcc::canvas::set_fill_style(ctx, 241, 196, 15);
    webcc::canvas::fill(ctx);

    // Draw text
    webcc::canvas::set_font(ctx, "30px Arial");
    webcc::canvas::set_fill_style(ctx, 255, 255, 255);
    webcc::canvas::fill_text(ctx, "Move your mouse!", 280, 500);

    // Flush commands to JS
    webcc::flush();
}

int main() {
    // Setup DOM
    int body = webcc::dom::get_body();
    int canvas = webcc::canvas::create_canvas("game-canvas", 800, 600);
    webcc::dom::append_child(body, canvas);
    
    // Get Context
    ctx = webcc::canvas::get_context(canvas, "2d");
    
    // Initialize mouse input on the canvas
    webcc::input::init_mouse(canvas);

    // Start the main loop
    webcc::system::set_main_loop(update);
    
    // Flush commands to JS
    webcc::flush();
    
    return 0;
}
```

### Building & Running

1.  **Build the toolchain** (first time only):
    Bootstraps the `webcc` compiler. This script compiles a bootstrap version of the tool, generates the API headers from the schema, and then compiles the final `webcc` binary with the schema embedded.
    ```bash
    ./build.sh
    ```

2.  **Compile your app**:
    ```bash
    ./webcc main.cc
    ```

3.  **Run**:
    ```bash
    python3 -m http.server
    ```
    Open [http://localhost:8000](http://localhost:8000).

## CLI Reference

The `webcc` tool is your primary interface for the framework.

### 1. Generate Headers
Generates the C++ header files in `include/webcc/` from `schema.def`.
> **Note**: This is used internally by `build.sh`. If you modify `schema.def`, you should run `./build.sh` to rebuild the tool so that the embedded schema matches your changes.
```bash
./webcc --headers [schema.def]
```

### 2. Compile Application
Compiles your C++ source files into `app.wasm`, and generates the optimized `app.js` and `index.html`.
Use the `--out <dir>` flag to specify the output directory (defaults to the current directory).
```bash
./webcc main.cc [other_sources.cc ...] [--out dist]
```

## Examples

Check the `examples/` directory for complete demos.

### 1. Canvas 2D (`webcc_canvas`)
Interactive 2D graphics with mouse tracking.

<img src="docs/images/canvas_demo.gif" width="400" alt="Canvas Demo">

### 2. WebGL 3D (`webcc_webgl`)
A rotating 3D cube using raw WebGL calls.

<img src="docs/images/webgl_demo.gif" width="400" alt="WebGL Demo">

### 3. WebGPU (`webcc_webgpu`)
A triangle rendered using the WebGPU API.

### 4. DOM Manipulation (`webcc_dom`)
Creating and styling HTML elements from C++.

<img src="docs/images/dom_demo.gif" width="400" alt="DOM Demo">

## Installation

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/io-eric/webcc.git
    cd webcc
    ```

2.  **Prerequisites**:
    - Linux, macOS, or Windows (via WSL) with Bash.
    - `clang++` (version 8+ recommended) for compiling WASM.
    - A C++20 compiler for building the CLI tool.

## Architecture

WebCC works by serializing API calls into a linear memory buffer (the **Command Buffer**). When you call a function like `webcc::canvas::fill_rect`, it writes a compact binary opcode and its arguments to this buffer.

When `webcc::flush()` is called, the buffer is passed to the JavaScript runtime, which decodes the commands and executes the corresponding Web APIs in a tight loop. This batching approach significantly reduces the overhead of crossing the WebAssembly/JavaScript boundary.

> **Note**: Functions that return a value (e.g., `create_element`) are implemented as **direct WASM imports** (synchronous calls). To ensure correct execution order, they automatically trigger a `flush()` before running, ensuring all pending buffered commands are executed first.

### Event System
WebCC uses a secondary shared memory buffer for sending events (like mouse clicks, key presses, or WebSocket messages) from JavaScript to C++.
- **Zero-Copy**: Events are written directly into WASM memory by the JS runtime.
- **Polling**: The C++ application polls this buffer (e.g., once per frame) to process pending events.

### Schema Generation
The toolchain generates `src/cli/webcc_schema.h` which embeds command definitions directly into the binary. This avoids the need to parse `schema.def` at runtime.

### Compilation & Linking
WebCC acts as a wrapper around `clang++`. It:
1.  **Scans** your code to determine which Web APIs are used.
2.  **Generates** a tree-shaken `app.js` containing only the necessary JS glue code for the features you use.
3.  **Compiles** your C++ code to WebAssembly.
4.  **Caches** compiled object files in a `.webcc_cache` directory (located inside the output directory) to speed up subsequent builds.

### Resource Handles vs. Strings
To maximize performance, WebCC uses **integer handles** to reference resources (like DOM elements, Canvases, Audio objects, and WebGL programs).
- **Creation**: Functions like `create_element` or `create_canvas` return a unique `int` handle.
- **Usage**: Subsequent commands use this integer handle, avoiding expensive string lookups or map queries on the JavaScript side during hot code paths (like rendering loops).
- **Strings**: Strings are still used where necessary (e.g., setting text content, colors, or font styles).
  - **Per-Frame Deduplication**: WebCC implements a smart string cache that resets every frame. If you use the same string (e.g., setting "red" color for 50 different objects) multiple times in a single frame, the string data is only sent across the WASM boundary **once**. Subsequent uses send a tiny 2-byte ID, significantly reducing bandwidth for repetitive text rendering.

### Easy Extensibility
The entire API surface is defined in a single configuration file: `schema.def`.
- **Format**: `NAMESPACE|TYPE|NAME|FUNC_NAME|ARG_TYPES|JS_ACTION`
- **Generation**: The `webcc` tool parses this file to automatically generate:
  1.  **C++ Headers**: Type-safe function prototypes (e.g., `webcc/canvas.h`).
  2.  **JavaScript Runtime**: The switch-case logic to execute commands in `app.js`.

To add a new Web API feature, simply add a line to `schema.def` and run `./build.sh` to regenerate the toolchain and headers.

## Contributing ✅

- **Contributions welcome.** If you'd like to add a command, update `schema.def` following the file format and run `./build.sh` to regenerate the toolchain.
- **Small PRs are best.** Include a short example (or a unit test) demonstrating the new API and a brief description in the PR.
- **Tips:** Prefer returning integer handles for created resources (use `RET:int32`), register DOM/audio/image objects in the `elements` map when appropriate, and ensure your JS implementation is robust (checks for missing handles, etc.).
****

## Modules

- **`webcc/dom.h`**: DOM manipulation (create, append, remove, attributes).
- **`webcc/canvas.h`**: HTML5 Canvas 2D context.
- **`webcc/webgl.h`**: WebGL context.
- **`webcc/wgpu.h`**: WebGPU context.
- **`webcc/audio.h`**: Audio playback and control.
- **`webcc/input.h`**: Mouse and keyboard input.
- **`webcc/system.h`**: System utilities.
- **`webcc/websocket.h`**: WebSocket communication.
- **`webcc/storage.h`**: Local storage.
- **`webcc/image.h`**: Image loading.

## License

[MIT](LICENSE)
