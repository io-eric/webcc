# DOM API

The `webcc::dom` module provides functions for manipulating the Document Object Model (DOM).

## Header

```cpp
#include "webcc/dom.h"
```

## Functions

### `get_body`

Gets a handle to the `<body>` element of the document.

```cpp
webcc::DOMElement get_body();
```

### `get_element_by_id`

Gets a handle to an element by its ID.

```cpp
webcc::DOMElement get_element_by_id(webcc::string_view id);
```

### `create_element`

Creates a new HTML element with the specified tag name.

```cpp
webcc::DOMElement create_element(webcc::string_view tag);
```

### `create_element_deferred`

Creates a new HTML element using a pre-assigned deferred handle. Unlike `create_element`, this function does not return a handle, instead, you provide the handle upfront. This allows the creation command to be batched with other commands, avoiding a synchronous flush.

```cpp
void create_element_deferred(webcc::handle handle, webcc::string_view tag);
```

See [Deferred Handles](#deferred-handles) below for more details.

### `create_comment_deferred`

Creates a new comment node using a pre-assigned deferred handle.

```cpp
void create_comment_deferred(webcc::handle handle, webcc::string_view text);
```

### `append_child`

Appends a child element to a parent element. Note: Derived handle types like `Canvas`, `Audio`, and `Image` can be passed directly as they implicitly convert to `DOMElement`.

```cpp
void append_child(webcc::DOMElement parent_handle, webcc::DOMElement child_handle);
```

### `insert_before`

Inserts a child element before a reference element within a parent. If the reference element is null, the child is appended to the end.

```cpp
void insert_before(webcc::DOMElement parent, webcc::DOMElement child, webcc::DOMElement reference);
```

### `move_before`

Moves an existing element before a reference element within a parent. In the DOM, this is functionally equivalent to `insert_before`, but provided as a distinct semantic operation.

```cpp
void move_before(webcc::DOMElement parent, webcc::DOMElement node, webcc::DOMElement reference);
```

### `remove_element`

Removes an element from the DOM.

```cpp
void remove_element(webcc::DOMElement handle);
```

### Attributes

```cpp
void set_attribute(webcc::DOMElement handle, webcc::string_view name, webcc::string_view value);
void get_attribute(webcc::DOMElement handle, webcc::string_view name);
```

### Content

```cpp
void set_inner_html(webcc::DOMElement handle, webcc::string_view html);
void set_inner_text(webcc::DOMElement handle, webcc::string_view text);
```

### Classes

```cpp
void add_class(webcc::DOMElement handle, webcc::string_view cls);
void remove_class(webcc::DOMElement handle, webcc::string_view cls);
```

### Events

#### `add_click_listener`

Adds a click event listener to an element. When the element is clicked, a `ClickEvent` will be generated.

```cpp
void add_click_listener(webcc::DOMElement handle);
```

#### `ClickEvent`

Structure representing a click event.

```cpp
struct ClickEvent {
    webcc::DOMElement handle; // The handle of the element that was clicked
};
```

## Deferred Handles

WebCC uses a command buffer architecture where API calls are batched and sent to JavaScript in bulk (see [Architecture](../architecture.md)). However, functions that return values, like `create_element`, must synchronously call into JavaScript and trigger a `flush()` to ensure correct execution order. This can be expensive when creating many elements in a loop.

**Deferred handles** solve this problem by letting C++ assign the handle *before* the element is created. The creation command is then added to the command buffer like any other command, and the element is created when the buffer is flushed.

### How It Works

1. **Generate a deferred handle** using `webcc::next_deferred_handle()`. This returns a unique integer handle that won't collide with handles assigned by JavaScript.
2. **Create the element** using `create_element_deferred(handle, tag)`. This buffers the command without flushing.
3. **Use the handle immediately** in subsequent buffered commands (e.g., `set_attribute`, `append_child`).
4. **Flush** when ready. All commands execute in order, and the element is created with the pre-assigned handle.

### Example

```cpp
#include "webcc/dom.h"

void create_many_elements(webcc::DOMElement parent, int count) {
    for (int i = 0; i < count; i++) {
        // Generate a deferred handle (no JS call)
        webcc::handle h = webcc::next_deferred_handle();
        
        // Buffer the creation command
        webcc::dom::create_element_deferred(h, "div");
        
        // Use the handle immediately in other buffered commands
        webcc::dom::set_attribute(webcc::DOMElement(h), "class", "item");
        webcc::dom::append_child(parent, webcc::DOMElement(h));
    }
    
    // All elements created in a single flush
    webcc::flush();
}
```

### When to Use Deferred Handles

| Scenario | Use |
|----------|-----|
| Creating a single element | `create_element` (simpler API) |
| Creating many elements in a loop | `create_element_deferred` (better performance) |
| Building complex DOM trees | `create_element_deferred` (batch all operations) |
| Framework/library code | `create_element_deferred` (minimize flush overhead) |

### Handle Allocation

Deferred handles are allocated starting from `0x100000` (1,048,576) and increment upward. JavaScript-assigned handles start from lower values. This ensures there are no collisions between the two allocation schemes.

```cpp
inline int32_t next_deferred_handle() {
    static int32_t counter = 0x100000;  // Start high to avoid JS collision
    return counter++;
}
```
