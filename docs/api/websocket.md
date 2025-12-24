# WebSocket API

The `webcc::websocket` module provides an interface for WebSocket connections.

## Header

```cpp
#include "webcc/websocket.h"
```

## Functions

### `create`

Creates a new WebSocket connection to the specified URL.

```cpp
webcc::handle create(webcc::string_view url, uint32_t events);
```

The `events` parameter is a bitmask specifying which events to listen for (e.g., `MASK_MESSAGE | MASK_OPEN`).

### `send`

Sends a text message over the WebSocket connection.

```cpp
void send(webcc::handle handle, webcc::string_view msg);
```

### `close`

Closes the WebSocket connection.

```cpp
void close(webcc::handle handle);
```

## Events

### `MessageEvent`

Generated when a message is received.

```cpp
struct MessageEvent {
    webcc::handle handle;
    webcc::string_view data;
};
```

### `OpenEvent`

Generated when the connection is opened.

```cpp
struct OpenEvent {
    webcc::handle handle;
};
```

### `CloseEvent`

Generated when the connection is closed.

```cpp
struct CloseEvent {
    webcc::handle handle;
};
```

### `ErrorEvent`

Generated when an error occurs.

```cpp
struct ErrorEvent {
    webcc::handle handle;
};
```
