#include "webcc/canvas.h"
#include "webcc/dom.h"
#include "webcc/system.h"
#include "webcc/core/format.h"

// Benchmark settings
constexpr int RECT_COUNT = 10000;
constexpr int BENCHMARK_FRAMES = 500;

webcc::CanvasContext2D ctx;
int frame_count = 0;
int total_frames = 0;
double last_time = 0;
double start_time = -1.0;

struct Rect {
    float x, y, w, h;
    int r, g, b;
};

Rect rects[RECT_COUNT];

static unsigned long int next = 1;
int my_rand(void) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

void init_rects() {
    for (int i = 0; i < RECT_COUNT; ++i) {
        rects[i] = {
            static_cast<float>(my_rand() % 800),
            static_cast<float>(my_rand() % 600),
            static_cast<float>(my_rand() % 50 + 10),
            static_cast<float>(my_rand() % 50 + 10),
            my_rand() % 255,
            my_rand() % 255,
            my_rand() % 255
        };
    }
}

void update(float time_ms) {
    if (start_time < 0) start_time = time_ms;

    // Calculate FPS
    if (time_ms - last_time >= 1000.0) {
        webcc::formatter<64> fmt;
        fmt << "FPS: " << frame_count;
        webcc::system::log(fmt.c_str());
        frame_count = 0;
        last_time = time_ms;
    }
    frame_count++;
    total_frames++;

    // Check if benchmark is done
    if (total_frames >= BENCHMARK_FRAMES) {
        double duration_sec = (time_ms - start_time) / 1000.0;
        double avg_fps = total_frames / duration_sec;

        size_t current_pages = __builtin_wasm_memory_size(0);
        double wasm_mem = (current_pages * 64.0 * 1024.0) / 1024.0 / 1024.0;

        webcc::formatter<512> js;
        js << "const mem = performance.memory ? performance.memory.usedJSHeapSize / 1024 / 1024 : 0;"
           << "fetch('/report', {"
           << "  method: 'POST',"
           << "  body: JSON.stringify({"
           << "    name: 'webcc',"
           << "    fps: " << avg_fps << ","
           << "    browser: navigator.userAgent,"
           << "    memory_used_mb: mem,"
           << "    wasm_heap_mb: " << wasm_mem
           << "  })"
           << "}).then(() => {"
           << "  document.body.innerHTML = '<h1 style=\"color:white\">Benchmark Complete</h1>';"
           << "  window.close();"
           << "});";

        webcc::handle script = webcc::dom::create_element("script");
        webcc::dom::set_inner_text(script, js.c_str());
        webcc::dom::append_child(webcc::dom::get_body(), script);

        webcc::system::log("Benchmark complete. Report sent.");
        webcc::system::set_main_loop(nullptr); // Stop loop
        webcc::flush();
        return;
    }

    // Clear screen
    webcc::canvas::set_fill_style(ctx, 255, 255, 255);
    webcc::canvas::fill_rect(ctx, 0, 0, 800, 600);

    // Draw Rects
    for (int i = 0; i < RECT_COUNT; ++i) {
        const auto& r = rects[i];
        webcc::canvas::set_fill_style(ctx, r.r, r.g, r.b);
        webcc::canvas::fill_rect(ctx, r.x, r.y, r.w, r.h);
    }

    webcc::flush();
}

int main() {
    webcc::Canvas canvas = webcc::canvas::create_canvas("canvas", 800, 600);
    webcc::dom::append_child(webcc::dom::get_body(), canvas);

    ctx = webcc::canvas::get_context_2d(canvas);

    init_rects();

    webcc::formatter<128> fmt;
    fmt << "Starting WebCC Benchmark with " << RECT_COUNT << " rects...";
    webcc::system::log(fmt.c_str());

    webcc::system::set_main_loop(update);

    webcc::flush();

    return 0;
}
