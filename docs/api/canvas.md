# Canvas API

The `webcc::canvas` module provides an interface to the HTML5 Canvas 2D API.

## Header

```cpp
#include "webcc/canvas.h"
```

## Functions

### `create_canvas`

Creates a new `<canvas>` element with the specified ID and dimensions, and appends it to the document body. Returns a `Canvas` handle, which can be implicitly converted to `DOMElement` for use with DOM functions.

```cpp
webcc::Canvas create_canvas(webcc::string_view dom_id, float width, float height);
```

### `get_context_2d`

Gets the 2D rendering context for a canvas.

```cpp
webcc::CanvasContext2D get_context_2d(webcc::Canvas canvas_handle, canvas::ContextFlags flags = canvas::ContextFlags(0));
```

`flags` is a combination of `canvas::ContextFlags`:

| Flag | Effect |
| --- | --- |
| `ContextFlags::LOW_LATENCY` | The browser may put the canvas on screen without waiting for the rest of the page, which noticeably cuts the delay between pen and ink. A hint: browsers that can't do it ignore it. |
| `ContextFlags::OPAQUE` | No transparency, a little faster. The page behind the canvas doesn't show through. |

```cpp
auto ctx = canvas::get_context_2d(c);                                                   // normal
auto ink = canvas::get_context_2d(c, canvas::ContextFlags::LOW_LATENCY | canvas::ContextFlags::OPAQUE);
```

The flags only take effect the first time a context is requested for a canvas.

### `get_context_webgl`

Gets the WebGL rendering context for a canvas.

```cpp
webcc::WebGLContext get_context_webgl(webcc::Canvas canvas_handle);
```

### `get_context_webgpu`

Gets the WebGPU rendering context for a canvas.

```cpp
webcc::WGPUContext get_context_webgpu(webcc::Canvas canvas_handle);
```

### `set_size`

Sets the width and height of the canvas.

```cpp
void set_size(webcc::Canvas handle, float width, float height);
```

### `free`

Releases a canvas and its contexts, and frees its pixel memory right away. Use it for cache canvases that are no longer needed. A canvas that was added to the page should be removed with `dom::remove_element` first. Drawing with a freed canvas or its context does nothing.

```cpp
void free(webcc::Canvas handle);
```

### Drawing Rectangles

```cpp
void fill_rect(webcc::CanvasContext2D ctx, float x, float y, float w, float h);
void stroke_rect(webcc::CanvasContext2D ctx, float x, float y, float w, float h);
void clear_rect(webcc::CanvasContext2D ctx, float x, float y, float w, float h);
void rect(webcc::CanvasContext2D ctx, float x, float y, float w, float h);
```

### Paths

```cpp
void begin_path(webcc::CanvasContext2D ctx);
void close_path(webcc::CanvasContext2D ctx);
void move_to(webcc::CanvasContext2D ctx, float x, float y);
void line_to(webcc::CanvasContext2D ctx, float x, float y);
void bezier_curve_to(webcc::CanvasContext2D ctx, float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
void quadratic_curve_to(webcc::CanvasContext2D ctx, float cpx, float cpy, float x, float y);
void stroke(webcc::CanvasContext2D ctx);
void fill(webcc::CanvasContext2D ctx);
void clip(webcc::CanvasContext2D ctx);
void arc(webcc::CanvasContext2D ctx, float x, float y, float radius, float start_angle, float end_angle);
void ellipse(webcc::CanvasContext2D ctx, float x, float y, float radius_x, float radius_y, float rotation, float start_angle, float end_angle, uint8_t counter_clockwise);
void arc_to(webcc::CanvasContext2D ctx, float x1, float y1, float x2, float y2, float radius);
```

### Styles

