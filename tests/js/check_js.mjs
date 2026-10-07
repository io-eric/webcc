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

  // Subprotocols, binary send, close code/reason, state getters
  websocket_full: `#include "webcc/websocket.h"
int main(){ namespace w=webcc::websocket;
  auto ws=w::connect("wss://x","chat, other");
  auto ws2=w::connect("wss://y"); w::close(ws2);
  if (w::get_ready_state(ws)==w::STATE_OPEN) w::close(ws);
  uint8_t b[3]={1,2,3};
  w::send_binary(ws,webcc::bytes_view(b,3));
  w::get_ready_state(ws); w::get_buffered_amount(ws);
  w::get_protocol(ws); w::get_extensions(ws); w::get_url(ws);
  w::close_with_code(ws,1000,"bye"); w::close(ws); }`,

  // Pointer listener: event helper, coalesced samples, immediate update
  pointer: `#include "webcc/dom.h"
int main(){ auto el=webcc::dom::create_element("canvas");
  webcc::dom::append_child(webcc::dom::get_body(),el);
  webcc::dom::add_pointer_listener(el,7);
  webcc::dom::remove_pointer_listener(el); }`,

  // Keyboard with mods/repeat/key and shortcut blocking
  keyboard: `#include "webcc/input.h"
int main(){ webcc::input::init_keyboard();
  webcc::input::prevent_key(83,2); webcc::input::allow_key(83,2); }`,

  // Wheel listener with Safari gesture fallback
  wheel: `#include "webcc/dom.h"
int main(){ auto el=webcc::dom::create_element("canvas");
  webcc::dom::add_wheel_listener(el,1); webcc::dom::remove_wheel_listener(el); }`,

  // Resize observer and devicePixelRatio
  resize: `#include "webcc/dom.h"
#include "webcc/system.h"
int main(){ auto el=webcc::dom::create_element("canvas");
  webcc::dom::observe_resize(el); webcc::dom::unobserve_resize(el);
  return (int)webcc::system::get_device_pixel_ratio(); }`,

  // RET:bytes and the blobs map
  blob: `#include "webcc/blob.h"
int main(){ namespace b=webcc::blob;
  uint8_t d[3]={1,2,3};
  auto h=b::create(webcc::bytes_view(d,3));
  b::size(h); auto r=b::read(h); auto t=b::take(h); b::free(h);
  return (int)(r.size()+t.size()); }`,

  // idb events carry Blob handles
  idb: `#include "webcc/idb.h"
int main(){ namespace i=webcc::idb;
  uint8_t d[3]={1,2,3};
  auto db=i::open("t");
  i::put(db,"k",webcc::bytes_view(d,3)); i::get(db,"k");
  i::remove(db,"k"); i::keys(db); i::close(db); }`,

  // binary fetch: bytes param with a default, fetches + blobs maps
  fetch_binary: `#include "webcc/fetch.h"
int main(){ uint8_t d[2]={1,2};
  auto r=webcc::fetch::request("PUT","/x","{}",webcc::bytes_view(d,2));
  webcc::fetch::request("GET","/y"); webcc::fetch::abort(r); }`,

  clipboard: `#include "webcc/clipboard.h"
int main(){ webcc::clipboard::write_text("hi"); webcc::clipboard::init_paste(1); }`,

  files: `#include "webcc/files.h"
#include "webcc/dom.h"
int main(){ uint8_t d[2]={1,2};
  webcc::files::open(".txt",1); webcc::files::save("a.bin","",webcc::bytes_view(d,2));
  auto el=webcc::dom::create_element("div");
  webcc::dom::add_drop_listener(el); webcc::dom::remove_drop_listener(el); }`,

  image: `#include "webcc/image.h"
#include "webcc/blob.h"
int main(){ uint8_t d[2]={1,2};
  auto b=webcc::blob::create(webcc::bytes_view(d,2));
  auto i=webcc::image::from_blob(b,"image/png"); webcc::image::load("/a.png");
  webcc::image::free(i); }`,

  focus_style: `#include "webcc/dom.h"
int main(){ auto el=webcc::dom::create_element("div");
  webcc::dom::set_style(el,"left","1px"); webcc::dom::focus(el,1); webcc::dom::blur(el);
  webcc::dom::add_focus_listener(el); webcc::dom::remove_focus_listener(el);
  return (int)webcc::dom::get_property(el,"innerText").length(); }`,

  lifecycle: `#include "webcc/system.h"
int main(){ webcc::system::init_lifecycle(); webcc::system::init_visibility_change();
  return webcc::system::is_online(); }`,

  canvas_cache: `#include "webcc/canvas.h"
int main(){ namespace c=webcc::canvas;
  auto v=c::create_canvas("v",100,100); auto ctx=c::get_context_2d(v);
  auto t=c::create_canvas("",10,10); c::draw_image(ctx,t,0,0);
  c::draw_image_scaled(ctx,t,0,0,20,20); c::draw_image_full(ctx,t,0,0,5,5,0,0,10,10);
  c::free(t); }`,

  pdf: `#include "webcc/pdf.h"
#include "webcc/blob.h"
#include "webcc/canvas.h"
int main(){ namespace p=webcc::pdf; uint8_t d[2]={1,2};
  p::set_library("pdf.min.mjs","pdf.worker.min.mjs");
  auto doc=p::open(webcc::blob::create(webcc::bytes_view(d,2)));
  auto c=webcc::canvas::create_canvas("",1,1);
  auto r=p::render_page(doc,0,c,1); p::cancel_render(r); p::close(doc);
  auto w=p::create_writer(); p::add_page(w,100,100); p::set_fill_color(w,1,2,3,0.5f);
  p::move_to(w,0,0); p::quad_to(w,1,1,2,2); p::stroke(w); p::draw_image(w,c,0,0,1,1);
  p::draw_text(w,"hi",0,10,12); p::finish(w);
  return (int)p::page_width(doc,0); }`,

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
