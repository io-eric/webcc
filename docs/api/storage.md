# Storage API

The `webcc::storage` module provides an interface to the Local Storage API.

Local Storage holds strings only, about 5 MB per site. For binary data or anything larger, use [IndexedDB](idb.md).

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

### `get_item`

Reads a value from local storage by its key. Returns an empty string if the key
does not exist.

```cpp
webcc::string get_item(webcc::string_view key);
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

### `persist`

Asks the browser to keep this site's data, IndexedDB included, when it frees space. Without it, data can be evicted under storage pressure; with it, only the user can clear it. The answer comes back as `PersistedEvent`.

```cpp
void persist();

struct PersistedEvent { uint8_t granted; };
```

- Chrome grants it on its own for sites the user visits often, has installed or bookmarked, and says no otherwise, without asking. Firefox asks the user. Safari grants it to installed web apps.
- Once granted it stays granted, so one call per session is enough. Call it once there is something worth keeping, e.g. after the first save, and tell the user when the answer is `0`: their notes stay until the browser needs the space.
- Browsers without the API answer `0` right away.

```cpp
if (auto p = e.as<webcc::storage::PersistedEvent>()) {
    if (!p->granted) show_hint("The browser may clear your notes when it runs out of space");
}
```
