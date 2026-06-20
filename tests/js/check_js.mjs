// Validates that generated app.js is syntactically valid JavaScript.
//
// app.js is assembled from string templates and schema-derived action snippets
// in generators.cc. This uses the webcc binary to generate app.js for a range of
// feature combinations and syntax-checks each with `new vm.Script`, so a
// malformed JS action is caught in CI rather than at runtime in the browser.
//
// Usage: node tests/js/check_js.mjs <path-to-webcc-binary> <repo-root>
import { execFileSync } from "node:child_process";
import { mkdtempSync, writeFileSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import vm from "node:vm";

const [, , webccBin, repoRoot] = process.argv;
if (!webccBin || !repoRoot) {
  console.error("usage: node check_js.mjs <webcc-binary> <repo-root>");
  process.exit(2);
}

// Feature programs chosen to exercise different generator paths:
// void commands, return-value imports, event delegation, string args, floats.
const cases = {
  canvas: `#include "webcc/canvas.h"
int main(){ auto c=webcc::canvas::create_canvas("c",640,480);
  auto x=webcc::canvas::get_context_2d(c);
  webcc::canvas::fill_rect(x,0,0,100,100);
  webcc::canvas::fill_text(x,"hi",10,10); }`,

  dom_events: `#include "webcc/dom.h"
int main(){ auto b=webcc::dom::get_body();
  auto i=webcc::dom::create_element("input");
  webcc::dom::add_input_listener(i);
  webcc::dom::add_click_listener(i);
  webcc::dom::append_child(b,i); }`,

  webgl: `#include "webcc/webgl.h"
#include "webcc/canvas.h"
int main(){ auto c=webcc::canvas::create_canvas("c",640,480);
  auto gl=webcc::canvas::get_context_webgl(c);
  webcc::webgl::clear_color(gl,0,0,0,1);
  webcc::webgl::clear(gl,16384); }`,

  websocket: `#include "webcc/websocket.h"
int main(){ auto ws=webcc::websocket::connect("wss://x");
  webcc::websocket::send(ws,"hello"); }`,

  fetch_storage: `#include "webcc/fetch.h"
#include "webcc/storage.h"
int main(){ webcc::fetch::get("/api","{}");
  webcc::storage::set_item("k","v"); }`,

  webgpu: `#include "webcc/wgpu.h"
int main(){ webcc::wgpu::request_adapter(); }`,

  // WEBCC_JS inline-JavaScript escape hatch: each named function becomes a wasm
  // import whose name carries its full source, mirrored back into app.js as a
  // handler. Numeric + const char* params, a quoted/multi-line body, and the
  // void/int/double return types must all produce syntactically valid JS.
  inline_js: `#include "webcc/system.h"
WEBCC_JS(void, log_msg, (const char* m), { console.log("inline:", m); });
WEBCC_JS(int, js_add, (int a, int b), { return a + b; });
WEBCC_JS(double, js_now, (), { return performance.now(); });
WEBCC_JS(void, add_para, (const char* text), {
  const p = document.createElement('p');
  p.textContent = text;
  document.body.appendChild(p);
});
int main(){
  log_msg("hello");
  int s = js_add(2, 3);
  double t = js_now();
  add_para("made in raw JS");
  (void)s; (void)t;
}`,
};

const work = mkdtempSync(join(tmpdir(), "webcc-js-"));
let failures = 0;

for (const [name, src] of Object.entries(cases)) {
  const srcPath = join(work, `${name}.cc`);
  const outDir = join(work, name);
  writeFileSync(srcPath, src);
  execFileSync("mkdir", ["-p", outDir]);

  // app.js is generated from the linked wasm's import table (linker-driven
  // feature detection), so a successful compile+link is now a prerequisite. We
  // run webcc fully, then syntax-check whatever app.js it produced.
  try {
    execFileSync(webccBin, ["-o", outDir, srcPath], {
      cwd: repoRoot,
      stdio: "pipe",
    });
  } catch (e) {
    // webcc exited non-zero (e.g. compile/link failure). app.js won't exist;
    // the readFileSync below reports it as a failure for this case.
  }

  let js;
  try {
    js = readFileSync(join(outDir, "app.js"), "utf8");
  } catch {
    console.error(`  FAIL  ${name}: app.js was not generated`);
    failures++;
    continue;
  }

  try {
    // Parse-only: constructing a Script compiles (syntax-checks) without running.
    new vm.Script(js, { filename: `${name}/app.js` });
    console.log(`  PASS  ${name} (app.js parses, ${js.length} bytes)`);
  } catch (e) {
    console.error(`  FAIL  ${name}: ${e.message}`);
    failures++;
  }
}

rmSync(work, { recursive: true, force: true });

console.log("");
if (failures) {
  console.error(`${failures} JS validation failure(s).`);
  process.exit(1);
}
console.log("All generated app.js files are valid JavaScript.");
