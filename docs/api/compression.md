# Compression API

`CompressionStream` and `DecompressionStream`: bytes in, a `webcc::Blob` out.

## Header

```cpp
#include "webcc/compression.h"
```

## Functions

```cpp
webcc::Compression compress(webcc::bytes_view data, webcc::string_view format = "deflate");
webcc::Compression decompress(webcc::bytes_view data, webcc::string_view format = "deflate");
```

- `format`: `deflate` (zlib), `deflate-raw` or `gzip`.
- The input is copied; the result arrives as a `DONE` event.

## Events

```cpp
struct DoneEvent { webcc::Compression job; webcc::Blob data; webcc::string_view error; };
```

One per call. `data` is the result (see [`blob`](blob.md)); on failure it's invalid and `error` is set.
