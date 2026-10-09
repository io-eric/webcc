// Golden-snapshot and behavioral tests for code generation.
//
// These run the generators (generate_js_runtime, emit_headers) against the real
// schema.def and compare output to committed golden files in tests/snapshots/,
// so any change to generated headers or JS shows up as a reviewable diff.
//
// To refresh goldens after an intentional change: ./tests/run.sh --update
#include "framework.h"
#include "schema.h"
#include "generators.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <set>
#include <unistd.h>

using namespace webcc;

namespace
{
    bool update_mode()
    {
        const char *v = std::getenv("WEBCC_UPDATE_SNAPSHOTS");
        return v && v[0] == '1';
    }

    std::string snapshot_path(const std::string &name)
    {
        return std::string(WEBCC_SNAPSHOT_DIR) + "/" + name;
    }

    // Compare `actual` against the golden file `name`. In update mode, (re)writes
    // the golden instead of comparing.
    void check_snapshot(const std::string &name, const std::string &actual)
    {
        std::string path = snapshot_path(name);
        if (update_mode())
        {
            std::ofstream out(path, std::ios::binary);
            out << actual;
            out.close();
            std::cout << "    [updated snapshot] " << name << "\n";
            return;
        }

        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            ::webcc_test::record_failure(
                "missing snapshot '" + name +
                "' - run ./tests/run.sh --update to create it");
            return;
        }
        std::string expected((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        if (actual != expected)
        {
            // Find first differing line for a readable message.
            size_t line = 1, col = 1;
            size_t n = std::min(actual.size(), expected.size());
            size_t i = 0;
            for (; i < n && actual[i] == expected[i]; ++i)
            {
                if (actual[i] == '\n')
                {
                    ++line;
                    col = 1;
                }
                else
                    ++col;
            }
            ::webcc_test::record_failure(
                "snapshot '" + name + "' differs at line " + std::to_string(line) +
                " col " + std::to_string(col) +
                " (run ./tests/run.sh --update to accept)");
        }
    }

    // Load the real project schema (absolute path passed in at compile time).
    SchemaDefs real_defs()
    {
        return load_defs(std::string(WEBCC_SCHEMA_DEF));
    }

    // Build the module-"w" marker set (opcode strings) for the given void
    // commands, mirroring what the linker emits when those wrappers are
    // referenced by user code. generate_js_runtime detects void commands from
    // this set, exactly as it does from the real wasm import table.
    std::set<std::string> void_markers(const SchemaDefs &defs,
                                       const std::set<std::string> &qualified_names)
    {
        std::set<std::string> out;
        for (const auto &c : defs.commands)
            if (qualified_names.count(c.ns + "::" + c.func_name))
                out.insert(std::to_string((int)c.opcode));
        return out;
    }
}

TEST(codegen_js_canvas_snapshot)
{
    SchemaDefs defs = real_defs();
    // A canvas-only program. Return commands are detected from the linker import
    // set; void commands (fill_rect) are detected from their module-"w" markers.
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_canvas_create_canvas",
        "webcc_canvas_get_context_2d",
    };
    auto markers = void_markers(defs, {"canvas::fill_rect"});
    generate_js_runtime(defs, imports, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    check_snapshot("app_canvas.js", js);
}

TEST(codegen_js_treeshakes_unused_modules)
{
    SchemaDefs defs = real_defs();
    // Same canvas-only program as above (return imports + void-command markers).
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_canvas_create_canvas",
        "webcc_canvas_get_context_2d",
    };
    auto markers = void_markers(defs, {"canvas::fill_rect"});
    generate_js_runtime(defs, imports, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    // Canvas code IS present...
    CHECK(js.find("fillRect") != std::string::npos);
    CHECK(js.find("getContext('2d', { desynchronized: !!(flags & 1), alpha: !(flags & 2) })") != std::string::npos);
    // ...but unused modules are tree-shaken out.
    CHECK(js.find("webcc_dom_get_element_by_id") == std::string::npos);
    CHECK(js.find("new WebSocket") == std::string::npos);
    CHECK(js.find("localStorage") == std::string::npos);
    CHECK(js.find("navigator.gpu") == std::string::npos);
}

TEST(codegen_js_emits_event_delegation_only_when_used)
{
    SchemaDefs defs = real_defs();
    // A DOM program that registers a click listener but no keydown listener.
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_dom_get_body",
        "webcc_dom_create_element",
    };
    auto markers = void_markers(defs, {"dom::append_child", "dom::add_click_listener"});
    generate_js_runtime(defs, imports, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    // Click delegation wired up; keydown delegation not (unused).
    CHECK(js.find("addEventListener('click'") != std::string::npos);
    CHECK(js.find("push_event_dom_CLICK") != std::string::npos);
    CHECK(js.find("addEventListener('keydown'") == std::string::npos);
}

TEST(codegen_dom_user_snapshot)
{
    SchemaDefs defs = real_defs();
    // A DOM program: create a div, set its text, append it to the body.
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_dom_get_body",
        "webcc_dom_create_element",
    };
    auto markers = void_markers(defs, {"dom::set_inner_text", "dom::append_child"});
    generate_js_runtime(defs, imports, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    check_snapshot("app_dom.js", js);
}

// --- WEBCC_JS inline-JavaScript escape hatch -------------------------------
// Named WEBCC_JS functions reach the generator as imports from module "wjs_fn",
// each of the form `name(params){body}` (the JS source itself). main.cc reads
// them into a set; here we feed that set directly, mirroring the import table.

TEST(codegen_js_inline_js_fn_handlers)
{
    SchemaDefs defs = real_defs();
    std::set<std::string> imports = {"webcc_js_flush"};
    std::set<std::string> markers;
    std::set<std::string> fns = {
        "js_add(int a, int b){ return a + b; }",
        "set_title(const char* title){ document.title = title; }",
    };
    generate_js_runtime(defs, imports, markers, fns, "/tmp");
    std::string js = read_file("/tmp/app.js");

    // The wjs_fn import module is emitted.
    CHECK(js.find("wjs_fn:") != std::string::npos);
    // Named params become the handler's params; the body is reproduced verbatim.
    CHECK(js.find("(a, b) => {") != std::string::npos);
    CHECK(js.find("return a + b;") != std::string::npos);
    // The object key is the full source, JS-escaped, so it parses back to the
    // exact import name the wasm expects.
    CHECK(js.find("\"js_add(int a, int b){ return a + b; }\"") != std::string::npos);
    // A const char* parameter is auto-decoded from the pointer to a JS string.
    CHECK(js.find("title = __webcc_utf8(title)") != std::string::npos);
    CHECK(js.find("const __webcc_utf8 = ") != std::string::npos);
}

TEST(codegen_js_inline_js_fn_no_utf8_without_string_params)
{
    SchemaDefs defs = real_defs();
    std::set<std::string> imports = {"webcc_js_flush"};
    std::set<std::string> markers;
    // Only numeric params -> the UTF-8 string decoder must NOT be emitted.
    std::set<std::string> fns = {"js_add(int a, int b){ return a + b; }"};
    generate_js_runtime(defs, imports, markers, fns, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("js_add") != std::string::npos);
    CHECK(js.find("__webcc_utf8") == std::string::npos);
}

TEST(codegen_js_no_inline_js_when_none_used)
{
    SchemaDefs defs = real_defs();
    std::set<std::string> imports = {"webcc_js_flush", "webcc_canvas_create_canvas"};
    std::set<std::string> markers;
    // A build with no WEBCC_JS functions emits no wjs_fn module at all.
    generate_js_runtime(defs, imports, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("wjs_fn") == std::string::npos);
    CHECK(js.find("__webcc_utf8") == std::string::npos);
}

// String results over the scratch buffer size are fetched with webcc_js_read_result.
TEST(codegen_js_large_string_result)
{
    SchemaDefs defs = real_defs();
    std::set<std::string> imports = {"webcc_js_flush", "webcc_storage_get_item"};
    generate_js_runtime(defs, imports, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("if (encoded.length > 4096) _big_result = encoded;") != std::string::npos);
    CHECK(js.find("webcc_js_read_result: (ptr) =>") != std::string::npos);
    CHECK(js.find("let _big_result = null;") != std::string::npos);

    // Not emitted when no string-returning command is used
    imports = {"webcc_js_flush", "webcc_canvas_create_canvas"};
    generate_js_runtime(defs, imports, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("_big_result") == std::string::npos);
    CHECK(js.find("webcc_js_read_result") == std::string::npos);
}

// Text and binary frames are separate events, close carries code/reason.
TEST(codegen_js_websocket_events)
{
    SchemaDefs defs = real_defs();
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_websocket_connect",
        "webcc_websocket_send_binary",
    };
    generate_js_runtime(defs, imports, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_websocket_MESSAGE(") != std::string::npos);
    CHECK(js.find("function push_event_websocket_BINARY_MESSAGE(") != std::string::npos);
    CHECK(js.find("function push_event_websocket_CLOSE(handle, code, reason, was_clean)") != std::string::npos);
    CHECK(js.find("binaryType = 'arraybuffer'") != std::string::npos);
    // bytes param of a return-value command
    CHECK(js.find("webcc_websocket_send_binary: (handle, data_ptr, data_len)") != std::string::npos);
    CHECK(js.find("const data = new Uint8Array(memory.buffer, data_ptr, data_len);") != std::string::npos);
    CHECK(js.find("webcc_websocket_connect: (url_ptr, url_len, protocols_ptr, protocols_len)") != std::string::npos);
}

// The buffer-full check uses the real payload length.
TEST(codegen_js_event_size_check_uses_payload_length)
{
    SchemaDefs defs = real_defs();
    std::set<std::string> imports = {"webcc_js_flush", "webcc_websocket_connect"};
    generate_js_runtime(defs, imports, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("+ data_1.length > EVENT_BUFFER_SIZE") != std::string::npos);
    CHECK(js.find("4096 > EVENT_BUFFER_SIZE") == std::string::npos);
    // 24-bit length in the header
    CHECK(js.find("| (len >> 16 << 8) | (len << 16);") != std::string::npos);
}

// opcodes over 255
TEST(codegen_command_opcodes_past_255)
{
    const char *def_path = "/tmp/webcc_test_many_ops.def";
    {
        std::ofstream out(def_path);
        for (int i = 1; i <= 300; ++i)
            out << "big|command|C" << i << "|f" << i << "|int32:v|{ sink(v); }\n";
    }
    SchemaDefs defs = load_defs(def_path);
    std::remove(def_path);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }
    const char *tmp = "/tmp/webcc_many_ops_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());
    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }
    emit_headers(defs);
    std::string header = read_file("include/webcc/big.h");
    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed - subsequent tests unsafe");
        return;
    }

    CHECK(header.find("OP_C300 = 0x12c,") != std::string::npos);
    CHECK(header.find("import_name(\"300\")") != std::string::npos);

    auto markers = void_markers(defs, {"big::f44", "big::f300"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("case 300: {") != std::string::npos);
    CHECK(js.find("case 44: {") != std::string::npos);
}

