# Image API

The `webcc::image` module provides functions for loading images.

## Header

```cpp
#include "webcc/image.h"
```

## Functions

### `load`

Loads an image from the specified URL. Returns a handle to the image object.

```cpp
webcc::handle load(webcc::string_view src);
```

This handle can be used with `webcc::canvas::draw_image`.
