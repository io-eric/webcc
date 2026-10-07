# IndexedDB API

The `webcc::idb` module is a key-value store on [IndexedDB](https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API): string keys, binary values, no practical size limit. Use it for data that doesn't fit Local Storage (strings only, about 5 MB).

## Header

```cpp
#include "webcc/idb.h"
```

## How it works

IndexedDB is asynchronous. Every call returns a handle right away, and the result arrives later as an event carrying that handle:

| Call | Result event |
| --- | --- |
| `open` | `OpenedEvent` |
| `put`, `remove` | `DoneEvent` or `ErrorEvent` |
| `get` | `ValueEvent` or `ErrorEvent` |
| `keys` | `KeysEvent` or `ErrorEvent` |

- Calls made before `OpenedEvent` wait for the database, so there is no need to wait for it before the first `get`.
- Calls on one database run in the order they were made. A `get` after a `put` of the same key sees the new value.
- Values travel back as a [Blob](blob.md) handle, not inside the event, so large values don't hit the 1 MB event buffer.

## Functions

### `open`

Opens (or creates) the database `name`. Each database holds one key-value store.

```cpp
webcc::Database open(webcc::string_view name);
```

### `put`

Stores `value` under `key`, replacing any old value. The bytes are copied during the call, so the buffer can be reused right away. `DoneEvent` arrives once the write is committed.

```cpp
webcc::IdbRequest put(webcc::Database db, webcc::string_view key, webcc::bytes_view value);
```

### `get`

Reads the value stored under `key`. `ValueEvent` has `found = 0` and an invalid `blob` when the key doesn't exist.

```cpp
webcc::IdbRequest get(webcc::Database db, webcc::string_view key);
```

### `remove`

Deletes `key`. `DoneEvent` also arrives when the key didn't exist.

```cpp
webcc::IdbRequest remove(webcc::Database db, webcc::string_view key);
```

### `keys`

Lists the keys starting with `prefix`, sorted. An empty prefix lists every key. `KeysEvent::keys` holds them joined with `'\n'`, so keys should not contain newlines.

```cpp
webcc::IdbRequest keys(webcc::Database db, webcc::string_view prefix = "");
```

### `close`

Closes the database. Calls already made still finish. Calls made after `close` get `ErrorEvent`.

```cpp
void close(webcc::Database db);
```

## Events

```cpp
struct OpenedEvent { webcc::Database db; uint8_t ok; };
struct DoneEvent   { webcc::IdbRequest request; };
struct ValueEvent  { webcc::IdbRequest request; webcc::Blob blob; uint8_t found; };
struct KeysEvent   { webcc::IdbRequest request; webcc::string_view keys; };
struct ErrorEvent  { webcc::IdbRequest request; webcc::string_view message; };
```

`ok` is `0` when the browser refused to open the database (private mode in some browsers, storage disabled). Calls on it then get `ErrorEvent`. A `ValueEvent` blob must be released with `blob::take` or `blob::free`.

## Example

```cpp
#include "webcc/idb.h"
#include "webcc/blob.h"
#include "webcc/system.h"
#include "webcc/webcc.h"

webcc::Database db;
webcc::IdbRequest load;

void update(double) {
    webcc::Event e;
    while (webcc::poll_event(e)) {
        if (auto v = e.as<webcc::idb::ValueEvent>()) {
            if (v->request == load && v->found) {
                webcc::vector<uint8_t> data = webcc::blob::take(v->blob);
                // ... decode the notebook
            }
        } else if (auto err = e.as<webcc::idb::ErrorEvent>()) {
            webcc::system::error(err->message);
        }
    }
    webcc::flush();
}

int main() {
    db = webcc::idb::open("notes");
    load = webcc::idb::get(db, "notebook/1");   // no need to wait for open
    webcc::system::set_update(update);
    webcc::flush();
}
```

## Notes

- Browsers may evict IndexedDB data under storage pressure unless the site has persistent storage (`navigator.storage.persist()`).
- Keys are compared as strings, so `keys("note/")` matches `note/1` and `note/b` but not `notes`.
