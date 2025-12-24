# WebGPU API

The `webcc::wgpu` module provides an interface to the WebGPU API.

> **Note:** This API is experimental and subject to change.

## Header

```cpp
#include "webcc/wgpu.h"
```

## Initialization

WebGPU initialization is asynchronous.

```cpp
void request_adapter();
void request_device(webcc::handle adapter_handle);
```

You must listen for `AdapterReadyEvent` and `DeviceReadyEvent` to proceed.

## Functions

### Device & Queue

```cpp
webcc::handle get_queue(webcc::handle device_handle);
void configure(webcc::handle context_handle, webcc::handle device_handle, webcc::string_view format);
```

### Shaders & Pipelines

```cpp
webcc::handle create_shader_module(webcc::handle device_handle, webcc::string_view code);
webcc::handle create_render_pipeline_simple(webcc::handle device_handle, webcc::handle vs_module_handle, webcc::handle fs_module_handle, webcc::string_view vs_entry, webcc::string_view fs_entry, webcc::string_view format);
```

### Command Encoding

```cpp
webcc::handle create_command_encoder(webcc::handle device_handle);
webcc::handle finish_encoder(webcc::handle encoder_handle);
void queue_submit(webcc::handle queue_handle, webcc::handle command_buffer_handle);
```

### Render Pass

```cpp
webcc::handle get_current_texture_view(webcc::handle context_handle);
webcc::handle begin_render_pass(webcc::handle encoder_handle, webcc::handle view_handle, float r, float g, float b, float a);
void end_pass(webcc::handle pass_handle);
```

### Drawing

```cpp
void set_pipeline(webcc::handle pass_handle, webcc::handle pipeline_handle);
void draw(webcc::handle pass_handle, int32_t vertex_count, int32_t instance_count, int32_t first_vertex, int32_t first_instance);
```
