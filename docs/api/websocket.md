# WebSocket API

The `webcc::websocket` module provides an interface for WebSocket connections.

## Header

```cpp
#include "webcc/websocket.h"
```

## Functions

### `connect`

Creates a new WebSocket connection to the specified URL with all event listeners automatically enabled.

```cpp
webcc::WebSocket connect(webcc::string_view url);
```

All events (message, open, close, error) are automatically subscribed.

### `send`

Sends a text message over the WebSocket connection.

```cpp
void send(webcc::WebSocket handle, webcc::string_view msg);
```

### `close`

Closes the WebSocket connection.

```cpp
void close(webcc::WebSocket handle);
```

## Events

### `MessageEvent`

Generated when a message is received.

```cpp
struct MessageEvent {
    webcc::WebSocket handle;
    webcc::string_view data;
};
```

### `OpenEvent`

Generated when the connection is opened.

```cpp
struct OpenEvent {
    webcc::WebSocket handle;
};
```

### `CloseEvent`

Generated when the connection is closed.

```cpp
struct CloseEvent {
    webcc::WebSocket handle;
};
```

### `ErrorEvent`

Generated when an error occurs.

```cpp
struct ErrorEvent {
    webcc::WebSocket handle;
};
```
