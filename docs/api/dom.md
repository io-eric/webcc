# DOM API

The `webcc::dom` module provides functions for manipulating the Document Object Model (DOM).

## Header

```cpp
#include "webcc/dom.h"
```

## Functions

### `get_body`

Gets a handle to the `<body>` element of the document.

```cpp
webcc::DOMElement get_body();
```

### `get_element_by_id`

Gets a handle to an element by its ID.

```cpp
webcc::DOMElement get_element_by_id(webcc::string_view id);
```

### `create_element`

Creates a new HTML element with the specified tag name.

```cpp
webcc::DOMElement create_element(webcc::string_view tag);
```

### `create_element_deferred`

Creates a new HTML element using a pre-assigned deferred handle. Unlike `create_element`, this function does not return a handle, instead, you provide the handle upfront. This allows the creation command to be batched with other commands, avoiding a synchronous flush.

```cpp
void create_element_deferred(webcc::handle handle, webcc::string_view tag);
```

See [Deferred Handles](#deferred-handles) below for more details.

### `create_comment_deferred`

Creates a new comment node using a pre-assigned deferred handle.

```cpp
void create_comment_deferred(webcc::handle handle, webcc::string_view text);
```

### `append_child`

Appends a child element to a parent element. Note: Derived handle types like `Canvas`, `Audio`, and `Image` can be passed directly as they implicitly convert to `DOMElement`.

```cpp
void append_child(webcc::DOMElement parent_handle, webcc::DOMElement child_handle);
```

### `insert_before`

Inserts a child element before a reference element within a parent. If the reference element is null, the child is appended to the end.

```cpp
void insert_before(webcc::DOMElement parent, webcc::DOMElement child, webcc::DOMElement reference);
```

### `move_before`

Moves an existing element before a reference element within a parent. In the DOM, this is functionally equivalent to `insert_before`, but provided as a distinct semantic operation.

```cpp
void move_before(webcc::DOMElement parent, webcc::DOMElement node, webcc::DOMElement reference);
```

### `remove_element`

Removes an element from the DOM.

```cpp
void remove_element(webcc::DOMElement handle);
```

### Fullscreen and Pointer Lock

Request fullscreen or pointer lock on a specific DOM element (or canvas).

```cpp
void request_fullscreen(webcc::DOMElement handle);
void request_pointer_lock(webcc::DOMElement handle);
```

### Attributes

```cpp
void set_attribute(webcc::DOMElement handle, webcc::string_view name, webcc::string_view value);
webcc::string get_attribute(webcc::DOMElement handle, webcc::string_view name);
```

### Properties

```cpp
void set_property(webcc::DOMElement handle, webcc::string_view name, webcc::string_view value);
webcc::string get_property(webcc::DOMElement handle, webcc::string_view name);
```

`get_property` reads a live JavaScript property as a string, e.g. `"innerText"` of a `contenteditable` box, `"value"` of an input, or `"scrollTop"`. It returns an empty string when the property is unset. Attributes only hold what was set initially, so use properties to read what the user edited.

### Style

```cpp
void set_style(webcc::DOMElement handle, webcc::string_view name, webcc::string_view value);
```

Sets one CSS property without touching the rest of the `style` attribute. `name` is the CSS name: `"left"`, `"font-size"`, or a custom property like `"--ink"`. An empty `value` removes the property.

```cpp
webcc::dom::set_style(box, "left", "120px");
webcc::dom::set_style(box, "top", "48px");
```

### Focus

```cpp
void focus(webcc::DOMElement handle, uint8_t prevent_scroll = 0);
void blur(webcc::DOMElement handle);
void add_focus_listener(webcc::DOMElement handle);
void remove_focus_listener(webcc::DOMElement handle);

struct FocusEvent { webcc::DOMElement handle; };
struct BlurEvent  { webcc::DOMElement handle; };
```

- `focus` moves keyboard focus to the element. `prevent_scroll = 1` keeps the page from scrolling it into view.
- On phones the on-screen keyboard only opens when `focus` runs inside a tap or key handler. Pointer down, click and key events run the update function right away, so calling it there works.
- `add_focus_listener` sends `FocusEvent` and `BlurEvent` when the element itself gains or loses focus. Both run the update function right away.

**Focusing from a canvas tap.** A press on a non-focusable element such as a canvas normally moves focus to the page body right after the pointer down. That undoes a `focus` call made while handling it. Register the canvas with `add_pointer_listener(canvas, 7)` (or at least flag `4`, block touch scrolling), which cancels the press's default action and keeps the focus where you put it.

```cpp
// Text box over the canvas: tap to place it, type, tap again to finish
if (auto p = e.as<webcc::dom::PointerEvent>(); p && p->phase == 0) {
    if (editing) {
        save_text(webcc::dom::get_property(box, "innerText"));
        webcc::dom::blur(box);
        editing = false;
    } else {
        webcc::dom::set_style(box, "left", webcc::string("") + (int)p->x + "px");
        webcc::dom::set_style(box, "top", webcc::string("") + (int)p->y + "px");
        webcc::dom::focus(box, 1);
        editing = true;
    }
}
```

### Content

```cpp
void set_inner_html(webcc::DOMElement handle, webcc::string_view html);
void set_inner_text(webcc::DOMElement handle, webcc::string_view text);
```

### Classes

```cpp
void add_class(webcc::DOMElement handle, webcc::string_view cls);
void remove_class(webcc::DOMElement handle, webcc::string_view cls);
```

### Events

#### `add_click_listener`

Adds a click event listener to an element. When the element is clicked, a `ClickEvent` will be generated.

```cpp
void add_click_listener(webcc::DOMElement handle);
```

#### `ClickEvent`

Structure representing a click event.

```cpp
struct ClickEvent {
    webcc::DOMElement handle; // The handle of the element that was clicked
};
```

#### `add_pointer_listener`

Reports mouse, pen and touch input on an element as `PointerEvent`s.

```cpp
void add_pointer_listener(webcc::DOMElement handle, uint8_t flags = 0);
```

`flags` is a combination of `dom::PointerFlags`:

| Flag | Effect |
| --- | --- |
| `PointerFlags::CAPTURE` | Capture the pointer on down, so moves and the final up keep arriving when it leaves the element. |
| `PointerFlags::COALESCED` | Report every coalesced sample of a move instead of one per event. Pens and fast mice produce several samples per frame; without this, fast strokes look jagged. |
| `PointerFlags::NO_SCROLL` | Set `touch-action: none` and cancel the default action on down, so touch and pen don't scroll the page or select text. |
| `PointerFlags::PREDICT` | After each move with a button down, also report where the browser expects the pointer to be next, as `PointerPhase::PREDICTED` samples. See below. |

For a drawing surface use the first three, plus `PREDICT` for less visible lag:

```cpp
using F = dom::PointerFlags;
dom::add_pointer_listener(canvas, F::CAPTURE | F::COALESCED | F::NO_SCROLL | F::PREDICT);
```

**Predicted samples.** A frame shows where the pen was, not where it is: the ink trails the pen tip by a frame or two. With `POINTER_PREDICT` the browser extrapolates the motion and each move is followed by a few `POINTER_PREDICTED` samples a little ahead of it. Draw them as a temporary tail at the end of the live stroke, and throw them away when the next frame starts: they are guesses, never part of the stroke you store. Only Chrome and Edge produce them; elsewhere none arrive and strokes work as before.

```cpp
if (p->phase == dom::PointerPhase::MOVE) stroke.push(p->x, p->y, p->pressure);
else if (p->phase == dom::PointerPhase::PREDICTED) tail.push(p->x, p->y, p->pressure);
// frame: draw stroke + tail, then tail.clear()
```

Calling it again on the same element does nothing.

Down, up and cancel run the main loop function right away; moves arrive with the next frame.

#### `remove_pointer_listener`

```cpp
void remove_pointer_listener(webcc::DOMElement handle);
```

#### `observe_resize`

Reports the size of an element as `ResizeEvent`s: once right away, then whenever its size or the device pixel ratio changes (window resize, layout change, browser zoom, moving the window to another monitor).

```cpp
void observe_resize(webcc::DOMElement handle);
void unobserve_resize(webcc::DOMElement handle);
```

Calling `observe_resize` again on the same element does nothing. Resize events run the main loop function right away, so the app can resize and redraw before the browser paints the new size.

#### `ResizeEvent`

```cpp
struct ResizeEvent {
    webcc::DOMElement handle;
    float width, height;                // content box in CSS pixels
    int32_t pixel_width, pixel_height;  // the same box in device pixels
    float dpr;                          // device pixels per CSS pixel
};
```

For a sharp canvas, size the element with CSS and its drawing buffer with the device-pixel size:

```cpp
dom::set_attribute(canvas, "style", "width:100%;height:100%");
dom::observe_resize(canvas);

if (auto r = e.as<dom::ResizeEvent>()) {
    canvas::set_size(canvas, r->pixel_width, r->pixel_height);
    // draw at scale r->dpr, or in device pixels directly
}
```

`pixel_width`/`pixel_height` are the exact device pixels the browser uses (`devicePixelContentBoxSize`). In browsers without it (Safari) they are `round(width * dpr)`, which can be one pixel off at fractional scales.

#### `add_wheel_listener`

Reports mouse wheel and trackpad scrolling on an element as `WheelEvent`s.

```cpp
void add_wheel_listener(webcc::DOMElement handle, uint8_t prevent_default = 0);
void remove_wheel_listener(webcc::DOMElement handle);
```

With `prevent_default` set to `true` the page doesn't scroll or zoom while the pointer is over the element, which is what a pannable, zoomable canvas wants. Calling `add_wheel_listener` again on the same element does nothing.

#### `WheelEvent`

```cpp
struct WheelEvent {
    webcc::DOMElement handle;
    float delta_x, delta_y;  // CSS pixels; positive = scroll right / down
    float x, y;              // CSS pixels from the element's top-left corner
    input::Mods mods;        // flags: Mods::SHIFT, CTRL, ALT, META
};
```

- **Pinch to zoom:** a trackpad pinch arrives as a wheel event with `input::Mods::CTRL` set; `delta_y < 0` means zoom in. Safari reports pinches as gesture events instead; these are converted to the same form.
- **Line and page scrolling** (Firefox with a mouse wheel) is converted to pixels: 40 per line, the element's height per page.
- Wheel events arrive with the next frame, like pointer moves.

A typical pan and zoom handler:

```cpp
if (auto w = e.as<dom::WheelEvent>()) {
    if (any(w->mods, input::Mods::CTRL)) zoom_at(w->x, w->y, exp(-w->delta_y / 100));
    else             pan(w->delta_x, w->delta_y);
}
```

#### `PointerEvent`

```cpp
struct PointerEvent {
    webcc::DOMElement handle;
    dom::PointerPhase phase;  // DOWN, MOVE, UP, CANCEL, PREDICTED
    int32_t pointer_id;   // tells apart fingers and devices
    dom::PointerType pointer_type; // MOUSE, PEN, TOUCH
    dom::Buttons buttons;     // flags: PRIMARY (pen tip), SECONDARY (barrel), AUXILIARY, BACK, FORWARD, ERASER
    float x, y;           // CSS pixels from the element's top-left corner
    float pressure;       // 0..1; a mouse reports 0.5 while a button is down
    float tilt_x, tilt_y; // pen tilt in degrees, -90..90
    double time;          // event timestamp in milliseconds
};
```

Moves are also reported while nothing is pressed (`buttons == 0`), e.g. a pen hovering over the screen. A stroke ends with either up or cancel; cancel means the browser took over the pointer (e.g. a touch turned into a scroll), so don't treat it as a finished stroke.

#### `add_drop_listener`

Lets the user drop files on an element. Each dropped file produces a `DropEvent` with its bytes. Dragging files over the element shows the copy cursor; elsewhere the browser keeps its default (usually opening the file). Calling it again on the same element does nothing.

```cpp
void add_drop_listener(webcc::DOMElement handle);
void remove_drop_listener(webcc::DOMElement handle);
```

#### `DropEvent`

```cpp
struct DropEvent {
    webcc::DOMElement handle; // The element the files were dropped on
    webcc::Blob data;         // File contents, release with blob::take or blob::free
    webcc::string_view name;  // File name, e.g. "notes.md"
    webcc::string_view mime;  // e.g. "image/png"; empty when the browser doesn't know the type
    float x, y;               // Drop position in CSS px, relative to the element
    uint32_t index, count;    // This file's position in the drop, and how many files it had
};
```

Files are read asynchronously, so the events of one drop can arrive in any order and over several frames; `count` tells when all of them are in. Each one runs the update function right away. Dropped text or links (not files) are ignored.

## Deferred Handles

WebCC uses a command buffer architecture where API calls are batched and sent to JavaScript in bulk (see [Architecture](../architecture.md)). However, functions that return values, like `create_element`, must synchronously call into JavaScript and trigger a `flush()` to ensure correct execution order. This can be expensive when creating many elements in a loop.

**Deferred handles** solve this problem by letting C++ assign the handle *before* the element is created. The creation command is then added to the command buffer like any other command, and the element is created when the buffer is flushed.


### How It Works

1. **Generate a deferred handle** using `webcc::next_deferred_handle()` (or reserve several at once with `webcc::reserve_deferred_handles(n)`). This returns a unique integer handle that won't collide with handles assigned by JavaScript.
2. **Create the element** using `create_element_deferred(handle, tag)`. This buffers the command without flushing.
3. **Use the handle immediately** in subsequent buffered commands (e.g., `set_attribute`, `append_child`).
4. **Flush** when ready. All commands execute in order, and the element is created with the pre-assigned handle.

### Example

```cpp
#include "webcc/dom.h"

void create_many_elements(webcc::DOMElement parent, int count) {
    for (int i = 0; i < count; i++) {
        // Generate a deferred handle (no JS call)
        webcc::handle h = webcc::next_deferred_handle();
        
        // Buffer the creation command
        webcc::dom::create_element_deferred(h, "div");
        
        // Use the handle immediately in other buffered commands
        webcc::dom::set_attribute(webcc::DOMElement(h), "class", "item");
        webcc::dom::append_child(parent, webcc::DOMElement(h));
    }
    
    // All elements created in a single flush
    webcc::flush();
}
```

### When to Use Deferred Handles

| Scenario | Use |
|----------|-----|
| Creating a single element | `create_element` (simpler API) |
| Creating many elements in a loop | `create_element_deferred` (better performance) |
| Building complex DOM trees | `create_element_deferred` (batch all operations) |
| Framework/library code | `create_element_deferred` (minimize flush overhead) |

### Handle Allocation

Deferred handles are allocated starting from `0x100000` (1,048,576) and increment upward. JavaScript-assigned handles start from lower values. This ensures there are no collisions between the two allocation schemes.

### Reserving a Block

When you know how many nodes you are about to create, `webcc::reserve_deferred_handles(n)` reserves `n` consecutive handles in one step and returns the first. It shares the counter with `next_deferred_handle()`, so the two can be mixed freely. This is smaller and faster than calling `next_deferred_handle()` per node, since the counter is read and written once.

```cpp
int32_t base = webcc::reserve_deferred_handles(3);
webcc::DOMElement list(base), first(base + 1), second(base + 2);
webcc::dom::create_element_deferred(list, "ul");
webcc::dom::create_element_deferred(first, "li");
webcc::dom::create_element_deferred(second, "li");
```
