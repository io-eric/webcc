# WebGL API

The `webcc::webgl` module provides an interface to the WebGL API.

## Header

```cpp
#include "webcc/webgl.h"
```

## Functions

### Context Management

```cpp
void viewport(webcc::WebGLContext ctx, int32_t x, int32_t y, int32_t width, int32_t height);
void clear_color(webcc::WebGLContext ctx, float r, float g, float b, float a);
void clear(webcc::WebGLContext ctx, uint32_t mask);
void enable(webcc::WebGLContext ctx, uint32_t cap);
```

### Shaders and Programs

```cpp
webcc::WebGLShader create_shader(webcc::WebGLContext ctx, uint32_t type, webcc::string_view source);
webcc::WebGLProgram create_program(webcc::WebGLContext ctx);
void attach_shader(webcc::WebGLContext ctx, webcc::WebGLProgram prog, webcc::WebGLShader shader);
void link_program(webcc::WebGLContext ctx, webcc::WebGLProgram prog);
void use_program(webcc::WebGLContext ctx, webcc::WebGLProgram prog);
```

### Buffers and Attributes

```cpp
webcc::WebGLBuffer create_buffer(webcc::WebGLContext ctx);
void bind_buffer(webcc::WebGLContext ctx, uint32_t target, webcc::WebGLBuffer buf);
void buffer_data(webcc::WebGLContext ctx, uint32_t target, uint32_t data_ptr, uint32_t data_len, uint32_t usage);
void bind_attrib_location(webcc::WebGLContext ctx, webcc::WebGLProgram prog, uint32_t index, webcc::string_view name);
void enable_vertex_attrib_array(webcc::WebGLContext ctx, uint32_t index);
void vertex_attrib_pointer(webcc::WebGLContext ctx, uint32_t index, int32_t size, uint32_t type, uint8_t normalized, int32_t stride, int32_t offset);
```

### Uniforms

```cpp
webcc::WebGLUniform get_uniform_location(webcc::WebGLContext ctx, webcc::WebGLProgram prog, webcc::string_view name);
void uniform_1f(webcc::WebGLContext ctx, webcc::WebGLUniform loc, float val);
```

### Drawing

```cpp
void draw_arrays(webcc::WebGLContext ctx, uint32_t mode, int32_t first, int32_t count);
```
