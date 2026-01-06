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
void request_device(webcc::WGPUAdapter adapter);
```

You must listen for `AdapterReadyEvent` and `DeviceReadyEvent` to proceed.

## Functions

### Device & Queue

```cpp
webcc::WGPUQueue get_queue(webcc::WGPUDevice device);
void configure(webcc::WGPUContext context, webcc::WGPUDevice device, webcc::string_view format);
```

### Shaders & Pipelines

```cpp
webcc::WGPUShaderModule create_shader_module(webcc::WGPUDevice device, webcc::string_view code);
webcc::WGPURenderPipeline create_render_pipeline_simple(webcc::WGPUDevice device, webcc::WGPUShaderModule vs_module, webcc::WGPUShaderModule fs_module, webcc::string_view vs_entry, webcc::string_view fs_entry, webcc::string_view format);
```

### Command Encoding

```cpp
webcc::WGPUCommandEncoder create_command_encoder(webcc::WGPUDevice device);
webcc::WGPUCommandBuffer finish_encoder(webcc::WGPUCommandEncoder encoder);
void queue_submit(webcc::WGPUQueue queue, webcc::WGPUCommandBuffer command_buffer);
```

### Render Pass

```cpp
webcc::WGPUTextureView get_current_texture_view(webcc::WGPUContext context);
webcc::WGPURenderPass begin_render_pass(webcc::WGPUCommandEncoder encoder, webcc::WGPUTextureView view, float r, float g, float b, float a);
void end_pass(webcc::WGPURenderPass pass);
```

### Drawing

```cpp
void set_pipeline(webcc::WGPURenderPass pass, webcc::WGPURenderPipeline pipeline);
void draw(webcc::WGPURenderPass pass, int32_t vertex_count, int32_t instance_count, int32_t first_vertex, int32_t first_instance);
```
