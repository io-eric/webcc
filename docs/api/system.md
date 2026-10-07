# System API

The `webcc::system` module provides system-level functions for logging, main loop management, browser interaction, routing helpers, and page visibility state.

## Header

```cpp
#include "webcc/system.h"
```

## Logging

These functions print messages to the browser's developer console.

```cpp
void log(webcc::string_view msg);
void warn(webcc::string_view msg);
void error(webcc::string_view msg);
```

## Main Loop

Sets the main loop function, which will be called repeatedly by the browser (typically via `requestAnimationFrame`).

```cpp
template <typename T_func>
void set_main_loop(T_func func);
```

The callback function should have the signature `void(double time_ms)`.

### Frames on demand

An app that only changes in response to input (an editor, a notes app) has no reason to redraw 60 times a second while idle. `set_update` registers the same kind of function without starting a loop:

```cpp
template <typename T_func>
void set_update(T_func func);   // register without a loop
void request_frame();           // run the update function once on the next frame
```

With `set_update`, the update function runs:

- right away for input events (click, key, pointer down/up, resize), as with `set_main_loop`
- on the next frame after any other event arrives (pointer move, fetch result, WebSocket message), so polling `poll_event` inside the update still sees everything
- on the next frame after `request_frame()`

Several events or `request_frame()` calls before a frame produce one update. To animate, call `request_frame()` from inside the update while the animation is running and stop calling it when it ends. `request_frame()` is a no-op when a `set_main_loop` loop is already running.

```cpp
bool animating = false;

void update(double t) {
    webcc::Event e;
    while (webcc::poll_event(e)) { /* ... */ }
    draw();
    if (animating) webcc::system::request_frame();
    webcc::flush();
}

int main() {
    webcc::system::set_update(update);
    webcc::system::request_frame();   // first paint
    webcc::flush();
}
```

## Browser Interaction

```cpp
void set_title(webcc::string_view title);
void reload();
void open_url(webcc::string_view url);
```

For element-based browser features like fullscreen and pointer lock, use `webcc::dom` APIs:

```cpp
webcc::dom::request_fullscreen(webcc::DOMElement handle);
webcc::dom::request_pointer_lock(webcc::DOMElement handle);
```

## Routing Helpers

```cpp
webcc::string get_pathname();
webcc::string get_search();
webcc::string get_query_param(webcc::string_view name);
void push_state(webcc::string_view path);
void init_popstate();
```

After calling `init_popstate()`, route changes are emitted through the event system.

```cpp
struct PopstateEvent {
	webcc::string_view path;
};
```

## Time

These functions provide access to the system time.

```cpp
// Returns the time in milliseconds since the page started loading (performance.now())
double get_time();

// Returns the number of milliseconds elapsed since the epoch, UTC (Date.now())
double get_date_now();

// Returns the local timezone offset from UTC in milliseconds.
// Add it to a UTC epoch-ms value to convert it to local time.
double get_timezone_offset_ms();
```

## Display

```cpp
// Device pixels per CSS pixel (window.devicePixelRatio): 2 on most HiDPI screens,
// fractional with OS scaling (e.g. 1.25). It can change while the app runs, e.g. when
// the window moves to another monitor; dom::observe_resize reports those changes.
double get_device_pixel_ratio();
```

## Visibility API

Page visibility lets your app detect whether it is currently visible to the user.

```cpp
webcc::string get_visibility_state(); // Usually "visible" or "hidden"
uint8_t is_hidden();                  // 1 when hidden, 0 otherwise
void init_visibility_change();
```

Use `is_hidden()` when you only need a fast boolean-style check.
Use `get_visibility_state()` when you want the exact browser-reported state string.

After calling `init_visibility_change()`, visibility updates are emitted through the event system. Each one runs the update function right away, because a hidden tab gets no animation frames: without that, the "hidden" event would wait until the user came back.

```cpp
struct VisibilityChangeEvent {
	uint8_t hidden;
	webcc::string_view state;
};
```

### Example

```cpp
#include "webcc/system.h"
#include "webcc/webcc.h"

int main() {
	webcc::system::init_visibility_change();
	webcc::flush();

	webcc::Event e;
	while (webcc::poll_event(e)) {
		if (auto v = e.as<webcc::system::VisibilityChangeEvent>()) {
			if (v->hidden) {
				webcc::system::log("App hidden");
			} else {
				webcc::system::log("App visible");
			}
		}
	}

	return 0;
}
```

## Page Lifecycle

```cpp
void init_lifecycle();
uint8_t is_online();   // 1 when the browser thinks it has a connection

struct PageHideEvent { uint8_t persisted; };
struct PageShowEvent { uint8_t persisted; };
struct OnlineEvent   { uint8_t online; };
```

After `init_lifecycle()`:

- `PageHideEvent` arrives when the user leaves the page, closes the tab, or reloads. `persisted = 1` means the page goes into the back/forward cache and may come back.
- `PageShowEvent` arrives when the page comes back from the back/forward cache (`persisted` is always `1`). A normal page load sends nothing. Reconnect WebSockets here.
- `OnlineEvent` arrives when the connection goes away (`online = 0`) or comes back (`1`).

All three run the update function right away, before the browser moves on.

### When to save

`PageHideEvent` is the last moment the app runs, but only synchronous work is sure to finish then. A `storage::set_item` call lands; an `idb::put` started there is usually lost when the page is torn down. And on phones, a page in the background can be killed without any event at all.

So save in layers:

1. Save to IndexedDB as the user works (e.g. a moment after they stop typing or drawing).
2. Save to IndexedDB when `VisibilityChangeEvent` reports `hidden = 1`: the user switched tabs or apps, and the page is still fully alive.
3. On `PageHideEvent`, write anything still unsaved with `storage::set_item` as a fallback, and pick it up on the next start.

```cpp
if (auto v = e.as<webcc::system::VisibilityChangeEvent>()) {
    if (v->hidden) save_to_idb();
} else if (e.as<webcc::system::PageHideEvent>()) {
    if (dirty) webcc::storage::set_item("unsaved", encode_pending());
}
```