// RET:bytes returns a vector<uint8_t>, small results via the scratch buffer.
TEST(codegen_bytes_return)
{
    const char *def_path = "/tmp/webcc_test_bytes_ret.def";
    {
        std::ofstream out(def_path);
        out << "net|command|READ|read|int32:id RET:bytes|{ const ret = store[id]; }\n";
    }
    SchemaDefs defs = load_defs(def_path);
    std::remove(def_path);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }
    const char *tmp = "/tmp/webcc_bytes_ret_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());
    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }
    emit_headers(defs);
    std::string header = read_file("include/webcc/net.h");
    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed - subsequent tests unsafe");
        return;
    }

    CHECK(header.find("extern \"C\" uint32_t webcc_net_read(int32_t id);") != std::string::npos);
    CHECK(header.find("inline webcc::vector<uint8_t> read(") != std::string::npos);
    CHECK(header.find("return ::webcc::take_bytes_result(len);") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_net_read"}, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("if (ret.length > 4096) _big_result = ret;") != std::string::npos);
    CHECK(js.find("return ret.length;") != std::string::npos);
    CHECK(js.find("webcc_js_read_result: (ptr) =>") != std::string::npos);
}

// handle-ness comes from the schema, not the name
TEST(codegen_handle_types_are_explicit)
{
    const char *def_path = "/tmp/webcc_test_names.def";
    {
        std::ofstream out(def_path);
        out << "ns|event|EV|int32:pointer_id uint32:id int32:handle handle(Thing):thing\n"
               "ns|command|CMD|cmd|int32:user_id handle:raw handle(Thing):thing|{}\n";
    }
    SchemaDefs defs = load_defs(def_path);
    std::remove(def_path);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }
    const char *tmp = "/tmp/webcc_names_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());
    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }
    emit_headers(defs);
    std::string header = read_file("include/webcc/ns.h");
    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed - subsequent tests unsafe");
        return;
    }

    CHECK(header.find("int32_t pointer_id;") != std::string::npos);
    CHECK(header.find("uint32_t id;") != std::string::npos);
    CHECK(header.find("int32_t handle;") != std::string::npos);
    CHECK(header.find("webcc::Thing thing;") != std::string::npos);
    CHECK(header.find("inline void cmd(int32_t user_id, webcc::handle raw, webcc::Thing thing){") != std::string::npos);
}

