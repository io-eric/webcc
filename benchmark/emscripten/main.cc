#include <emscripten.h>
#include <emscripten/val.h>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <cstdio>

using namespace emscripten;

// Benchmark settings
constexpr int RECT_COUNT = 10000;
constexpr int BENCHMARK_FRAMES = 500;

val ctx = val::undefined();
int frame_count = 0;
int total_frames = 0;
double last_time = 0;
double start_time = -1.0;

struct Rect {
    float x, y, w, h;
    std::string color; // Pre-compute color string for emscripten to avoid string formatting in loop
};

std::vector<Rect> rects;

static unsigned long int next = 1;
int my_rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

void init_rects() {
    rects.reserve(RECT_COUNT);
    for (int i = 0; i < RECT_COUNT; ++i) {
        char color[32];
        sprintf(color, "rgb(%d,%d,%d)", my_rand() % 255, my_rand() % 255, my_rand() % 255);
        rects.push_back({
            static_cast<float>(my_rand() % 800),
            static_cast<float>(my_rand() % 600),
            static_cast<float>(my_rand() % 50 + 10),
            static_cast<float>(my_rand() % 50 + 10),
            std::string(color)
        });
    }
}

void update(void* user_data) {
    double time_ms = emscripten_get_now();
    if (start_time < 0) start_time = time_ms;

    // Calculate FPS
    if (time_ms - last_time >= 1000.0) {
        std::cout << "FPS: " << frame_count << std::endl;
        frame_count = 0;
        last_time = time_ms;
    }
    frame_count++;
    total_frames++;

    if (total_frames >= BENCHMARK_FRAMES) {
        double duration_sec = (time_ms - start_time) / 1000.0;
        double avg_fps = total_frames / duration_sec;
        
        val performance = val::global("performance");
        double mem = 0;
        if (performance["memory"].as<bool>()) {
            mem = performance["memory"]["usedJSHeapSize"].as<double>() / 1024.0 / 1024.0;
        }

        val heap = val::module_property("HEAP8")["buffer"]["byteLength"];
        double wasm_mem = heap.as<double>() / 1024.0 / 1024.0;

        val data = val::object();
        data.set("name", std::string("emscripten"));
        data.set("fps", avg_fps);
        data.set("browser", val::global("navigator")["userAgent"]);
        data.set("memory_used_mb", mem);
        data.set("wasm_heap_mb", wasm_mem);

        val fetch_opts = val::object();
        fetch_opts.set("method", std::string("POST"));
        fetch_opts.set("body", val::global("JSON").call<val>("stringify", data));

        // Use window.fetch instead of fetch.fetch
        val::global("window").call<val>("fetch", std::string("/report"), fetch_opts)
            .call<val>("then", val::module_property("closeWindow")); // Call helper to close/clear

        std::cout << "Benchmark complete. Report sent." << std::endl;
        emscripten_cancel_main_loop();
        return;
    }

    // Clear screen
    ctx.set("fillStyle", val("white"));
    ctx.call<void>("fillRect", 0, 0, 800, 600);

    // Draw Rects
    for (const auto& r : rects) {
        ctx.set("fillStyle", val(r.color));
        ctx.call<void>("fillRect", r.x, r.y, r.w, r.h);
    }
}

int main() {
    val document = val::global("document");
    val canvas = document.call<val>("createElement", std::string("canvas"));
    canvas.set("width", 800);
    canvas.set("height", 600);
    document.call<val>("getElementById", std::string("body"));
    document.call<val>("querySelector", std::string("body")).call<void>("appendChild", canvas);

    ctx = canvas.call<val>("getContext", std::string("2d"));

    init_rects();

    std::cout << "Starting Emscripten Benchmark with " << RECT_COUNT << " rects..." << std::endl;

    emscripten_set_main_loop_arg(update, nullptr, 0, 1);

    return 0;
}