```cpp
void set_fill_style(webcc::CanvasContext2D ctx, uint8_t r, uint8_t g, uint8_t b);
void set_fill_style_str(webcc::CanvasContext2D ctx, webcc::string_view color);
void set_stroke_style(webcc::CanvasContext2D ctx, uint8_t r, uint8_t g, uint8_t b);
void set_stroke_style_str(webcc::CanvasContext2D ctx, webcc::string_view color);
void set_line_width(webcc::CanvasContext2D ctx, float width);
void set_global_alpha(webcc::CanvasContext2D ctx, float alpha);
void set_global_composite_operation(webcc::CanvasContext2D ctx, webcc::string_view op);
void set_line_cap(webcc::CanvasContext2D ctx, webcc::string_view cap);
void set_line_join(webcc::CanvasContext2D ctx, webcc::string_view join);
void set_shadow(webcc::CanvasContext2D ctx, float blur, float off_x, float off_y, webcc::string_view color);
void set_miter_limit(webcc::CanvasContext2D ctx, float limit);
```

### Text

```cpp
void fill_text(webcc::CanvasContext2D ctx, webcc::string_view text, float x, float y);
void stroke_text(webcc::CanvasContext2D ctx, webcc::string_view text, float x, float y);
void fill_text_f(webcc::CanvasContext2D ctx, webcc::string_view fmt, float val, float x, float y);
void fill_text_i(webcc::CanvasContext2D ctx, webcc::string_view fmt, int32_t val, float x, float y);
void set_font(webcc::CanvasContext2D ctx, webcc::string_view font);
void set_text_align(webcc::CanvasContext2D ctx, webcc::string_view align);
void set_text_baseline(webcc::CanvasContext2D ctx, webcc::string_view baseline);
float measure_text_width(webcc::CanvasContext2D ctx, webcc::string_view text);
```

### Transformations

```cpp
void translate(webcc::CanvasContext2D ctx, float x, float y);
void rotate(webcc::CanvasContext2D ctx, float angle);
void scale(webcc::CanvasContext2D ctx, float x, float y);
void reset_transform(webcc::CanvasContext2D ctx);
void save(webcc::handle handle);
void restore(webcc::handle handle);
void set_transform(webcc::handle handle, float a, float b, float c, float d, float e, float f);
void transform(webcc::handle handle, float a, float b, float c, float d, float e, float f);
```

### Images

```cpp
void draw_image(webcc::CanvasContext2D ctx, webcc::DOMElement source, double x, double y);
void draw_image_scaled(webcc::CanvasContext2D ctx, webcc::DOMElement source, double x, double y, double w, double h);
void draw_image_full(webcc::CanvasContext2D ctx, webcc::DOMElement source, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);
void set_image_smoothing_enabled(webcc::CanvasContext2D ctx, uint8_t enabled);
```

`source` is an `Image` or a `Canvas` (both convert to `DOMElement`). An image that hasn't loaded yet, a broken image, a zero-size canvas or any other element draws nothing.

### Caching with offscreen canvases

A canvas created with `create_canvas` and never added to the page is an offscreen canvas: draw into it once, then draw it onto the visible canvas each frame with `draw_image`. Copying a finished picture is much cheaper than redrawing everything in it, so a page with thousands of strokes stays fast:

```cpp
// Finished strokes go into a cache tile once
webcc::Canvas tile = webcc::canvas::create_canvas("", 512, 512);
webcc::CanvasContext2D tile_ctx = webcc::canvas::get_context_2d(tile);
draw_strokes(tile_ctx, finished_strokes);

// Every frame: copy the tile, then draw only what is still changing
void frame() {
    webcc::canvas::clear_rect(ctx, 0, 0, width, height);
    webcc::canvas::draw_image(ctx, tile, tile_x - scroll_x, tile_y - scroll_y);
    draw_stroke(ctx, current_stroke);
}

// A tile far off screen can be dropped and redrawn later
webcc::canvas::free(tile);
```

Size tiles in device pixels (CSS size × `system::get_device_pixel_ratio()`) and draw them back with `draw_image_scaled` at their CSS size, so cached content stays sharp on HiDPI screens.

### Debugging

```cpp
void log_canvas_info(webcc::handle handle);
```
```