// add_pointer_listener pulls in the POINTER event helper.
TEST(codegen_js_pointer_listener)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::add_pointer_listener"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_dom_POINTER(handle, phase, pointer_id, pointer_type, buttons, x, y, pressure, tilt_x, tilt_y, time)") != std::string::npos);
    CHECK(js.find("getCoalescedEvents") != std::string::npos);
    CHECK(js.find("setPointerCapture") != std::string::npos);
    CHECK(js.find("for (const p of e.getPredictedEvents()) send(4, e, r, p);") != std::string::npos);
    CHECK(js.find("_triggerDiscreteUpdate()") != std::string::npos);
    // Not pulled in by unrelated DOM use
    markers = void_markers(defs, {"dom::append_child"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("push_event_dom_POINTER") == std::string::npos);
}

// set_update: no loop, events request frames
TEST(codegen_js_frames_on_demand)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"system::set_update", "system::request_frame", "dom::add_pointer_listener"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function _requestFrame()") != std::string::npos);
    CHECK(js.find("_frameOnDemand = true;") != std::string::npos);
    CHECK(js.find("cancelAnimationFrame(_frameRaf)") != std::string::npos);
    // the POINTER helper wakes the update
    size_t helper = js.find("function push_event_dom_POINTER(");
    CHECK(helper != std::string::npos);
    size_t helper_end = js.find("\n    }\n", helper);
    CHECK(helper_end != std::string::npos);
    CHECK(js.find("_wake();", helper) < helper_end);
    // set_main_loop turns on-demand mode back off
    markers = void_markers(defs, {"system::set_main_loop"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("_frameOnDemand = false;") != std::string::npos);
}

