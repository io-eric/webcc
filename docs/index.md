# WebCC Documentation

Welcome to the documentation for **WebCC**, a lightweight, zero-dependency C++ toolchain and framework for building WebAssembly applications.

## Overview

WebCC provides a direct, high-performance bridge between C++ and HTML5 APIs. It is designed to be minimal and efficient, generating small WASM binaries without the need for heavy runtimes like Emscripten for simple use cases.

### Key Features

- **Zero Dependencies**: No external libraries required.
- **Lightweight**: Generates minimal WASM binaries.
- **High Performance**: Uses a binary command buffer to batch API calls.
- **Comprehensive API**: Supports DOM, Canvas 2D, WebGL, WebGPU, Audio, Input, and more.

## Table of Contents

- [Getting Started](getting_started.md): Installation, setup, and your first "Hello World" application.
- API Reference: Detailed documentation for all WebCC modules.
    - [Canvas](api/canvas.md)
    - [DOM](api/dom.md)
    - [WebGL](api/webgl.md)
    - [WebGPU](api/wgpu.md)
    - [Audio](api/audio.md)
    - [Input](api/input.md)
    - [System](api/system.md)
    - [Fetch](api/fetch.md)
    - [Image](api/image.md)
    - [Storage](api/storage.md)
    - [WebSocket](api/websocket.md)
- [Architecture](architecture.md): Learn how WebCC works under the hood.
