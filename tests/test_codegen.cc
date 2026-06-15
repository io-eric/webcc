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
}

TEST(codegen_js_canvas_snapshot)
{
    SchemaDefs defs = real_defs();
    // Imports the linker emits for a canvas-only program (create a canvas, get a
    // 2D context, fill a rect). Return commands -> webcc_<ns>_<func>; void
    // commands -> webcc_use_<ns>_<func> markers.
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_canvas_create_canvas",
        "webcc_canvas_get_context_2d",
        "webcc_use_canvas_fill_rect",
    };
    generate_js_runtime(defs, imports, "/tmp");
    std::string js = read_file("/tmp/app.js");
    check_snapshot("app_canvas.js", js);
}

TEST(codegen_js_treeshakes_unused_modules)
{
    SchemaDefs defs = real_defs();
    // Same canvas-only program as above, expressed as its linker import set.
    std::set<std::string> imports = {
        "webcc_js_flush",
        "webcc_canvas_create_canvas",
        "webcc_canvas_get_context_2d",
        "webcc_use_canvas_fill_rect",
    };
    generate_js_runtime(defs, imports, "/tmp");
    std::string js = read_file("/tmp/app.js");

    // Canvas code IS present...
    CHECK(js.find("fillRect") != std::string::npos);
    CHECK(js.find("getContext('2d')") != std::string::npos);
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
        "webcc_use_dom_append_child",
        "webcc_use_dom_add_click_listener",
    };
    generate_js_runtime(defs, imports, "/tmp");
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
        "webcc_use_dom_set_inner_text",
        "webcc_use_dom_append_child",
    };
    generate_js_runtime(defs, imports, "/tmp");
    std::string js = read_file("/tmp/app.js");
    check_snapshot("app_dom.js", js);
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
