# Clipboard API

The `webcc::clipboard` module copies text to the clipboard and delivers what the user pastes: text, and images as binary data.

## Header

```cpp
#include "webcc/clipboard.h"
```

## Functions

### `write_text`

Copies `text` to the clipboard.

```cpp
void write_text(webcc::string_view text);
```

Browsers only allow this during a user gesture, so call it while handling a click, key press or pointer down. Those events run the update function right away, so the call stays inside the gesture. Called from a timer or a fetch result, it fails with a warning in the console.

On pages served over plain `http` (not `localhost`), where `navigator.clipboard` is missing, it falls back to `document.execCommand('copy')`.

### `init_paste`

Starts listening for pastes (Ctrl+V, Cmd+V, the browser's Paste menu) anywhere on the page.

```cpp
void init_paste(uint8_t prevent_default = 0);
```

- Pastes into an `<input>`, `<textarea>` or `contenteditable` element are left to the browser and produce no event, so text fields keep working.
- With `prevent_default = 1`, the browser's own handling of the other pastes is blocked.
- Calling it again does nothing.

## Events

```cpp
struct PasteTextEvent  { webcc::string_view text; };
struct PasteImageEvent { webcc::Blob image; webcc::string_view mime; };
```

- `PasteTextEvent` carries the plain-text form of the paste.
- `PasteImageEvent` comes once per pasted image, with the encoded file bytes (PNG, JPEG...) in a [Blob](blob.md) and its MIME type, e.g. `"image/png"`. Release the blob with `blob::take` or `blob::free`.
- A paste can produce both: copying an image from a web page usually also gives its URL or alt text. The image event arrives slightly later, since reading the file is asynchronous.
- Both run the update function right away.

## Example

```cpp
#include "webcc/clipboard.h"
#include "webcc/blob.h"
#include "webcc/webcc.h"

void update(double) {
    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto t = e.as<webcc::clipboard::PasteTextEvent>()) {
            insert_text(t->text);
        } else if (auto img = e.as<webcc::clipboard::PasteImageEvent>()) {
            insert_image(webcc::blob::take(img->image), img->mime);
        }
    }
    webcc::flush();
}

int main() {
    webcc::clipboard::init_paste(1);
    webcc::system::set_update(update);
    webcc::flush();
}
```

## Notes

- Reading the clipboard without a paste (`navigator.clipboard.read`) is not supported. Browsers prompt for permission and Firefox and Safari restrict it further; listening for paste avoids both.
- Writing images is not supported yet.
