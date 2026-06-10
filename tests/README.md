# WebCC Tests

Zero-dependency test suite covering the binary wire format, schema parsing, and
code generation.

```sh
./tests/run.sh            # build & run everything
./tests/run.sh --update   # regenerate golden snapshots, then run
./tests/run.sh --skip-js  # C++ tests only (no webcc build / node needed)
```

Exit code is non-zero on any failure, so CI fails loudly. The suite runs in CI
(`.github/workflows/ci.yml`) between building the toolchain and the examples.

## Layers

**C++ unit tests** (host-native, linked against the real toolchain sources):

| File | What it covers |
| --- | --- |
| [test_command_buffer.cc](test_command_buffer.cc) | The C++/JS wire format: little-endian ints, IEEE-754 floats/doubles, 8-byte double alignment, 4-byte string padding. This is the contract the generated JS decoder walks. |
| [test_schema.cc](test_schema.cc) | `load_defs` parsing: opcode assignment, `handle(T)` extraction, `RET:` handling, inheritance, pipes inside JS actions, plus the `schema.wcc.bin` binary-cache round-trip. |
| [test_codegen.cc](test_codegen.cc) | Golden snapshots of `emit_headers` and `generate_js_runtime` output, plus tree-shaking assertions (a canvas-only build embeds canvas code and not DOM/WebSocket/WebGPU). |

**JS validation** ([js/check_js.mjs](js/check_js.mjs)): generates `app.js` for
several feature combinations using the real `webcc` binary and parses each with
Node's `vm.Script`, confirming the generated JavaScript is syntactically valid.

## Snapshots

Golden files live in [snapshots/](snapshots/). When you change codegen, run
`./tests/run.sh --update` and review the diff before committing. The diff is the
review of your generator change.

## Adding a test

Add a `TEST(name) { ... }` block (see [framework.h](framework.h)) to any
`test_*.cc`, then list the file in the compile line in [run.sh](run.sh). Tests
self-register; no manual wiring.

## Possible next addition

The JS layer syntax-checks `app.js`. A full execution round-trip (C++ encodes
commands, the generated JS decoder runs, assert the decoded DOM/canvas calls)
would extend coverage to the runtime behavior end to end.
