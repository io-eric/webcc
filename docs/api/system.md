# System API

The `webcc::system` module provides system-level functions for logging, main loop management, and browser interaction.

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
void request_fullscreen(webcc::DOMElement handle);
```

## Time

These functions provide access to the system time.

```cpp
// Returns the time in milliseconds since the page started loading (performance.now())
double get_time();

// Returns the number of milliseconds elapsed since the epoch (Date.now())
double get_date_now();
```