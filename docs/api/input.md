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
void init_mouse(webcc::DOMElement handle); // handle is usually the canvas or body
```

Note: `Canvas` handles can be passed directly as they implicitly convert to `DOMElement`.

## Shortcuts

```cpp
void prevent_key(int32_t key_code, input::Mods mods = input::Mods(0));
void allow_key(int32_t key_code, input::Mods mods = input::Mods(0));
```

`prevent_key` stops the browser's own action for a key combination, so the app can use it as a shortcut: Ctrl+S would otherwise save the page, Ctrl+Z would undo inside a text field. `mods` must match exactly (Ctrl+Z doesn't cover Ctrl+Shift+Z), and on macOS the same shortcuts use Cmd (`Mods::META`) instead of Ctrl (`Mods::CTRL`), so register both. It works whether or not `init_keyboard` was called, and the key events are still delivered. `allow_key` undoes it.

```cpp
input::prevent_key(83, input::Mods::CTRL);  // Ctrl+S
input::prevent_key(83, input::Mods::META);  // Cmd+S
```

### Modifier keys

`input::Mods` is a flags type, also used by `dom::WheelEvent`. Combine values with `|` and test them with `any`:

```cpp
if (any(k->mods, input::Mods::CTRL | input::Mods::META) && k->key == "z") undo();
if (k->mods == input::Mods::SHIFT) { /* Shift alone */ }
```

## Pointer Lock

```cpp
void request_pointer_lock(webcc::DOMElement handle);
void exit_pointer_lock();
```

## Events

Input events are polled using the main event loop.

### Keyboard Events

```cpp
struct KeyDownEvent {
    int32_t key_code;        // KeyboardEvent.keyCode
    input::Mods mods;        // flags: Mods::SHIFT, CTRL, ALT, META (Cmd on macOS)
    uint8_t repeat;          // 1 when the key is held down and auto-repeats
    webcc::string_view key;  // KeyboardEvent.key: "z", "Z", "+", "Enter", "ä"
};

struct KeyUpEvent {
    int32_t key_code;
    uint8_t mods;
    webcc::string_view key;
};
```

`key` is the character or key name after the keyboard layout is applied, so it's the better choice for shortcuts like `+`/`-` whose `key_code` differs between browsers and layouts. Pressing a modifier key itself also produces an event (`key` is `"Shift"`, `"Control"`, ...), with its own bit already set in `mods` on key down.

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
