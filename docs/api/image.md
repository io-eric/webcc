# Image API

The `webcc::image` module loads images, from a URL or from encoded bytes, for drawing with `webcc::canvas::draw_image`.

## Header

```cpp
#include "webcc/image.h"
```

## Functions

### `load`

Loads an image from a URL. Returns an `Image` handle, which can be implicitly converted to `DOMElement` for use with DOM functions.

```cpp
webcc::Image load(webcc::string_view src);
```

### `from_blob`

Creates an image from encoded file bytes held in a [Blob](blob.md): PNG, JPEG, WebP, GIF, SVG, anything the browser can show. Use it for pasted, dropped, opened or downloaded images, or bytes read from [IndexedDB](idb.md).

```cpp
webcc::Image from_blob(webcc::Blob blob, webcc::string_view mime = "");
```

- `mime` is the image type, e.g. `"image/png"`. Browsers detect most formats on their own, but SVG needs `"image/svg+xml"`.
- The bytes are copied during the call, so the blob can be freed right after it.
- For bytes in C++ memory, wrap them first: `from_blob(blob::create(bytes), mime)`.

### `free`

Releases the image. Drawing it afterwards does nothing, and no event arrives for it any more.

```cpp
void free(webcc::Image handle);
```

## Events

```cpp
struct LoadedEvent { webcc::Image handle; int32_t width; int32_t height; };
struct ErrorEvent  { webcc::Image handle; };
```

- Every `load` and `from_blob` ends in exactly one of them.
- `LoadedEvent` arrives once the image is decoded, so the first `draw_image` doesn't stall. `width` and `height` are the image's own size in pixels.
- `ErrorEvent` means the URL failed, the bytes aren't an image, or the blob handle was invalid.
- Drawing before `LoadedEvent` is allowed and draws nothing.

## Example

```cpp
webcc::Image pasted;

void update(double) {
    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto p = e.as<webcc::clipboard::PasteImageEvent>()) {
            pasted = webcc::image::from_blob(p->image, p->mime);
            webcc::blob::free(p->image);
        } else if (auto l = e.as<webcc::image::LoadedEvent>()) {
            if (l->handle == pasted) place_image(pasted, l->width, l->height);
        } else if (auto err = e.as<webcc::image::ErrorEvent>()) {
            if (err->handle == pasted) webcc::system::warn("pasted data is not an image");
        }
    }
    webcc::flush();
}
```
