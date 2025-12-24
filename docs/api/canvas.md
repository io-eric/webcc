# Canvas API

The `webcc::canvas` module provides an interface to the HTML5 Canvas 2D API.

## Header

```cpp
#include "webcc/canvas.h"
```

## Functions

### `create_canvas`

Creates a new `<canvas>` element with the specified ID and dimensions, and appends it to the document body.

```cpp
webcc::handle create_canvas(webcc::string_view dom_id, float width, float height);
```

### `get_context`

Gets the 2D rendering context for a canvas.

```cpp
webcc::handle get_context(webcc::handle canvas_handle, webcc::string_view context_type);
```

### `set_size`

Sets the width and height of the canvas.

```cpp
void set_size(webcc::handle handle, float width, float height);
```

### Drawing Rectangles

```cpp
void fill_rect(webcc::handle handle, float x, float y, float w, float h);
void stroke_rect(webcc::handle handle, float x, float y, float w, float h);
void clear_rect(webcc::handle handle, float x, float y, float w, float h);
```

### Paths

```cpp
void begin_path(webcc::handle handle);
void close_path(webcc::handle handle);
void move_to(webcc::handle handle, float x, float y);
void line_to(webcc::handle handle, float x, float y);
void stroke(webcc::handle handle);
void fill(webcc::handle handle);
void arc(webcc::handle handle, float x, float y, float radius, float start_angle, float end_angle);
```

### Styles

```cpp
void set_fill_style(webcc::handle handle, uint8_t r, uint8_t g, uint8_t b);
void set_fill_style_str(webcc::handle handle, webcc::string_view color);
void set_stroke_style(webcc::handle handle, uint8_t r, uint8_t g, uint8_t b);
void set_stroke_style_str(webcc::handle handle, webcc::string_view color);
void set_line_width(webcc::handle handle, float width);
void set_global_alpha(webcc::handle handle, float alpha);
void set_line_cap(webcc::handle handle, webcc::string_view cap);
void set_line_join(webcc::handle handle, webcc::string_view join);
void set_shadow(webcc::handle handle, float blur, float off_x, float off_y, webcc::string_view color);
```

### Text

```cpp
void fill_text(webcc::handle handle, webcc::string_view text, float x, float y);
void fill_text_f(webcc::handle handle, webcc::string_view fmt, float val, float x, float y);
void fill_text_i(webcc::handle handle, webcc::string_view fmt, int32_t val, float x, float y);
void set_font(webcc::handle handle, webcc::string_view font);
void set_text_align(webcc::handle handle, webcc::string_view align);
```

### Transformations

```cpp
void translate(webcc::handle handle, float x, float y);
void rotate(webcc::handle handle, float angle);
void scale(webcc::handle handle, float x, float y);
void save(webcc::handle handle);
void restore(webcc::handle handle);
```

### Images

```cpp
void draw_image(webcc::handle handle, webcc::handle img_handle, float x, float y);
```
