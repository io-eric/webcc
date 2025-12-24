# Input API

The `webcc::input` module provides functions for handling keyboard and mouse input.

## Header

```cpp
#include "webcc/input.h"
```

## Initialization

Before receiving input events, you must initialize the respective input systems.

```cpp
void init_keyboard();
void init_mouse(webcc::handle handle); // handle is usually the canvas or body
```

## Pointer Lock

```cpp
void request_pointer_lock(webcc::handle handle);
void exit_pointer_lock();
```

## Events

Input events are polled using the main event loop.

### Keyboard Events

```cpp
struct KeyDownEvent {
    int32_t key_code;
};

struct KeyUpEvent {
    int32_t key_code;
};
```

### Mouse Events

```cpp
struct MouseDownEvent {
    int32_t button;
    int32_t x;
    int32_t y;
};

struct MouseUpEvent {
    int32_t button;
    int32_t x;
    int32_t y;
};

struct MouseMoveEvent {
    int32_t x;
    int32_t y;
};
```
