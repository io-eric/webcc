#include "webcc/wgpu.h"
#include "webcc/canvas.h"
#include "webcc/dom.h"
#include "webcc/system.h"

const char* shader_code = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
    var pos = array<vec2<f32>, 3>(
        vec2<f32>(0.0, 0.5),
        vec2<f32>(-0.5, -0.5),
        vec2<f32>(0.5, -0.5)
    );
    return vec4<f32>(pos[in_vertex_index], 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0); // Red
}
)";

webcc::Canvas canvas_handle;
webcc::WGPUContext wgpu_ctx;
webcc::WGPUDevice device;
webcc::WGPUQueue queue;
webcc::WGPURenderPipeline pipeline;
bool ready = false;

float last_time = 0.0f;
float fps = 0.0f;
webcc::Canvas hud_canvas;
webcc::CanvasContext2D hud_ctx;

void update(float time_ms) {
    // Calculate Delta Time (in seconds)
    float delta_time = (time_ms - last_time) / 1000.0f;
    last_time = time_ms;
    
    // Avoid huge delta_time on first frame
    if (delta_time > 0.1f) delta_time = 0.016f;

    // Calculate FPS
    if (delta_time > 0.0f) {
        fps = 1.0f / delta_time;
    }

    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto event = e.as<webcc::wgpu::AdapterReadyEvent>()) {
            webcc::WGPUAdapter handle(event->handle);
            
            if (!handle.is_valid()) {
                webcc::system::error("WebGPU adapter request failed.");
                webcc::DOMElement body = webcc::dom::get_body();
                webcc::DOMElement msg = webcc::dom::create_element("div");
                webcc::dom::set_attribute(msg, "style", "position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); background: rgba(255, 0, 0, 0.8); color: white; padding: 20px; border-radius: 8px; font-family: sans-serif; font-weight: bold; z-index: 1000;");
                webcc::dom::set_inner_text(msg, "WebGPU is not supported or enabled in this browser.");
                webcc::dom::append_child(body, msg);
            } else {
                webcc::system::log("Adapter ready!");
                webcc::wgpu::request_device(handle);
            }
        } else if (auto event = e.as<webcc::wgpu::DeviceReadyEvent>()) {
            webcc::WGPUDevice handle(event->handle);
            
            webcc::system::log("Device ready!");
            device = handle;
            queue = webcc::wgpu::get_queue(device);
            
            wgpu_ctx = webcc::canvas::get_context(canvas_handle, "webgpu");
            webcc::wgpu::configure(wgpu_ctx, device, "preferred");

            webcc::WGPUShaderModule shader_module = webcc::wgpu::create_shader_module(device, shader_code);
                
                // Create a simple pipeline
                pipeline = webcc::wgpu::create_render_pipeline_simple(device, shader_module, shader_module, "vs_main", "fs_main", "preferred");
                
                ready = true;
            ready = true;
        }
    }

    if (ready) {
        webcc::WGPUCommandEncoder encoder = webcc::wgpu::create_command_encoder(device);
        webcc::WGPUTextureView view = webcc::wgpu::get_current_texture_view(wgpu_ctx);
        
        // Clear to dark blue
        webcc::WGPURenderPass pass = webcc::wgpu::begin_render_pass(encoder, view, 0.0f, 0.0f, 0.2f, 1.0f);
        
        webcc::wgpu::set_pipeline(pass, pipeline);
        webcc::wgpu::draw(pass, 3, 1, 0, 0);
        
        webcc::wgpu::end_pass(pass);
        
        webcc::WGPUCommandBuffer cmd_buffer = webcc::wgpu::finish_encoder(encoder);
        webcc::wgpu::queue_submit(queue, cmd_buffer);

        // Draw FPS text via Canvas 2D overlay
        webcc::canvas::clear_rect(hud_ctx, 0, 0, 200, 40);
        webcc::canvas::set_font(hud_ctx, "20px Arial");
        webcc::canvas::set_fill_style(hud_ctx, 0, 255, 0);
        webcc::canvas::fill_text_f(hud_ctx, "FPS: %f", fps, 10, 25);
    }

    webcc::flush();
}

int main() {
    webcc::system::set_title("WebCC WebGPU Demo");
    
    webcc::DOMElement body = webcc::dom::get_body();

    // Style the body to center content
    webcc::dom::set_attribute(body, "style", "margin: 0; height: 100vh; display: flex; justify-content: center; align-items: center; background: #111; color: #eee; font-family: sans-serif;");

    // Create a container for the game
    webcc::DOMElement game_container = webcc::dom::create_element("div");
    webcc::dom::set_attribute(game_container, "style", "position: relative; border: 2px solid #444; box-shadow: 0 0 20px rgba(0,0,0,0.5); display: flex; flex-direction: column; align-items: center; background: #222; padding: 10px;");
    webcc::dom::append_child(body, game_container);

    // Add a title via DOM
    webcc::DOMElement game_title = webcc::dom::create_element("h1");
    webcc::dom::set_inner_text(game_title, "WebCC WebGPU Demo");
    webcc::dom::set_attribute(game_title, "style", "color: #fff; margin: 10px 0; font-family: monospace;");
    webcc::dom::append_child(game_container, game_title);

    // Add some description text
    webcc::DOMElement game_desc = webcc::dom::create_element("p");
    webcc::dom::set_inner_text(game_desc, "This is a Triangle rendered with WebGPU via WebCC.");
    webcc::dom::set_attribute(game_desc, "style", "color: #aaa; margin-bottom: 20px; font-size: 14px;");
    webcc::dom::append_child(game_container, game_desc);

    // HUD canvas overlay for FPS (like canvas example)
    hud_canvas = webcc::canvas::create_canvas("hud-canvas", 800, 600);
    hud_ctx = webcc::canvas::get_context(hud_canvas, "2d");
    webcc::dom::set_attribute(hud_canvas, "style", "position: absolute; left: 0; top: 0; pointer-events: none;");
    webcc::dom::append_child(game_container, hud_canvas);

    webcc::Canvas canvas = webcc::canvas::create_canvas("wgpu-canvas", 600, 600);
    webcc::dom::append_child(game_container, canvas);
    
    canvas_handle = canvas;

    // Start WebGPU initialization
    webcc::wgpu::request_adapter();

    webcc::system::set_main_loop(update);

    webcc::flush();
    
    return 0;
}