TEST(codegen_js_svg_elements_get_their_namespace)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::create_element", "dom::create_element_deferred_scoped"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("createElementNS('http://www.w3.org/2000/svg', tag)") != std::string::npos);
    CHECK(js.find("const el = _mkel(tag);") != std::string::npos);
    CHECK(js.find("document.createElement(tag); elements[handle]") == std::string::npos);
}

TEST(codegen_js_events_in_hidden_tab)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"system::set_main_loop", "dom::add_pointer_listener"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("if (!document.hidden) return _requestFrame();") != std::string::npos);
    CHECK(js.find("_wakeTimer = setTimeout(") != std::string::npos);
    CHECK(js.find("const loop = (t) => { _update(t);") != std::string::npos);
    size_t helper = js.find("function push_event_dom_POINTER(");
    CHECK(helper != std::string::npos);
    size_t helper_end = js.find("\n    }\n", helper);
    CHECK(js.find("_wake();", helper) < helper_end);
    // full buffer: run an update to drain it before dropping
    CHECK(js.find("&& pos && _update(performance.now())) { _eventViews(); pos = event_offset_view[0]; }", helper) < helper_end);
}

// Key events carry mods, repeat and the key string.
TEST(codegen_js_keyboard_events)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"input::init_keyboard", "input::prevent_key"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_input_KEY_DOWN(key_code, mods, repeat, key)") != std::string::npos);
    CHECK(js.find("function push_event_input_KEY_UP(key_code, mods, key)") != std::string::npos);
    CHECK(js.find("e.preventDefault()") != std::string::npos);
}

