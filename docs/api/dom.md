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

### `append_child`

Appends a child element to a parent element. Note: Derived handle types like `Canvas`, `Audio`, and `Image` can be passed directly as they implicitly convert to `DOMElement`.

```cpp
void append_child(webcc::DOMElement parent_handle, webcc::DOMElement child_handle);
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
