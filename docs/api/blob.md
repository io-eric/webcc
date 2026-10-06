# Blob API

The `webcc::blob` module holds binary data on the JavaScript side until C++ asks for it.

Events travel through a fixed 1MB buffer, so large binary data (a file, a response body) is not sent inside an event. The event carries a `webcc::Blob` handle instead, and the application reads the bytes when it wants them.

## Header

```cpp
#include "webcc/blob.h"
```

## Functions

### `create`

Copies bytes into a new blob.

```cpp
webcc::Blob create(webcc::bytes_view data);
```

### `size`

Returns the size of the blob in bytes, or `0` if the handle is invalid or already freed.

```cpp
uint32_t size(webcc::Blob handle);
```

### `read`

Returns a copy of the data. The blob stays alive.

```cpp
webcc::vector<uint8_t> read(webcc::Blob handle);
```

### `take`

Returns the data and frees the blob.

```cpp
webcc::vector<uint8_t> take(webcc::Blob handle);
```

### `free`

Frees the blob without reading it.

```cpp
void free(webcc::Blob handle);
```

## Notes

- A blob lives until `take` or `free` is called on it. Blobs that are never released stay in memory for the lifetime of the page.
- `read` and `take` return an empty vector for an invalid or freed handle.