// add_wheel_listener pulls in the WHEEL event helper.
TEST(codegen_js_wheel_listener)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::add_wheel_listener"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_dom_WHEEL(handle, delta_x, delta_y, x, y, mods)") != std::string::npos);
    CHECK(js.find("{ passive: false }") != std::string::npos);
    CHECK(js.find("gesturechange") != std::string::npos);
    // ZOOM_ONLY: the page keeps scrolling, only ctrl/meta wheel and pinch gestures are taken
    CHECK(js.find("zoomOnly && (e.ctrlKey || e.metaKey)") != std::string::npos);
}

// add_scroll_listener pulls in the SCROLL event helper; the listener is passive and coalesced per frame.
TEST(codegen_js_scroll_listener)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::add_scroll_listener"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_dom_SCROLL(handle, left, top)") != std::string::npos);
    CHECK(js.find("{ passive: true }") != std::string::npos);
    // the node comes from the event, so a morph can move the listener to another node
    CHECK(js.find("push_event_dom_SCROLL(handle, t.scrollLeft, t.scrollTop)") != std::string::npos);
}

// morph pulls in its helper; place moves a node only when it isn't before ref already
TEST(codegen_js_morph)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::morph", "dom::place", "dom::detach"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("const __wcc_morph = {") != std::string::npos);
    CHECK(js.find("__wcc_morph.item(elements, old_handle, new_handle)") != std::string::npos);
    CHECK(js.find("node.nextSibling !== ref") != std::string::npos);
}

// set_timeout pulls in the TIMER event helper and fires a discrete update.
TEST(codegen_js_timer)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"system::clear_timeout"});
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_system_set_timeout"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_system_TIMER(timer)") != std::string::npos);
    CHECK(js.find("__wcc_timers") != std::string::npos);
    CHECK(js.find("clearTimeout(timers[timer])") != std::string::npos);
}

// to_data_url returns the canvas as a data URL string.
TEST(codegen_js_canvas_data_url)
{
    SchemaDefs defs = real_defs();
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_canvas_to_data_url"}, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("c.toDataURL(mime, quality)") != std::string::npos);
}

// observe_resize pulls in the RESIZE event helper.
TEST(codegen_js_resize_observer)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::observe_resize"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");

    CHECK(js.find("function push_event_dom_RESIZE(handle, width, height, pixel_width, pixel_height, dpr)") != std::string::npos);
    CHECK(js.find("device-pixel-content-box") != std::string::npos);
    CHECK(js.find("new ResizeObserver") != std::string::npos);
}

// The blobs map is only emitted when a blob command is used.
TEST(codegen_js_blob_map)
{
    SchemaDefs defs = real_defs();
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_blob_take"}, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("const blobs = [];") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_canvas_create_canvas"}, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("blobs") == std::string::npos);
}

