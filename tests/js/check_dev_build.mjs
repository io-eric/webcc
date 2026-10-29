// A dev build (--dev, unoptimized) gets a 1MB stack and keeps the functions' names; a release
// build keeps its 64KB stack and strips them. Unoptimized code takes many times the stack per
// call, so a big view's function that fits in 64KB optimized ran off the bottom of it in a dev
// build ("memory access out of bounds"), and the trace named no function.
//
// Usage: node tests/js/check_dev_build.mjs <path-to-webcc-binary>
import { execFileSync } from "node:child_process";
import { mkdtempSync, writeFileSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

const [, , webccBin] = process.argv;
const program = `#include "webcc/dom.h"
int main(){ auto el = webcc::dom::create_element("div"); webcc::dom::set_inner_text(el, "hi"); }`;

function leb(b, i) {
  let r = 0, s = 0, x;
  do { x = b[i++]; r |= (x & 0x7f) << s; s += 7; } while (x & 0x80);
  return [r >>> 0, i];
}

// the stack pointer's start (with --stack-first, the stack's size) and whether a name section is there
function inspect(wasm) {
  let i = 8, stack = -1, names = false;
  while (i < wasm.length) {
    const id = wasm[i];
    let size; [size, i] = leb(wasm, i + 1);
    const end = i + size;
    if (id === 6 && stack < 0) {
      let j = leb(wasm, i)[1] + 2;   // the first global: its type and mutability
      if (wasm[j] === 0x41) stack = leb(wasm, j + 1)[0];
    } else if (id === 0) {
      const [len, j] = leb(wasm, i);
      if (Buffer.from(wasm.subarray(j, j + len)).toString() === "name") names = true;
    }
    i = end;
  }
  return { stack, names };
}

let failed = 0;
for (const dev of [true, false]) {
  const dir = mkdtempSync(join(tmpdir(), "webcc-dev-"));
  try {
    writeFileSync(join(dir, "main.cc"), program);
    const args = [join(dir, "main.cc"), "--out", join(dir, "out"), "--cache-dir", join(dir, "cache"), "--quiet"];
    execFileSync(webccBin, dev ? [...args, "--dev"] : args, { stdio: "pipe" });
    const { stack, names } = inspect(readFileSync(join(dir, "out", "app.wasm")));
    const ok = dev ? stack >= 1048576 && names : stack <= 65536 + 16 && !names;
    console.log(`  ${ok ? "ok" : "FAIL"}  ${dev ? "dev" : "release"}: stack ${stack}, names ${names}`);
    if (!ok) failed++;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}
process.exit(failed ? 1 : 0);
