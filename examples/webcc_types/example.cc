#include "webcc/dom.h"
#include "webcc/canvas.h"
#include "webcc/system.h"
#include "webcc/input.h"
#include "webcc/core/string.h"
#include "webcc/core/string_view.h"
#include "webcc/core/array.h"
#include "webcc/core/optional.h"
#include "webcc/core/unique_ptr.h"
#include "webcc/core/delegate.h"
#include "webcc/core/allocator.h"

// Global handles
webcc::handle log_container;
webcc::handle canvas_ctx;
webcc::handle button_string;
webcc::handle button_array;
webcc::handle button_unique;
webcc::handle button_optional;
webcc::handle button_delegate;
webcc::handle button_custom;
webcc::handle button_allocate;
webcc::handle button_clear;

// Helper to log to DOM
void log(const char* msg) {
    webcc::handle p = webcc::dom::create_element("div");
    webcc::dom::set_inner_text(p, msg);
    webcc::dom::set_attribute(p, "style", "font-family: monospace; margin: 2px 0; color: #ddd; border-bottom: 1px solid #333; padding: 2px;");
    webcc::dom::append_child(log_container, p);
    webcc::system::log(msg);
}

// Heap visualization
void draw_heap_viz() {
    if (!canvas_ctx.is_valid()) return;

    uintptr_t base = (uintptr_t)&webcc::__heap_base;
    uintptr_t current = webcc::heap_ptr;
    size_t used = current - base;
    
    // Get current memory size (in pages)
    size_t current_pages = __builtin_wasm_memory_size(0);
    size_t total_mem = current_pages * 64 * 1024;
    
    // Clear canvas
    webcc::canvas::set_fill_style(canvas_ctx, 30, 30, 30);
    webcc::canvas::fill_rect(canvas_ctx, 0, 0, 600, 150);

    // Draw total memory bar background
    webcc::canvas::set_fill_style(canvas_ctx, 50, 50, 50);
    webcc::canvas::fill_rect(canvas_ctx, 10, 40, 580, 40);

    // Draw used memory
    float percentage = (float)used / (float)total_mem;
    float width = 580.0f * percentage;
    
    // Color changes based on usage
    if (percentage < 0.5f) webcc::canvas::set_fill_style(canvas_ctx, 0, 200, 100);
    else if (percentage < 0.8f) webcc::canvas::set_fill_style(canvas_ctx, 200, 200, 0);
    else webcc::canvas::set_fill_style(canvas_ctx, 200, 50, 50);

    webcc::canvas::fill_rect(canvas_ctx, 10, 40, width, 40);

    // Text info
    webcc::canvas::set_font(canvas_ctx, "16px monospace");
    webcc::canvas::set_fill_style(canvas_ctx, 255, 255, 255);
    
    webcc::string text1 = webcc::string::concat("Heap Usage: ", (unsigned int)used, " / ", (unsigned int)total_mem, " bytes (", webcc::precision(percentage * 100.0f, 1), "%)");
    webcc::canvas::fill_text(canvas_ctx, text1.c_str(), 10, 25);

    webcc::string text2 = webcc::string::concat("Base: ", webcc::hex((unsigned int)base), "  Current: ", webcc::hex((unsigned int)current));
    webcc::canvas::fill_text(canvas_ctx, text2.c_str(), 10, 100);
    
    webcc::string text3 = webcc::string::concat("Pages: ", (unsigned int)current_pages, " (64KB each)");
    webcc::canvas::fill_text(canvas_ctx, text3.c_str(), 10, 125);
}

// --- Demos ---

void demo_string() {
    log("--- String & StringView Demo ---");
    
    webcc::string s = "Hello WebCC";
    log(s.c_str());
    
    webcc::string_view sv = s;
    if (sv.length() > 5) {
        log("String view length > 5");
    }
    
    // Move constructor
    webcc::string s2 = webcc::move(s);
    if (s.length() == 0) {
        log("Original string moved (empty now)");
    }
    log(s2.c_str());
    
    // Variadic concat demo
    webcc::string concatenated = webcc::string::concat("Hello", " ", "WebCC", " ", "with", " ", "variadic", " ", "concat", "!");
    log(concatenated.c_str());
}