// idb pulls in its helpers, databases and blobs
TEST(codegen_js_idb)
{
    SchemaDefs defs = real_defs();
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_idb_open", "webcc_idb_get"}, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("const databases = [];") != std::string::npos);
    CHECK(js.find("const blobs = [];") != std::string::npos);
    CHECK(js.find("function push_event_idb_OPENED(db, ok)") != std::string::npos);
    CHECK(js.find("function push_event_idb_VALUE(request, blob, found)") != std::string::npos);
    CHECK(js.find("createObjectStore('kv')") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_blob_take"}, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("databases") == std::string::npos);
    CHECK(js.find("push_event_idb_") == std::string::npos);
}

// fetch::request pulls in its map and DONE, the text API doesn't
TEST(codegen_js_fetch_request)
{
    SchemaDefs defs = real_defs();
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_fetch_request"}, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("const fetches = [];") != std::string::npos);
    CHECK(js.find("const blobs = [];") != std::string::npos);
    CHECK(js.find("function push_event_fetch_DONE(id, status, body)") != std::string::npos);
    CHECK(js.find("new AbortController()") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_fetch_get"}, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("fetches") == std::string::npos);
    CHECK(js.find("push_event_fetch_DONE") == std::string::npos);
}

// init_paste pulls in paste events and blobs, write_text doesn't
TEST(codegen_js_clipboard)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"clipboard::init_paste"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("function push_event_clipboard_PASTE_TEXT(text)") != std::string::npos);
    CHECK(js.find("function push_event_clipboard_PASTE_IMAGE(image, mime)") != std::string::npos);
    CHECK(js.find("const blobs = [];") != std::string::npos);
    CHECK(js.find("_triggerDiscreteUpdate(); } });") != std::string::npos);

    markers = void_markers(defs, {"clipboard::write_text"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("navigator.clipboard.writeText") != std::string::npos);
    CHECK(js.find("push_event_clipboard_") == std::string::npos);
    CHECK(js.find("blobs") == std::string::npos);
}

// files::open and drop pull in events and blobs, save doesn't
TEST(codegen_js_files)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::add_drop_listener"});
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_files_open"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("function push_event_files_OPENED(request, data, name, mime, index, count)") != std::string::npos);
    CHECK(js.find("function push_event_files_CANCELLED(request)") != std::string::npos);
    CHECK(js.find("function push_event_dom_DROP(handle, data, name, mime, x, y, index, count)") != std::string::npos);
    CHECK(js.find("const blobs = [];") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_files_save"}, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("a.download = name") != std::string::npos);
    CHECK(js.find("push_event_files_") == std::string::npos);
    CHECK(js.find("blobs") == std::string::npos);
}

// load and from_blob both pull in LOADED/ERROR
TEST(codegen_js_image_events)
{
    SchemaDefs defs = real_defs();
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_image_load"}, {}, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("function push_event_image_LOADED(handle, width, height)") != std::string::npos);
    CHECK(js.find("function push_event_image_ERROR(handle)") != std::string::npos);
    CHECK(js.find("img.decode()") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_image_from_blob"}, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("const blobs = [];") != std::string::npos);
    CHECK(js.find("URL.revokeObjectURL(url)") != std::string::npos);
}

// Focus listener pulls in FOCUS/BLUR, which run an update right away.
TEST(codegen_js_focus_listener)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"dom::add_focus_listener", "dom::set_style"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("function push_event_dom_FOCUS(handle)") != std::string::npos);
    CHECK(js.find("function push_event_dom_BLUR(handle)") != std::string::npos);
    CHECK(js.find("push_event_dom_FOCUS(handle); _triggerDiscreteUpdate();") != std::string::npos);
    CHECK(js.find("el.style.setProperty(name, value)") != std::string::npos);

    markers = void_markers(defs, {"dom::focus"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("preventScroll") != std::string::npos);
    CHECK(js.find("push_event_dom_FOCUS") == std::string::npos);
}

// lifecycle and visibility events update right away
TEST(codegen_js_lifecycle)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"system::init_lifecycle", "system::init_visibility_change"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("function push_event_system_PAGE_HIDE(persisted)") != std::string::npos);
    CHECK(js.find("function push_event_system_PAGE_SHOW(persisted)") != std::string::npos);
    CHECK(js.find("function push_event_system_ONLINE(online)") != std::string::npos);
    CHECK(js.find("push_event_system_PAGE_HIDE(e.persisted ? 1 : 0); _triggerDiscreteUpdate();") != std::string::npos);
    CHECK(js.find("document.visibilityState || 'visible'); _triggerDiscreteUpdate();") != std::string::npos);
}

// draw_image guard locals must not shadow w/h
TEST(codegen_js_draw_image_sources)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"canvas::draw_image_scaled", "canvas::free"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("const src = elements[source];") != std::string::npos);
    CHECK(js.find("ctx.drawImage(src, x, y, w, h)") != std::string::npos);
    CHECK(js.find("const w = src") == std::string::npos);
    CHECK(js.find("contexts[i].canvas === c") != std::string::npos);
}

