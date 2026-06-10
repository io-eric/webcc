// GENERATED FILE - DO NOT EDIT
#pragma once
#include "webcc/core/handle.h"

namespace webcc {

    // Type-safe handle types auto-generated from schema.def
    // Each type is a distinct compile-time type wrapping int32_t

    // Tag struct for CanvasContext2D
    struct CanvasContext2D_tag {};
    using CanvasContext2D = typed_handle<CanvasContext2D_tag>;

    // Tag struct for DOMElement
    struct DOMElement_tag {};
    using DOMElement = typed_handle<DOMElement_tag>;

    // Tag struct for FetchRequest
    struct FetchRequest_tag {};
    using FetchRequest = typed_handle<FetchRequest_tag>;

    // Tag struct for Image
    struct Image_tag : DOMElement_tag {};
    using Image = typed_handle<Image_tag>;

    // Tag struct for WGPUAdapter
    struct WGPUAdapter_tag {};
    using WGPUAdapter = typed_handle<WGPUAdapter_tag>;

    // Tag struct for WGPUCommandBuffer
    struct WGPUCommandBuffer_tag {};
    using WGPUCommandBuffer = typed_handle<WGPUCommandBuffer_tag>;

    // Tag struct for WGPUCommandEncoder
    struct WGPUCommandEncoder_tag {};
    using WGPUCommandEncoder = typed_handle<WGPUCommandEncoder_tag>;

    // Tag struct for WGPUContext
    struct WGPUContext_tag {};
    using WGPUContext = typed_handle<WGPUContext_tag>;

    // Tag struct for WGPUDevice
    struct WGPUDevice_tag {};
    using WGPUDevice = typed_handle<WGPUDevice_tag>;

    // Tag struct for WGPUQueue
    struct WGPUQueue_tag {};
    using WGPUQueue = typed_handle<WGPUQueue_tag>;

    // Tag struct for WGPURenderPass
    struct WGPURenderPass_tag {};
    using WGPURenderPass = typed_handle<WGPURenderPass_tag>;

    // Tag struct for WGPURenderPipeline
    struct WGPURenderPipeline_tag {};
    using WGPURenderPipeline = typed_handle<WGPURenderPipeline_tag>;

    // Tag struct for WGPUShaderModule
    struct WGPUShaderModule_tag {};
    using WGPUShaderModule = typed_handle<WGPUShaderModule_tag>;

    // Tag struct for WGPUTextureView
    struct WGPUTextureView_tag {};
    using WGPUTextureView = typed_handle<WGPUTextureView_tag>;

    // Tag struct for WebGLBuffer
    struct WebGLBuffer_tag {};
    using WebGLBuffer = typed_handle<WebGLBuffer_tag>;

    // Tag struct for WebGLContext
    struct WebGLContext_tag {};
    using WebGLContext = typed_handle<WebGLContext_tag>;

    // Tag struct for WebGLProgram
    struct WebGLProgram_tag {};
    using WebGLProgram = typed_handle<WebGLProgram_tag>;

    // Tag struct for WebGLShader
    struct WebGLShader_tag {};
    using WebGLShader = typed_handle<WebGLShader_tag>;

    // Tag struct for WebGLUniform
    struct WebGLUniform_tag {};
    using WebGLUniform = typed_handle<WebGLUniform_tag>;

    // Tag struct for WebSocket
    struct WebSocket_tag {};
    using WebSocket = typed_handle<WebSocket_tag>;

    // Tag struct for Audio
    struct Audio_tag : DOMElement_tag {};
    using Audio = typed_handle<Audio_tag>;

    // Tag struct for Canvas
    struct Canvas_tag : DOMElement_tag {};
    using Canvas = typed_handle<Canvas_tag>;

} // namespace webcc
