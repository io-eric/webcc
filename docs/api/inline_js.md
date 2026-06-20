# Inline JavaScript (`WEBCC_JS`)

`WEBCC_JS` is WebCC's **escape hatch**: it lets you define a function whose body
is raw JavaScript, for the rare cases the [schema](../../schema.def) doesn't
cover yet. It is the analog of Emscripten's `EM_JS`, built the WebCC way, so it
stays consistent with the rest of the toolchain (linker-driven feature
detection, tree-shaking, no runtime dispatch tables).

> **Prefer the schema.** A schema command is typed, batched through the command
> buffer, and reusable across your codebase. Reach for `WEBCC_JS` only when you
> need a browser API that isn't in the schema yet, and consider
> [contributing it to `schema.def`](../../README.md#contributing-) instead.

## Usage

You declare the function once (a header is fine) and call it like any other C++
function. Arguments are passed by name, so the JS body refers to them by name
(no `$0`/`$1`):

```cpp
#include "webcc/system.h"   // any webcc header makes WEBCC_JS available

WEBCC_JS(void, set_title, (const char* title), {
    document.title = title;
});

WEBCC_JS(int, js_add, (int a, int b), {
    return a + b;
});

WEBCC_JS(double, viewport_area, (), {
    return window.innerWidth * window.innerHeight;
});

void demo() {
    set_title("Hello");           // called like a normal function
    int s = js_add(2, 3);
    double a = viewport_area();
}
```

The macro shape is `WEBCC_JS(return_type, name, (params), { body })`.

Calls are **synchronous** and `flush()` the command buffer first (exactly like
webcc's other return-value calls), so the body observes the effects of any
preceding batched commands.

## Supported types (v1)

**Parameters**

| C++ parameter | In the JS body |
| --- | --- |
| `int`, integer handles | a JS number |
| `unsigned` / `uint32_t` | a JS number (reinterpreted as unsigned) |
| `float`, `double` | a JS number |
| `const char*` | auto-decoded to a JS **string** (read NUL-terminated, so string literals and `webcc::string` work) |

**Return type:** `void`, `int`, `float`, or `double`. For a string *return*, write
the result back yourself for now (e.g. through the scratch buffer); a typed
string return may come later.

Use block comments (`/* */`) inside the body, not `//`, because the source is
collapsed onto a single line.

## How it works

The whole function, signature and body, is compiled into a **wasm import whose
import name is the source itself**, in the module `wjs_fn`:

```
(import "wjs_fn" "set_title(const char* title){ document.title = title; }"
        (func (param i32)))
```

After linking, the `webcc` CLI reads these imports out of the linked module's
import table (the same mechanism used for every other feature), parses each back
into name / params / body, and emits a matching handler into `app.js`:

```js
imports.wjs_fn = {
  "set_title(const char* title){ document.title = title; }":
      (title) => { title = __webcc_utf8(title);  document.title = title; },
};
```

The wasm engine resolves the import by name, so **the import resolution _is_ the
dispatch**: no id table, no per-call lookup. Calling `set_title` from C++ is a
direct, typed wasm import call (arguments cross via the normal ABI), wrapped in a
tiny forwarding stub that adds the automatic `flush()`.

This buys two properties for free:

- **Exact tree-shaking.** A function only appears in the import table if it's
  actually called. Unused `WEBCC_JS` functions are dead-stripped and emit no
  handler, so they cost nothing in the wasm or the JS.
- **No source scanning.** Detection comes from the linked binary, never from
  grepping your `.cc` files, so it's immune to macros, aliases, and conditional
  compilation, consistent with the rest of WebCC's
  [feature detection](../architecture.md#compilation--linking).

`const char*` parameters arrive as integer pointers; the generated handler runs
them through a small `__webcc_utf8` helper that reads the NUL-terminated UTF-8
string from linear memory. That helper is only emitted when a function actually
takes a string parameter.

## Performance

Every `WEBCC_JS` call is **synchronous**: it crosses the JS boundary and flushes
the command buffer first, just like a schema return-value command. That's cheap
for occasional use but not for tight loops. On hot paths, prefer a real schema
command, which batches through the command buffer and crosses the boundary once
per `flush()`. Keep `WEBCC_JS` for setup, one-off calls, and APIs the schema
doesn't expose yet.

## Comparison with Emscripten `EM_JS`

Both declare a named C/C++ function with a JavaScript body and extract that body
at build time. The difference is the channel. Emscripten stores the body in a
custom section and wires it up through generated glue; WebCC carries the whole
source as the import name, so detection, tree-shaking, and dispatch all fall out
of the ordinary wasm import table with no extra runtime machinery.