// helpers emitted once when used, nothing when unused
TEST(codegen_js_helpers)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"pdf::add_page", "pdf::finish"});
    generate_js_runtime(defs, {"webcc_js_flush", "webcc_pdf_create_writer"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    size_t first = js.find("const __wcc_pdf = {");
    CHECK(first != std::string::npos);
    CHECK(js.find("const __wcc_pdf = {", first + 1) == std::string::npos);
    // used only inside the helper
    CHECK(js.find("const blobs = [];") != std::string::npos);
    CHECK(js.find("function push_event_pdf_WRITTEN(") != std::string::npos);
    CHECK(js.find("function push_event_pdf_OPENED(") != std::string::npos);

    generate_js_runtime(defs, {"webcc_js_flush", "webcc_blob_take"}, {}, {}, "/tmp");
    js = read_file("/tmp/app.js");
    CHECK(js.find("__wcc_pdf") == std::string::npos);
    CHECK(js.find("push_event_pdf_") == std::string::npos);
}

// groups become enum classes, cast at the wire
TEST(codegen_groups)
{
    const char *def_path = "/tmp/webcc_test_groups.def";
    {
        std::ofstream out(def_path);
        out << "gfx|enum|Phase:uint8|DOWN MOVE UP\n"
               "gfx|flags|Mods:uint8|SHIFT=1 CTRL=2\n"
               "gfx|event|TAP|handle(Pad):h Phase:phase Mods:mods\n"
               "gfx|command|LISTEN|listen|handle(Pad):h Mods:mods=0|{ sink(mods); }\n"
               "gfx|command|STATE|state|handle(Pad):h RET:Phase|{ return 0; }\n";
    }
    SchemaDefs defs = load_defs(def_path);
    std::remove(def_path);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }
    const char *tmp = "/tmp/webcc_groups_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());
    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }
    emit_headers(defs);
    std::string header = read_file("include/webcc/gfx.h");
    std::string enums = read_file("include/webcc/core/enums.h");
    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed");
        return;
    }

    CHECK(enums.find("enum class Phase : uint8_t {") != std::string::npos);
    CHECK(enums.find("UP = 2,") != std::string::npos);
    CHECK(enums.find("enum class Mods : uint8_t {") != std::string::npos);
    CHECK(enums.find("constexpr Mods operator|(Mods a, Mods b)") != std::string::npos);
    CHECK(enums.find("constexpr bool any(Mods v") != std::string::npos);
    CHECK(enums.find("Phase operator|") == std::string::npos); // choices don't combine
    CHECK(header.find("#include \"webcc/core/enums.h\"") != std::string::npos);
    CHECK(header.find("webcc::gfx::Phase phase;") != std::string::npos);
    CHECK(header.find("res.phase = static_cast<webcc::gfx::Phase>(*(uint8_t*)(data + offset));") != std::string::npos);
    CHECK(header.find("inline void listen(webcc::Pad h, webcc::gfx::Mods mods = webcc::gfx::Mods(0))") != std::string::npos);
    CHECK(header.find("inline webcc::gfx::Phase state(webcc::Pad h)") != std::string::npos);
    CHECK(header.find("return webcc::gfx::Phase(webcc_gfx_state(") != std::string::npos);
}

// init commands are idempotent
TEST(codegen_js_init_idempotent)
{
    SchemaDefs defs = real_defs();
    auto markers = void_markers(defs, {"input::init_keyboard", "system::init_visibility_change", "system::init_popstate",
                                       "system::init_lifecycle", "clipboard::init_paste"});
    generate_js_runtime(defs, {"webcc_js_flush"}, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("if (window.__wcc_kb) break;") != std::string::npos || js.find("if (window.__wcc_kb) continue;") != std::string::npos);
    CHECK(js.find("window.__wcc_vis") != std::string::npos);
    CHECK(js.find("window.__wcc_pop") != std::string::npos);
    CHECK(js.find("window.__wcc_life") != std::string::npos);
    CHECK(js.find("window.__wcc_paste") != std::string::npos);
}

