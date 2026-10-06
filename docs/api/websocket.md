# WebSocket API

The `webcc::websocket` module mirrors the browser's [WebSocket](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket) API: text and binary messages, subprotocols, close codes, and connection state.

## Header

```cpp
#include "webcc/websocket.h"
```

## Functions

### `connect`

Creates a new WebSocket connection to the specified URL. All events (open, message, binary message, close, error) are subscribed automatically.

```cpp
webcc::WebSocket connect(webcc::string_view url, webcc::string_view protocols = "");
```

`protocols` is an optional comma-separated list of subprotocols to offer the server, e.g. `"chat, superchat"`. The one the server picked is available from `get_protocol` once the connection is open.

```cpp
auto ws = webcc::websocket::connect("wss://example.com/socket");
```

If the URL or a protocol name is invalid the browser refuses to create the socket: an error is logged to the console and the returned handle is invalid (`handle.is_valid()` is `false`). No events are generated for it.

### `send`

Sends a text message. The message is dropped if the socket is not open.

```cpp
void send(webcc::WebSocket handle, webcc::string_view msg);
```

`send` is batched through the command buffer like other void commands, so the message goes out at the next `flush()`.

### `send_binary`

Sends a binary message. Returns `1` if the socket was open and the data was queued, `0` otherwise.

```cpp
uint8_t send_binary(webcc::WebSocket handle, webcc::bytes_view data);
```

`webcc::bytes_view` is a pointer + length view; it converts implicitly from `webcc::vector<uint8_t>` and similar containers. The data is handed to the browser straight from WASM memory, so there is no extra copy and no size limit from the command buffer. The call is synchronous and flushes pending commands first.

```cpp
uint8_t packet[3] = {1, 2, 3};
webcc::websocket::send_binary(ws, webcc::bytes_view(packet, 3));
```

### `close`

Closes the connection with the default close code.

```cpp
void close(webcc::WebSocket handle);
```

### `close_with_code`

Closes the connection with a close code and reason, which the other side receives.

```cpp
void close_with_code(webcc::WebSocket handle, int32_t code, webcc::string_view reason);
```

The browser only accepts `1000` or a code in the range `3000`-`4999`, and a reason of at most 123 bytes of UTF-8. Anything else is rejected: an error is logged to the console and the socket stays open.

### State

```cpp
int32_t       get_ready_state(webcc::WebSocket handle);     // STATE_CONNECTING, STATE_OPEN, STATE_CLOSING, STATE_CLOSED
uint32_t      get_buffered_amount(webcc::WebSocket handle); // bytes queued by send but not yet transmitted
webcc::string get_protocol(webcc::WebSocket handle);        // subprotocol selected by the server ("" if none)
webcc::string get_extensions(webcc::WebSocket handle);      // extensions selected by the server
webcc::string get_url(webcc::WebSocket handle);             // resolved URL of the connection
```

A socket stays queryable until its `CloseEvent` has been delivered. After that (or for an invalid handle) `get_ready_state` returns `STATE_CLOSED` and the others return `0` / `""`.

## Events

### `OpenEvent`

Generated when the connection is established.

```cpp
struct OpenEvent {
    webcc::WebSocket handle;
};
```

### `MessageEvent`

Generated when a text message is received.

```cpp
struct MessageEvent {
    webcc::WebSocket handle;
    webcc::string_view data;
};
```

### `BinaryMessageEvent`

Generated when a binary message is received.

```cpp
struct BinaryMessageEvent {
    webcc::WebSocket handle;
    webcc::bytes_view data;
};
```

### `CloseEvent`

Generated when the connection is closed, whether by either side or because it failed.

```cpp
struct CloseEvent {
    webcc::WebSocket handle;
    int32_t code;              // e.g. 1000 normal, 1006 abnormal (no close frame)
    webcc::string_view reason; // reason sent by the peer, may be empty
    uint8_t was_clean;         // 1 if the closing handshake completed
};
```

### `ErrorEvent`

Generated when the connection fails. As in the browser, an `ErrorEvent` is always followed by a `CloseEvent` for the same socket.

```cpp
struct ErrorEvent {
    webcc::WebSocket handle;
};
```

## Notes

- `data` and `reason` in events point into the event buffer and are only valid until the next `poll_event()` call. Copy them if you need to keep them.
- Incoming messages are queued in the event buffer (1MB) until the application polls. A message that does not fit in the free space is dropped and a warning is logged to the console, so keep single messages well under 1MB.
