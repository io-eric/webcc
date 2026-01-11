# Getting Started with WebCC

This guide will help you set up WebCC and build your first WebAssembly application.

## Prerequisites

Before you begin, ensure you have the following installed:
- Linux, macOS, or Windows (via WSL) with Bash.
- `clang++` (version 8+ recommended) for compiling WASM.
- A C++20 capable compiler (e.g., GCC 10+ or Clang 10+) for building the CLI tool.
- A web server for testing (e.g., Python 3).

## Installation

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/io-eric/webcc.git
    cd webcc
    ```

2.  **Build the toolchain:**
    Run the build script to bootstrap the `webcc` compiler.
    ```bash
    ./build.sh
    ```
    This uses Ninja for incremental builds. The script will:
    - Build a bootstrap compiler (without baked-in schema)
    - Generate `webcc_schema.h` from `schema.def`
    - Build the final compiler with the schema baked in
    
    On subsequent runs, it only rebuilds if `schema.def` changes.
    
    The script will also offer to install `webcc` to your system PATH.

    **Build options:**
    ```bash
    ./build.sh          # Incremental build (only rebuilds if schema.def changed)
    ./build.sh --force  # Force full rebuild
    ```

## Your First Application

Let's create a simple "Hello World" application that manipulates the DOM.

### 1. Create the C++ file

Create a file named `main.cc` with the following content:

```cpp
#include "webcc/dom.h"

int main() {
    // Get a handle to the document body
    webcc::DOMElement body = webcc::dom::get_body();

    // Create a new heading element
    webcc::DOMElement h1 = webcc::dom::create_element("h1");
    webcc::dom::set_inner_text(h1, "Hello from WebCC!");

    // Append the heading to the body
    webcc::dom::append_child(body, h1);

    // Flush commands to JS
    webcc::flush();

    return 0;
}
```

### 2. Compile the application

Use the `webcc` tool to compile your C++ code into WebAssembly.

```bash
webcc main.cc --out dist
```
(Use `./webcc` if you didn't install it to your PATH).

This command will generate the following in the `dist` directory:
- `app.wasm`: The compiled WebAssembly binary.
- `app.js`: The JavaScript glue code.
- `index.html`: A basic HTML file to run the application.
- `.webcc_cache/`: A directory containing cached object files for faster rebuilds.

### Custom HTML Template (Optional)

You can customize the generated `index.html` by creating a template file named `index.template.html` in your project directory:

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>My WebCC App</title>
    <meta name="description" content="My awesome WebAssembly app">
    <link rel="stylesheet" href="styles.css">
</head>
<body>
{{script}}
</body>
</html>
```

WebCC will replace `{{script}}` with the appropriate script tag. If the placeholder is omitted, the script is injected before `</body>`.

### 3. Run the application

Start a local web server to serve your files.

```bash
cd dist
python3 -m http.server 8000
```

Open your browser and navigate to `http://localhost:8000`. You should see "Hello from WebCC!" displayed on the page.

## Using the C++ Standard Library

WebCC includes a lightweight compatibility layer for common C++ Standard Library headers. This allows you to use familiar types like `std::vector`, `std::string`, and `std::cout` while keeping the binary size a fraction of what it would be with a full STL implementation.

Example using `std::cout`:
```cpp
#include <iostream>
#include "webcc/dom.h"

int main() {
    std::cout << "Initializing application..." << std::endl;
    // ...
    return 0;
}
```

The `webcc` tool automatically configures the include paths to use these optimized headers. Note that for the absolute minimum binary size, you can still use the underlying `webcc::` core types directly.

## Next Steps

- Explore the [API Reference](index.md) to learn about available modules.
- Check out the `examples/` directory in the repository for more complex examples.