// Constants become constexpr, defaults become C++ default arguments.
TEST(codegen_consts_and_defaults)
{
    const char *def_path = "/tmp/webcc_test_consts.def";
    {
        std::ofstream out(def_path);
        out << "gfx|const|FAST|1\n"
               "gfx|const|SLOW|0x10\n"
               "gfx|command|DRAW|draw|int32:x uint8:flags=0|{ sink(x, flags); }\n"
               "gfx|command|OPEN|open|string:url string:opts=\"\" RET:int32|{ return 1; }\n";
    }
    SchemaDefs defs = load_defs(def_path);
    std::remove(def_path);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }
    const char *tmp = "/tmp/webcc_consts_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());
    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }
    emit_headers(defs);
    std::string header = read_file("include/webcc/gfx.h");
    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed - subsequent tests unsafe");
        return;
    }

    CHECK(header.find("inline constexpr int32_t FAST = 1;") != std::string::npos);
    CHECK(header.find("inline constexpr int32_t SLOW = 0x10;") != std::string::npos);
    CHECK(header.find("inline void draw(int32_t x, uint8_t flags = 0){") != std::string::npos);
    CHECK(header.find("inline int32_t open(webcc::string_view url, webcc::string_view opts = \"\"){") != std::string::npos);
    // The extern declaration has no defaults
    CHECK(header.find("webcc_gfx_open(const char* url, uint32_t url_len, const char* opts, uint32_t opts_len);") != std::string::npos);
}

// `bytes` as a void command param, a return command param, and an event field.
TEST(codegen_bytes_type)
{
    const char *def_path = "/tmp/webcc_test_bytes.def";
    {
        std::ofstream out(def_path);
        out << "net|event|PACKET|int32:id bytes:data\n"
               "net|command|WRITE|write|bytes:data|{ sink(data); }\n"
               "net|command|WRITE_NOW|write_now|bytes:data RET:int32|{ return data.length; }\n";
    }
    SchemaDefs defs = load_defs(def_path);
    std::remove(def_path);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }
    const char *tmp = "/tmp/webcc_bytes_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());
    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }
    emit_headers(defs);
    std::string header = read_file("include/webcc/net.h");
    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed - subsequent tests unsafe");
        return;
    }

    // Event field + parse
    CHECK(header.find("webcc::bytes_view data;") != std::string::npos);
    CHECK(header.find("res.data = webcc::bytes_view(data + offset, data_len);") != std::string::npos);
    // Void command: pushed like a string
    CHECK(header.find("inline void write(webcc::bytes_view data){") != std::string::npos);
    CHECK(header.find("webcc::CommandBuffer::push_string((const char*)data.data(), data.length());") != std::string::npos);
    // Return command: pointer + length
    CHECK(header.find("extern \"C\" int32_t webcc_net_write_now(const uint8_t* data, uint32_t data_len);") != std::string::npos);
    CHECK(header.find("return webcc_net_write_now(data.data(), data.length());") != std::string::npos);

    std::set<std::string> imports = {"webcc_js_flush", "webcc_net_write_now"};
    auto markers = void_markers(defs, {"net::write"});
    generate_js_runtime(defs, imports, markers, {}, "/tmp");
    std::string js = read_file("/tmp/app.js");
    CHECK(js.find("const data = u8.subarray(pos, pos + data_len); pos += data_padded;") != std::string::npos);
    CHECK(js.find("webcc_net_write_now: (data_ptr, data_len)") != std::string::npos);
}

// emit_headers() writes to hard-coded relative paths (include/webcc/...). Run it
// inside a temp working directory so it can't clobber the real headers, then
// snapshot a representative subset.
TEST(codegen_headers_snapshot)
{
    SchemaDefs defs = real_defs(); // load BEFORE chdir (path is absolute anyway)

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        ::webcc_test::record_failure("getcwd failed");
        return;
    }

    const char *tmp = "/tmp/webcc_headers_test";
    std::string mk = std::string("mkdir -p ") + tmp;
    (void)system(mk.c_str());

    if (chdir(tmp) != 0)
    {
        ::webcc_test::record_failure("chdir to temp failed");
        return;
    }

    emit_headers(defs);

    // Snapshot the type-safe handle header (inheritance-ordered) and the canvas
    // namespace header (covers opcodes, void commands, and return-value wrappers).
    std::string handles = read_file("include/webcc/core/handles.h");
    std::string canvas = read_file("include/webcc/canvas.h");

    if (chdir(cwd) != 0)
    {
        ::webcc_test::record_failure("chdir back failed - subsequent tests unsafe");
        return;
    }

    CHECK(!handles.empty());
    CHECK(!canvas.empty());
    check_snapshot("handles.h", handles);
    check_snapshot("canvas.h", canvas);
}