void demo_array() {
    log("--- Array Demo ---");
    
    webcc::array<int, 5> arr;
    for(int i=0; i<5; ++i) arr[i] = (i + 1) * 11;
    
    webcc::string msg1 = webcc::string::concat("Array[2] = ", arr[2]);
    log(msg1.c_str());
    
    int sum = 0;
    for(auto& x : arr) sum += x;
    
    webcc::string msg2 = webcc::string::concat("Sum of array: ", sum);
    log(msg2.c_str());
}

void demo_unique_ptr() {
    log("--- Unique Ptr Demo ---");
    
    {
        auto ptr = webcc::make_unique<int>(12345);
        if (ptr) {
            webcc::string msg = webcc::string::concat("Unique ptr value: ", *ptr);
            log(msg.c_str());
        }
        // ptr goes out of scope here
    }
    log("Unique ptr scope ended (destructor called).");
}

void demo_optional() {
    log("--- Optional Demo ---");
    
    webcc::optional<int> opt;
    if (!opt) log("Optional is empty initially.");
    
    opt.construct(999);
    if (opt) {
        webcc::string msg = webcc::string::concat("Optional has value: ", *opt);
        log(msg.c_str());
    }
    
    opt.reset();
    if (!opt) log("Optional reset.");
}

struct DelegateHandler {
    int multiplier;
    void on_event(int val) {
        webcc::string msg = webcc::string::concat("Delegate called with ", val, ". Result: ", val * multiplier);
        log(msg.c_str());
    }
};

void demo_delegate() {
    log("--- Delegate Demo ---");
    
    DelegateHandler h;
    h.multiplier = 2;
    
    auto d = webcc::delegate<int>::from<DelegateHandler, &DelegateHandler::on_event>(&h);
    d(42);
    
    h.multiplier = 10;
    d(5);
}

void update(float time_ms) {
    // Poll events
    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto event = e.as<webcc::dom::ClickEvent>()) {
            if (event->handle == button_string) {
                demo_string();
            } else if (event->handle == button_array) {
                demo_array();
            } else if (event->handle == button_unique) {
                demo_unique_ptr();
            } else if (event->handle == button_optional) {
                demo_optional();
            } else if (event->handle == button_delegate) {
                demo_delegate();
            } else if (event->handle == button_custom) {
                log("--- Custom Concat Demo ---");
                webcc::string custom = webcc::string::concat("Custom", " ", "Concat", " ", "Example", "!");
                    log(custom.c_str());
            } else if (event->handle == button_allocate) {
                log("--- Manual Memory Allocation ---");
                webcc::malloc(1024 * 32);
                webcc::string msg = webcc::string::concat("Allocated 32KB chunk");
                log(msg.c_str());
            } else if (event->handle == button_clear) {
                webcc::dom::set_inner_html(log_container, "");
            }
            draw_heap_viz();
        }
    }
    webcc::flush();
}

// Main loop not needed for this static demo, but we can use it to update heap viz if we were allocating dynamically over time.
// For now, we just run everything in main.

