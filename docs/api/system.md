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

The callback function should have the signature `void(float time_ms)`.

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

// Returns the number of milliseconds elapsed since the epoch (Date.now())
double get_date_now();
```

## Visibility API

Page visibility lets your app detect whether it is currently visible to the user.

```cpp
webcc::string get_visibility_state(); // "visible", "hidden", etc.
uint8 is_hidden();                    // 1 when hidden, 0 otherwise
void init_visibility_change();
```

After calling `init_visibility_change()`, visibility updates are emitted through the event system.

```cpp
struct VisibilityChangeEvent {
	uint8 hidden;
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