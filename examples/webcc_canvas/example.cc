#include "webcc/canvas.h"
#include "webcc/dom.h"
#include "webcc/system.h"
#include "webcc/input.h"

// Global state
float rotation = 0.0f;
int mouse_x = 0.0f;
int mouse_y = 0.0f;
bool is_clicking = false;

// Canvas Handles
webcc::Canvas demo_canvas;
webcc::Canvas hud_canvas;
webcc::CanvasContext2D demo_ctx;
webcc::CanvasContext2D hud_ctx;

// FPS counting
float last_time = 0.0f;
float fps = 0.0f;

void update(float time_ms) {
    // Poll events
    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto event = e.as<webcc::input::MouseMoveEvent>()) {
            mouse_x = event->x;
            mouse_y = event->y;
        } else if (e.opcode == webcc::input::EVENT_MOUSE_DOWN) {
            is_clicking = true;
        } else if (e.opcode == webcc::input::EVENT_MOUSE_UP) {
            is_clicking = false;
        }
    }

    // Calculate Delta Time (in seconds)
    float delta_time = (time_ms - last_time) / 1000.0f;
    last_time = time_ms;
    
    // Avoid huge delta_time on first frame
    if (delta_time > 0.1f) delta_time = 0.016f;

    // Calculate FPS
    if (delta_time > 0.0f) {
        fps = 1.0f / delta_time;
    }

    // Clear background
    webcc::canvas::set_fill_style(demo_ctx, 20, 20, 30);
    webcc::canvas::fill_rect(demo_ctx, 0, 0, 600, 600);

    // Draw rotating square at center
    webcc::canvas::save(demo_ctx);
    webcc::canvas::translate(demo_ctx, 300, 300);
    webcc::canvas::rotate(demo_ctx, rotation);
    
    if (is_clicking) {
        webcc::canvas::set_fill_style(demo_ctx, 255, 50, 50);
        webcc::canvas::set_shadow(demo_ctx, 20, 0, 0, "red");
    } else {
        webcc::canvas::set_fill_style(demo_ctx, 100, 200, 255);
        webcc::canvas::set_shadow(demo_ctx, 10, 5, 5, "rgba(0,0,0,0.5)");
    }
    
    webcc::canvas::fill_rect(demo_ctx, -50, -50, 100, 100);
    webcc::canvas::restore(demo_ctx);

    // Draw circle at mouse position
    webcc::canvas::begin_path(demo_ctx);
    webcc::canvas::arc(demo_ctx, mouse_x, mouse_y, 10, 0, 6.28f);
    webcc::canvas::set_fill_style(demo_ctx, 255, 255, 0);
    webcc::canvas::fill(demo_ctx);

    // Draw FPS via HUD overlay 
    webcc::canvas::clear_rect(hud_ctx, 0, 0, 200, 40);
    webcc::canvas::set_font(hud_ctx, "20px Arial");
    webcc::canvas::set_fill_style(hud_ctx, 0, 255, 0);
    webcc::canvas::fill_text_f(hud_ctx, "FPS: %f", fps, 10, 25);

    // Rotate based on delta_time (e.g., 1 radian per second)
    rotation += 1.0f * delta_time;

    webcc::flush();
}

int main() {
    webcc::system::set_title("WebCC Canvas Demo");
    
    webcc::DOMElement body = webcc::dom::get_body();

    // Style the body to center content
    webcc::dom::set_attribute(body, "style", "margin: 0; height: 100vh; display: flex; justify-content: center; align-items: center; background: #111; color: #eee; font-family: sans-serif;");

    // Create a container for the game
    webcc::DOMElement game_container = webcc::dom::create_element("div");
    webcc::dom::set_attribute(game_container, "style", "position: relative; border: 2px solid #444; box-shadow: 0 0 20px rgba(0,0,0,0.5); display: flex; flex-direction: column; align-items: center; background: #222; padding: 10px;");
    webcc::dom::append_child(body, game_container);

    // Add a title via DOM
    webcc::DOMElement game_title = webcc::dom::create_element("h1");
    webcc::dom::set_inner_text(game_title, "WebCC Canvas Demo");
    webcc::dom::set_attribute(game_title, "style", "color: #fff; margin: 10px 0; font-family: monospace;");
    webcc::dom::append_child(game_container, game_title);

    // Add some description text
    webcc::DOMElement game_desc = webcc::dom::create_element("p");
    webcc::dom::set_inner_text(game_desc, "This text is a DOM element created from C++. The canvas below is Canvas 2D.");
    webcc::dom::set_attribute(game_desc, "style", "color: #aaa; margin-bottom: 20px; font-size: 14px;");
    webcc::dom::append_child(game_container, game_desc);

    // Create main render canvas and append to container
    demo_canvas = webcc::canvas::create_canvas("demo-canvas", 600, 600);
    demo_ctx = webcc::canvas::get_context_2d(demo_canvas);
    webcc::dom::append_child(game_container, demo_canvas);

    // HUD canvas overlay for FPS (to match WebGL example)
    hud_canvas = webcc::canvas::create_canvas("hud-canvas", 600, 600);
    hud_ctx = webcc::canvas::get_context_2d(hud_canvas);
    webcc::dom::set_attribute(hud_canvas, "style", "position: absolute; left: 0; top: 0; pointer-events: none;");
    webcc::dom::append_child(game_container, hud_canvas);
    
    // Initialize input listeners on the canvas so we get correct offsets
    webcc::dom::init_mouse(demo_canvas);

    webcc::system::log("Starting demo loop...");
    webcc::system::set_main_loop(update);
    
    webcc::flush();
    return 0;
}
