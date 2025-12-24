# Storage API

The `webcc::storage` module provides an interface to the Local Storage API.

## Header

```cpp
#include "webcc/storage.h"
```

## Functions

### `set_item`

Saves a key-value pair to local storage.

```cpp
void set_item(webcc::string_view key, webcc::string_view value);
```

### `remove_item`

Removes an item from local storage by its key.

```cpp
void remove_item(webcc::string_view key);
```

### `clear`

Clears all items from local storage.

```cpp
void clear();
```
