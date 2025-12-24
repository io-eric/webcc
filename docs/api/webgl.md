# WebGL API

The `webcc::webgl` module provides an interface to the WebGL API.

## Header

```cpp
#include "webcc/webgl.h"
```

## Functions

### Context Management

```cpp
void viewport(webcc::handle ctx_handle, int32_t x, int32_t y, int32_t width, int32_t height);
void clear_color(webcc::handle ctx_handle, float r, float g, float b, float a);
void clear(webcc::handle ctx_handle, uint32_t mask);
void enable(webcc::handle ctx_handle, uint32_t cap);
```

### Shaders and Programs

```cpp
webcc::handle create_shader(webcc::handle ctx_handle, uint32_t type, webcc::string_view source);
webcc::handle create_program(webcc::handle ctx_handle);
void attach_shader(webcc::handle ctx_handle, webcc::handle prog_handle, webcc::handle shader_handle);
void link_program(webcc::handle ctx_handle, webcc::handle prog_handle);
void use_program(webcc::handle ctx_handle, webcc::handle prog_handle);
```

### Buffers and Attributes

```cpp
webcc::handle create_buffer(webcc::handle ctx_handle);
void bind_buffer(webcc::handle ctx_handle, uint32_t target, webcc::handle buf_handle);
void buffer_data(webcc::handle ctx_handle, uint32_t target, uint32_t data_ptr, uint32_t data_len, uint32_t usage);
void bind_attrib_location(webcc::handle ctx_handle, webcc::handle prog_handle, uint32_t index, webcc::string_view name);
void enable_vertex_attrib_array(webcc::handle ctx_handle, uint32_t index);
void vertex_attrib_pointer(webcc::handle ctx_handle, uint32_t index, int32_t size, uint32_t type, uint8_t normalized, int32_t stride, int32_t offset);
```

### Uniforms

```cpp
webcc::handle get_uniform_location(webcc::handle ctx_handle, webcc::handle prog_handle, webcc::string_view name);
void uniform_1f(webcc::handle ctx_handle, webcc::handle loc_handle, float val);
```

### Drawing

```cpp
void draw_arrays(webcc::handle ctx_handle, uint32_t mode, int32_t first, int32_t count);
```
