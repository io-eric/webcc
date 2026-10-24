# PDF API

The `webcc::pdf` module reads PDFs: it opens a file and renders its pages into a canvas.

## Header

```cpp
#include "webcc/pdf.h"
```

## Import

Browsers can't render PDF pages on their own, so import uses [pdf.js](https://mozilla.github.io/pdf.js/) (Apache-2.0). Your app ships it: copy these files from the `pdfjs-dist` npm package's `build/` folder into your assets:

- `pdf.min.mjs` (about 450 KB) and `pdf.worker.min.mjs` (about 1.3 MB)
- optionally the `cmaps/` and `standard_fonts/` folders next to them. pdf.js only fetches them for PDFs that need them: Asian text, and fonts not embedded in the file. Without them those PDFs render with fallback fonts or missing text.

pdf.js is loaded the first time a PDF is opened, so it costs nothing until then. In an offline app (PWA) the service worker caches it with the rest of the build.

### `set_library`

Tells webcc where pdf.js is, relative to the page. Call it once before the first `open`.

```cpp
void set_library(webcc::string_view lib_url, webcc::string_view worker_url);

webcc::pdf::set_library("assets/pdf.min.mjs", "assets/pdf.worker.min.mjs");
```

### `open`

Opens a PDF from its bytes in a [Blob](blob.md), e.g. from `files::open` or a drop. The bytes are copied, so the blob can be freed right after. `OpenedEvent` follows.

```cpp
webcc::PdfDocument open(webcc::Blob data);
```

### `page_width`, `page_height`

The size of a page in PDF points (1/72 inch), valid after `OpenedEvent`. `page` is 0-based. A typical A4 page is 595 × 842.

```cpp
double page_width(webcc::PdfDocument doc, int32_t page);
double page_height(webcc::PdfDocument doc, int32_t page);
```

### `render_page`

Renders a page into a canvas, resizing the canvas to the page size times `scale`. `RenderedEvent` follows. For a sharp page, use the zoom times `system::get_device_pixel_ratio()` as the scale. Use an offscreen canvas and draw it with `canvas::draw_image`, as described in [caching](canvas.md#caching-with-offscreen-canvases).

```cpp
webcc::PdfRender render_page(webcc::PdfDocument doc, int32_t page, webcc::Canvas canvas, double scale);
void cancel_render(webcc::PdfRender render);   // e.g. the page scrolled away
```

### `close`

Releases the document. Pages already rendered stay in their canvases.

```cpp
void close(webcc::PdfDocument doc);
```

### Import events

```cpp
struct OpenedEvent   { webcc::PdfDocument doc; int32_t page_count; webcc::string_view error; };
struct RenderedEvent { webcc::PdfRender render; uint8_t ok; webcc::string_view error; };
```

- `page_count` is `0` and `error` says why when the file couldn't be opened (not a PDF, damaged, password protected, pdf.js not found).
- `ok` is `0` for a page that doesn't exist, a failed render, or a cancelled one (`error` is `"cancelled"`).

## Notes

- A rendered page is pixels in a canvas: to put it into a PDF of your own, use it as an image at the resolution it was rendered at. Render at scale 2 or more for print quality.
