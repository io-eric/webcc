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
webcc::handle get(webcc::string_view url);
```

### `post`

Initiates a POST request to the specified URL with the given body. Returns a handle that identifies the request.

```cpp
webcc::handle post(webcc::string_view url, webcc::string_view body);
```

## Events

Fetch operations are asynchronous. You must poll for events to receive the results.

### `SuccessEvent`

Generated when a request completes successfully.

```cpp
struct SuccessEvent {
    webcc::handle id;      // The request handle
    webcc::string_view data; // The response body
};
```

### `ErrorEvent`

Generated when a request fails.

```cpp
struct ErrorEvent {
    webcc::handle id;       // The request handle
    webcc::string_view error; // The error message
};
```
