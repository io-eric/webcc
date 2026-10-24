# Compression API

The browser's `CompressionStream` and `DecompressionStream`: bytes in, a `webcc::Blob` out when
they're done.

## Header

```cpp
#include "webcc/compression.h"
```

## Functions

```cpp
webcc::Compression compress(webcc::bytes_view data, webcc::string_view format = "deflate");
webcc::Compression decompress(webcc::bytes_view data, webcc::string_view format = "deflate");
```

- `format` is one of the browser's: `deflate` (zlib-wrapped, as PDF and PNG streams use),
  `deflate-raw` or `gzip`.
- The bytes are copied right away; the work runs off the call, so the result comes as an event.

## Events

```cpp
struct DoneEvent { webcc::Compression job; webcc::Blob data; webcc::string_view error; };
```

One `DONE` per call, for its handle: `data` holds the result (read or take it with
[`blob`](blob.md)), or `error` says why there is none (a format the browser doesn't know, bytes
that don't decompress).
