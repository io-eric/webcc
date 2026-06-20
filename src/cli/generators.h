#pragma once
#include "schema.h"
#include <string>
#include <set>
#include <vector>

namespace webcc
{

    // The constant set of symbols the JS runtime always reads from the module.
    // Independent of feature usage, so the wasm can be linked before detection.
    const std::set<std::string> &required_wasm_exports();

    // Generates the C++ header files for each namespace (e.g., webcc/dom.h).
    // Also saves a binary schema cache for fast runtime loading.
    void emit_headers(const SchemaDefs &defs);

    // Generates the JavaScript 'case' block for a single command's opcode.
    void gen_js_case(const SchemaCommand &c, class CodeWriter &w);

    // Scans a JS action string to find which resource maps it uses.
    std::set<std::string> get_maps_from_action(const std::string &action);

    // Helper to check if 'text' contains 'word' as a whole identifier.
    bool contains_whole_word(const std::string &text, const std::string &word);

    // Generates the JavaScript runtime (app.js) based on used commands.
    // Both command kinds are detected from the linked module's import table:
    // return-value commands appear as `webcc_<ns>_<func>` imports in `env`
    // (`wasm_imports`); void commands appear as per-opcode marker imports in
    // module "w" (`void_markers`, field name = decimal opcode). See
    // read_wasm_imports and emit_headers.
    //
    // `inline_js_fns` holds the WEBCC_JS escape-hatch functions: the set of
    // import names from module "wjs_fn", each of the form `name(params){body}`
    // (the JS source itself). Every entry is mirrored back into app.js as a
    // matching handler. See js.h.
    void generate_js_runtime(const SchemaDefs &defs, const std::set<std::string> &wasm_imports, const std::set<std::string> &void_markers, const std::set<std::string> &inline_js_fns, const std::string &out_dir);

    // Generates the HTML scaffolding (index.html).
    // If a template file exists (index.template.html), uses it and injects the script tag.
    // Supported placeholders: {{script}} for script tag injection.
    // If no placeholder, script is injected before </body>.
    void generate_html(const std::string &out_dir, const std::string &template_path = "");

    // Compiles the C++ code to WebAssembly.
    // required_exports: set of function names that JS needs exported from WASM
    bool compile_wasm(const std::vector<std::string> &input_files, const std::string &out_dir, const std::string &cache_dir, const std::set<std::string> &required_exports);

} // namespace webcc
