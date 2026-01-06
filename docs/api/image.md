# Image API

The `webcc::image` module provides functions for loading images.

## Header

```cpp
#include "webcc/image.h"
```

## Functions

### `load`

Loads an image from the specified URL. Returns an `Image` handle, which can be implicitly converted to `DOMElement` for use with DOM functions.

```cpp
webcc::Image load(webcc::string_view src);
```

This handle can be used with `webcc::canvas::draw_image`.
