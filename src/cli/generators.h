#pragma once
#include "schema.h"
#include <string>
#include <set>
#include <vector>

namespace webcc
{

    // Result of JS generation, including required WASM exports
    struct JsGenResult
    {
        std::set<std::string> required_exports;
    };

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
    // Returns information about required WASM exports.
    JsGenResult generate_js_runtime(const SchemaDefs &defs, const std::string &user_code, const std::string &out_dir);

    // Generates the HTML scaffolding (index.html).
    // If a template file exists (index.template.html), uses it and injects the script tag.
    // Supported placeholders: {{script}} for script tag injection.
    // If no placeholder, script is injected before </body>.
    void generate_html(const std::string &out_dir, const std::string &template_path = "");

    // Compiles the C++ code to WebAssembly.
    // required_exports: set of function names that JS needs exported from WASM
    bool compile_wasm(const std::vector<std::string> &input_files, const std::string &out_dir, const std::string &cache_dir, const std::set<std::string> &required_exports);

} // namespace webcc
