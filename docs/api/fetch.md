# Fetch API

The `webcc::fetch` module provides an interface for making HTTP requests.

## Header

```cpp
#include "webcc/fetch.h"
```

## Functions

### `get`

Initiates a GET request to the specified URL. Returns a handle that identifies the request.

```cpp
webcc::FetchRequest get(webcc::string_view url);
webcc::FetchRequest get(webcc::string_view url, webcc::string_view headers_json);
```

The `headers_json` argument is a JSON string of HTTP headers, e.g. `{"apikey":"...","Authorization":"Bearer ..."}`. If omitted, no custom headers are sent.

### `post`

Initiates a POST request to the specified URL with the given body. Returns a handle that identifies the request.

```cpp
webcc::FetchRequest post(webcc::string_view url, webcc::string_view body);
webcc::FetchRequest post(webcc::string_view url, webcc::string_view body, webcc::string_view headers_json);
```

The `headers_json` argument is a JSON string of HTTP headers, e.g. `{"apikey":"...","Authorization":"Bearer ..."}`. If omitted, no custom headers are sent.

### `patch`

Initiates a PATCH request to the specified URL with the given body. Returns a handle that identifies the request.

```cpp
webcc::FetchRequest patch(webcc::string_view url, webcc::string_view body);
webcc::FetchRequest patch(webcc::string_view url, webcc::string_view body, webcc::string_view headers_json);
```

The `headers_json` argument is a JSON string of HTTP headers, e.g. `{"apikey":"...","Authorization":"Bearer ..."}`. If omitted, no custom headers are sent.

### `request`

Sends a request with any method and a binary body, and reports the status and the binary response body. Use it for anything beyond simple text calls: uploads, downloads, checking status codes.

```cpp
webcc::FetchRequest request(webcc::string_view method, webcc::string_view url,
                            webcc::string_view headers_json = "", webcc::bytes_view body = {});
```

- `headers_json` works as in `get`. Invalid JSON logs a warning and sends no custom headers.
- An empty `body` sends no body (required for `GET` and `HEAD`). The bytes are copied during the call, so the buffer can be reused right away.
- Every HTTP response, `404` and `500` included, ends in `DoneEvent` with the status. Only a network failure (offline, DNS, CORS) or `abort` ends in `ErrorEvent`.

```cpp
webcc::vector<uint8_t> notebook = encode();
auto up = webcc::fetch::request("PUT", "/api/notebooks/1",
                                "{\"Content-Type\":\"application/octet-stream\"}", notebook);
```

### `abort`

Cancels a request made with `request`. It ends in `ErrorEvent` with the message `"aborted"`, or does nothing if the request already finished. Requests from `get`, `post` and `patch` can't be aborted.

```cpp
void abort(webcc::FetchRequest id);
```

## Events

Fetch operations are asynchronous. You must poll for events to receive the results.

### `SuccessEvent`

Generated when a request completes successfully.

```cpp
struct SuccessEvent {
    webcc::FetchRequest id;  // The request handle
    webcc::string_view data; // The response body
};
```

### `DoneEvent`

Generated when a request made with `request` gets a response, whatever its status.

```cpp
struct DoneEvent {
    webcc::FetchRequest id; // The request handle
    int32_t status;         // HTTP status, e.g. 200, 404
    webcc::Blob body;       // The response body, release with blob::take or blob::free
};
```

The body travels as a [Blob](blob.md) handle, so a large download doesn't hit the 1 MB event buffer.

```cpp
if (auto d = e.as<webcc::fetch::DoneEvent>()) {
    webcc::vector<uint8_t> body = webcc::blob::take(d->body);
    if (d->status == 200) load(body);
}
```

### `ErrorEvent`

Generated when a request fails. For `get`, `post` and `patch` that includes a non-2xx status; for `request` only a network failure or `abort`.

```cpp
struct ErrorEvent {
    webcc::FetchRequest id;   // The request handle
    webcc::string_view error; // The error message
};
```
