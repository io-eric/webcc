<p align="center">
  <img src="docs/images/logo.png" alt="WebCC Logo" width="250">
</p>

# WebCC

**WebCC** is a lightweight, zero-dependency C++ framework for building WebAssembly applications. It provides a direct, high-performance bridge between C++ and HTML5 APIs (DOM, Canvas, WebGL, Audio, ...) without the overhead of heavy runtimes like Emscripten.

## Features

- **🚀 Lightweight**: Generates minimal WASM binaries and glue code.
- **⚡ Fast**: Uses a binary command buffer to batch API calls, minimizing the C++/JS boundary overhead.
- **🌐 Web APIs**: First-class support for DOM, Canvas 2D, WebGL, Audio, Input, WebSockets, and more.
- **🛠️ Simple Toolchain**: A single CLI tool handles code generation and compilation.

## Quick Start

Here is a complete example of creating a Canvas and drawing to it from C++:

```cpp
#include "webcc/canvas.h"
#include "webcc/dom.h"
#include "webcc/system.h"

int main() {
    // Get the document body handle
    int body = webcc::dom::get_body();

    // Create a canvas element (800x600)
    // Returns an integer handle for fast access
    int canvas = webcc::canvas::create_canvas("game-canvas", 800, 600);
    
    // Append the canvas to the body
    webcc::dom::append_child(body, canvas);

    // Draw a blue background
    webcc::canvas::set_fill_style(canvas, 52, 152, 219); // RGB
    webcc::canvas::fill_rect(canvas, 0, 0, 800, 600);

    // Draw a yellow circle in the center
    webcc::canvas::begin_path(canvas);
    webcc::canvas::arc(canvas, 400, 300, 50, 0, 6.28318f);
    webcc::canvas::set_fill_style(canvas, 241, 196, 15);
    webcc::canvas::fill(canvas);

    // Draw some text
    webcc::canvas::set_font(canvas, "30px Arial");
    webcc::canvas::set_fill_style(canvas, 255, 255, 255);
    webcc::canvas::fill_text(canvas, "Hello WebCC!", 310, 500);

    // Flush commands to JS
    webcc::flush();
    
    return 0;
}
```

### Building & Running

1.  **Build the toolchain** (first time only):
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

## Examples

Check the `examples/` directory for complete demos.

### 1. Canvas 2D (`webcc_canvas`)
Interactive 2D graphics with mouse tracking.

<img src="docs/images/canvas_demo.gif" width="400" alt="Canvas Demo">

### 2. WebGL 3D (`webcc_webgl`)
A rotating 3D cube using raw WebGL calls.

<img src="docs/images/webgl_demo.gif" width="400" alt="WebGL Demo">

### 3. DOM Manipulation (`webcc_dom`)
Creating and styling HTML elements from C++.

<img src="docs/images/dom_demo.gif" width="400" alt="DOM Demo">

## Installation

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/io-eric/webcc.git
    cd webcc
    ```

2.  **Prerequisites**:
    - Linux/macOS with Bash.
    - `clang++` (version 8+ recommended) for compiling WASM.
    - A C++17 compiler for building the CLI tool.

## Architecture

WebCC works by serializing API calls into a linear memory buffer (the **Command Buffer**). When you call a function like `webcc::canvas::fill_rect`, it writes a compact binary opcode and its arguments to this buffer.

When `webcc::flush()` is called, the buffer is passed to the JavaScript runtime, which decodes the commands and executes the corresponding Web APIs in a tight loop. This batching approach significantly reduces the overhead of crossing the WebAssembly/JavaScript boundary.

### Resource Handles vs. Strings
To maximize performance, WebCC uses **integer handles** to reference resources (like DOM elements, Canvases, Audio objects, and WebGL programs).
- **Creation**: Functions like `create_element` or `create_canvas` return a unique `int` handle.
- **Usage**: Subsequent commands use this integer handle, avoiding expensive string lookups or map queries on the JavaScript side during hot code paths (like rendering loops).
- **Strings**: Strings are still used where necessary (e.g., setting text content, colors, or font styles).
  - **Per-Frame Deduplication**: WebCC implements a smart string cache that resets every frame. If you use the same string (e.g., setting "red" color for 50 different objects) multiple times in a single frame, the string data is only sent across the WASM boundary **once**. Subsequent uses send a tiny 2-byte ID, significantly reducing bandwidth for repetitive text rendering.

### Easy Extensibility
The entire API surface is defined in a single configuration file: `commands.def`.
- **Format**: `NAMESPACE|COMMAND_NAME|func_name|ARG_TYPES|JS_IMPLEMENTATION`
- **Generation**: The `webcc` tool parses this file to automatically generate:
  1.  **C++ Headers**: Type-safe function prototypes (e.g., `webcc/canvas.h`).
  2.  **JavaScript Runtime**: The switch-case logic to execute commands in `app.js`.

To add a new Web API feature, you simply add one line to `commands.def` and rebuild!

## Contributing ✅

- **Contributions welcome.** If you'd like to add a command, update `commands.def` following the file format and run `./build.sh` to regenerate headers and `app.js`.
- **Small PRs are best.** Include a short example (or a unit test) demonstrating the new API and a brief description in the PR.
- **Tips:** Prefer returning integer handles for created resources (use `RET:int32`), register DOM/audio/image objects in the `elements` map when appropriate, and ensure your JS implementation is robust (checks for missing handles, etc.).
****

## Modules

- **`webcc/dom.h`**: DOM manipulation (create, append, remove, attributes).
- **`webcc/canvas.h`**: HTML5 Canvas 2D context.
- **`webcc/webgl.h`**: WebGL context.
- **`webcc/audio.h`**: Audio playback and control.
- **`webcc/input.h`**: Mouse and keyboard input.
- **`webcc/system.h`**: System utilities.
- **`webcc/websocket.h`**: WebSocket communication.
- **`webcc/storage.h`**: Local storage.
- **`webcc/image.h`**: Image loading.

## License

[MIT](LICENSE)
