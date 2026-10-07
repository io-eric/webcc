# PDF API

The `webcc::pdf` module imports PDFs, rendering their pages into a canvas, and exports PDFs written with canvas-like drawing commands.

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

## Export

A writer collects pages and drawing commands, then builds the file in the background. Coordinates are PDF points with the origin at the top-left and y going down, like a canvas. Strokes stay vectors, so the file is sharp at any zoom.

```cpp
webcc::PdfWriter create_writer();
void add_page(webcc::PdfWriter w, float width, float height);   // starts a new page

void set_stroke_color(webcc::PdfWriter w, uint8_t r, uint8_t g, uint8_t b, float alpha = 1);
void set_fill_color(webcc::PdfWriter w, uint8_t r, uint8_t g, uint8_t b, float alpha = 1);
void set_line_width(webcc::PdfWriter w, float width);
void set_line_cap(webcc::PdfWriter w, uint8_t cap);     // CAP_BUTT, CAP_ROUND, CAP_SQUARE
void set_line_join(webcc::PdfWriter w, uint8_t join);   // JOIN_MITER, JOIN_ROUND, JOIN_BEVEL

void move_to(webcc::PdfWriter w, float x, float y);
void line_to(webcc::PdfWriter w, float x, float y);
void quad_to(webcc::PdfWriter w, float cx, float cy, float x, float y);
void curve_to(webcc::PdfWriter w, float c1x, float c1y, float c2x, float c2y, float x, float y);
void rect(webcc::PdfWriter w, float x, float y, float width, float height);
void close_path(webcc::PdfWriter w);
void stroke(webcc::PdfWriter w);
void fill(webcc::PdfWriter w);
void save(webcc::PdfWriter w);      // push the current colors, width, etc.
void restore(webcc::PdfWriter w);   // and pop them

void draw_image(webcc::PdfWriter w, webcc::DOMElement source, float x, float y, float width, float height);
void draw_text(webcc::PdfWriter w, webcc::string_view text, float x, float y, float size);

void finish(webcc::PdfWriter w);
```

- **Transparency** (`alpha` below 1) works for strokes and fills, e.g. a highlighter.
- **`draw_image`** takes an `Image` or a `Canvas`, so a page rendered by `render_page` can be the background of an exported page. Its pixels are copied right away, so the source can change or be freed afterwards. Opaque images are stored as JPEG, images with transparency as lossless RGB with an alpha mask. The same `Image` drawn several times is stored once.
- **`draw_text`** uses Helvetica, built into every PDF reader, in the fill color, with `y` as the baseline. It covers Latin-1 only; other characters (`€`, CJK, emoji) become `?`. For other text, draw it into a canvas and add that with `draw_image`.
- Commands before the first `add_page` are ignored.

### `WrittenEvent`

```cpp
struct WrittenEvent { webcc::PdfWriter writer; webcc::Blob data; webcc::string_view error; };
```

`data` holds the PDF file. Save it with `files::save`, store it, or upload it. The writer is gone after `finish`.

## Example: export a page of ink over an imported PDF

```cpp
auto w = webcc::pdf::create_writer();
webcc::pdf::add_page(w, 595, 842);
webcc::pdf::draw_image(w, background_canvas, 0, 0, 595, 842);   // rendered with render_page
webcc::pdf::set_stroke_color(w, 20, 20, 20);
webcc::pdf::set_line_width(w, 2);
webcc::pdf::set_line_cap(w, webcc::pdf::CAP_ROUND);
for (const auto &s : strokes) {
    webcc::pdf::move_to(w, s.x[0], s.y[0]);
    for (size_t i = 1; i < s.size(); i++) webcc::pdf::line_to(w, s.x[i], s.y[i]);
    webcc::pdf::stroke(w);
}
webcc::pdf::finish(w);

// later, in the event loop
if (auto done = e.as<webcc::pdf::WrittenEvent>()) {
    webcc::files::save("notes.pdf", "application/pdf", webcc::blob::take(done->data));
}
```

## Notes

- An imported page is exported as an image of the page at the resolution it was rendered at, not as the original vector page. Render at scale 2 or more before exporting for print quality.
- The writer needs `CompressionStream` (Chrome 80, Firefox 113, Safari 16.4).
