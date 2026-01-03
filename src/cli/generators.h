#pragma once
#include "schema.h"
#include <string>
#include <set>

namespace webcc
{

    // Generates the `webcc_schema.h` header file.
    void emit_schema_header(const SchemaDefs &defs);

    // Generates the C++ header files for each namespace (e.g., webcc/dom.h).
    void emit_headers(const SchemaDefs &defs);

    // Generates the JavaScript 'case' block for a single command's opcode.
    void gen_js_case(const SchemaCommand &c, class CodeWriter &w);

    // Scans a JS action string to find which resource maps it uses.
    std::set<std::string> get_maps_from_action(const std::string &action);

    // Helper to check if 'text' contains 'word' as a whole identifier.
    bool contains_whole_word(const std::string &text, const std::string &word);

    // Generates the JavaScript runtime (app.js) based on used commands.
    void generate_js_runtime(const SchemaDefs &defs, const std::string &user_code, const std::string &out_dir);

    // Generates the HTML scaffolding (index.html).
    void generate_html(const std::string &out_dir);

    // Compiles the C++ code to WebAssembly.
    bool compile_wasm(const std::vector<std::string> &input_files, const std::string &out_dir, const std::string &cache_dir);

} // namespace webcc
