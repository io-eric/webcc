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

### `ErrorEvent`

Generated when a request fails.

```cpp
struct ErrorEvent {
    webcc::FetchRequest id;   // The request handle
    webcc::string_view error; // The error message
};
```