int main() {
    webcc::system::set_title("WebCC Types Demo");
    
    webcc::handle body = webcc::dom::get_body();
    webcc::dom::set_attribute(body, "style", "background: #1e1e1e; color: #eee; font-family: 'Segoe UI', sans-serif; padding: 20px; max-width: 800px; margin: 0 auto;");

    webcc::handle h1 = webcc::dom::create_element("h1");
    webcc::dom::set_inner_text(h1, "WebCC Types & Allocator");
    webcc::dom::set_attribute(h1, "style", "border-bottom: 2px solid #4caf50; padding-bottom: 10px;");
    webcc::dom::append_child(body, h1);

    webcc::handle desc = webcc::dom::create_element("p");
    webcc::dom::set_inner_text(desc, "This example demonstrates the custom C++ types (string, array, unique_ptr, optional, delegate) and visualizes the bump allocator heap usage. Click the buttons on the left to run individual demos.");
    webcc::dom::append_child(body, desc);

    // Canvas for heap viz
    webcc::handle canvas_container = webcc::dom::create_element("div");
    webcc::dom::set_attribute(canvas_container, "style", "margin: 20px 0; border: 1px solid #555; box-shadow: 0 4px 8px rgba(0,0,0,0.3);");
    webcc::dom::append_child(body, canvas_container);

    webcc::handle canvas = webcc::canvas::create_canvas("heap-viz", 600, 150);
    canvas_ctx = webcc::canvas::get_context(canvas, "2d");
    webcc::dom::append_child(canvas_container, canvas);
    
    // Log container
    log_container = webcc::dom::create_element("div");
    webcc::dom::set_attribute(log_container, "style", "background: #252526; padding: 10px; border-radius: 4px; border: 1px solid #333; height: 400px; overflow-y: auto;");
    webcc::dom::append_child(body, log_container);

    // Buttons
    button_string = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_string, "String Demo");
    webcc::dom::set_attribute(button_string, "style", "position: absolute; left: 20px; top: 200px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #4CAF50; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_string);

    button_array = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_array, "Array Demo");
    webcc::dom::set_attribute(button_array, "style", "position: absolute; left: 20px; top: 230px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #4CAF50; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_array);

    button_unique = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_unique, "UniquePtr Demo");
    webcc::dom::set_attribute(button_unique, "style", "position: absolute; left: 20px; top: 260px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #4CAF50; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_unique);

    button_optional = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_optional, "Optional Demo");
    webcc::dom::set_attribute(button_optional, "style", "position: absolute; left: 20px; top: 290px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #4CAF50; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_optional);

    button_delegate = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_delegate, "Delegate Demo");
    webcc::dom::set_attribute(button_delegate, "style", "position: absolute; left: 20px; top: 320px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #4CAF50; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_delegate);

    button_custom = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_custom, "Custom Concat");
    webcc::dom::set_attribute(button_custom, "style", "position: absolute; left: 20px; top: 350px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #2196F3; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_custom);

    button_allocate = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_allocate, "Allocate 32KB");
    webcc::dom::set_attribute(button_allocate, "style", "position: absolute; left: 20px; top: 380px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #FF9800; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_allocate);

    button_clear = webcc::dom::create_element("button");
    webcc::dom::set_inner_text(button_clear, "Clear Log");
    webcc::dom::set_attribute(button_clear, "style", "position: absolute; left: 20px; top: 410px; padding: 5px 10px; font-size: 14px; cursor: pointer; background: #f44336; color: white; border: none; border-radius: 3px;");
    webcc::dom::append_child(body, button_clear);

    // Initial state
    draw_heap_viz();

    // Run demos
    // We'll use a small delay or just run them. Since it's single threaded, it will just happen.
    // To see the heap grow, we can allocate some stuff.
    
    demo_string();
    draw_heap_viz();
    
    demo_array();
    draw_heap_viz();
    
    demo_unique_ptr();
    draw_heap_viz();
    
    demo_optional();
    draw_heap_viz();
    
    demo_delegate();
    draw_heap_viz();
    
    // Allocate some extra memory to show growth
    log("--- Allocating extra memory ---");
    for(int i=0; i<5; ++i) {
        webcc::malloc(1024 * 32); // 32KB
        webcc::string msg = webcc::string::concat("Allocated 32KB chunk ", i+1);
        log(msg.c_str());
        draw_heap_viz();
    }

    // Init inputs - Use the new Click Listener system
    webcc::dom::add_click_listener(button_string);
    webcc::dom::add_click_listener(button_array);
    webcc::dom::add_click_listener(button_unique);
    webcc::dom::add_click_listener(button_optional);
    webcc::dom::add_click_listener(button_delegate);
    webcc::dom::add_click_listener(button_custom);
    webcc::dom::add_click_listener(button_allocate);
    webcc::dom::add_click_listener(button_clear);

    webcc::system::set_main_loop(update);

    webcc::flush();
    return 0;
}
