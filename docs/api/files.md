# Files API

The `webcc::files` module opens files the user picks in the browser's file dialog and saves data as a download. For files dropped on an element, see `add_drop_listener` in the [DOM API](dom.md).

## Header

```cpp
#include "webcc/files.h"
```

## Functions

### `open`

Shows the file dialog. Returns a handle; the result arrives as `OpenedEvent` (once per picked file) or `CancelledEvent`.

```cpp
webcc::FileRequest open(webcc::string_view accept = "", uint8_t multiple = 0);
```

- `accept` filters the dialog like `<input accept>`: extensions and MIME types, comma-separated, e.g. `".md,.txt,image/*"`. Empty allows any file. The user can still pick other files, so check what arrives.
- `multiple = 1` lets the user pick several files.
- Browsers only show the dialog during a user gesture, so call it while handling a click, key press or pointer down. Those events run the update function right away, so the call stays inside the gesture.

### `save`

Saves `data` as a download named `name`. Returns `1` once the download has started.

```cpp
uint8_t save(webcc::string_view name, webcc::string_view mime, webcc::bytes_view data);
```

- The browser decides where the file goes, usually the Downloads folder, and may ask first depending on the user's settings.
- `mime` sets the file type, e.g. `"application/json"`. Empty means `application/octet-stream`.
- The bytes go straight from WASM memory to the browser, not through the command buffer, so large files are fine.

```cpp
webcc::vector<uint8_t> data = export_notebook();
webcc::files::save("notebook.pond", "application/octet-stream", data);
```

## Events

```cpp
struct OpenedEvent {
    webcc::FileRequest request; // Handle returned by open
    webcc::Blob data;           // File contents, release with blob::take or blob::free
    webcc::string_view name;    // File name without the path, e.g. "notes.md"
    webcc::string_view mime;    // e.g. "image/png"; empty when the browser doesn't know the type
    uint32_t index, count;      // This file's position in the selection, and the selection size
};

struct CancelledEvent {
    webcc::FileRequest request; // The dialog was closed without picking a file
};
```

- Files are read asynchronously, so with `multiple` the events can arrive in any order and over several frames. `count` tells when all of them are in.
- Both events run the update function right away.
- `CancelledEvent` relies on the input's `cancel` event: Chrome 113+, Firefox 91+, Safari 16.4+. In older browsers a cancelled dialog sends nothing.

## Example

```cpp
webcc::FileRequest import_req;

void on_import_click() {
    import_req = webcc::files::open(".pond", 0);
}

void update(double) {
    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto f = e.as<webcc::files::OpenedEvent>()) {
            if (f->request == import_req) import_notebook(webcc::blob::take(f->data));
        } else if (auto c = e.as<webcc::files::CancelledEvent>()) {
            // nothing picked
        }
        // ... click handling that calls on_import_click()
    }
    webcc::flush();
}
```
