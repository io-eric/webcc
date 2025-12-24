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
webcc::handle get_body();
```

### `create_element`

Creates a new HTML element with the specified tag name.

```cpp
webcc::handle create_element(webcc::string_view tag);
```

### `append_child`

Appends a child element to a parent element.

```cpp
void append_child(webcc::handle parent_handle, webcc::handle child_handle);
```

### `remove_element`

Removes an element from the DOM.

```cpp
void remove_element(webcc::handle handle);
```

### Attributes

```cpp
void set_attribute(webcc::handle handle, webcc::string_view name, webcc::string_view value);
void get_attribute(webcc::handle handle, webcc::string_view name);
```

### Content

```cpp
void set_inner_html(webcc::handle handle, webcc::string_view html);
void set_inner_text(webcc::handle handle, webcc::string_view text);
```

### Classes

```cpp
void add_class(webcc::handle handle, webcc::string_view cls);
void remove_class(webcc::handle handle, webcc::string_view cls);
```

### Events

#### `add_click_listener`

Adds a click event listener to an element. When the element is clicked, a `ClickEvent` will be generated.

```cpp
void add_click_listener(webcc::handle handle);
```

#### `ClickEvent`

Structure representing a click event.

```cpp
struct ClickEvent {
    webcc::handle handle; // The handle of the element that was clicked
};
```
