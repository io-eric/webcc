#include "utils.h"
#include "schema.h"
#include "js_templates.h"
#include "generators.h"
#include "wasm.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>
#include <set>
#include <vector>
#include <filesystem>

int main(int argc, char **argv)
{
    std::string defs_path = "schema.def";
    std::vector<std::string> input_files;
    bool generate_headers = false;
    std::string out_dir = ".";
    std::string cache_dir_arg = "";
    std::string template_path = "";

    // Parse command-line arguments.
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--headers")
        {
            generate_headers = true;
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                defs_path = argv[++i];
            }
        }
        else if (arg == "--out" || arg == "-o")
        {
            if (i + 1 < argc)
            {
                out_dir = argv[++i];
            }
        }
        else if (arg == "--cache-dir")
        {
            if (i + 1 < argc)
            {
                cache_dir_arg = argv[++i];
            }
        }
        else if (arg == "--template" || arg == "-t")
        {
            if (i + 1 < argc)
            {
                template_path = argv[++i];
            }
        }
        else
        {
            input_files.push_back(arg);
        }
    }

    // Ensure output directory exists
    if (out_dir != ".")
    {
        mkdir(out_dir.c_str(), 0755);
    }

    // Load the command and event definitions.
    webcc::SchemaDefs defs;

    // If 'headers' command is given, generate headers and exit.
    if (generate_headers)
    {
        defs = webcc::load_defs(defs_path);
        webcc::emit_headers(defs);
        return 0;
    }

    if (input_files.empty())
    {
        std::cerr << "Usage: webcc [--defs <path>] [--out <dir> | -o <dir>] [--cache-dir <dir>] <source.cc> ... or webcc headers" << std::endl;
        return 1;
    }

    std::filesystem::path first_source(input_files[0]);
    std::string cache_dir;

    if (!cache_dir_arg.empty())
    {
        cache_dir = cache_dir_arg;
    }
    else
    {
        std::string source_dir = first_source.parent_path().string();
        if (source_dir.empty())
        {
            source_dir = ".";
        }
        cache_dir = source_dir + "/.webcc_cache";
    }

    // Load schema from binary cache (next to executable) or fall back to text parsing
    std::string exe_dir = webcc::get_executable_dir();
    std::string schema_cache_path = exe_dir + "/schema.wcc.bin";
    defs = webcc::load_defs_cached(schema_cache_path, defs_path);

    // A. READ USER CODE (all files), concatenated for the void-command scan.
    std::string user_code;
    for (const auto &path : input_files)
        user_code += webcc::read_file(path) + "\n";

    // B. COMPILE C++ TO WASM (Incremental).
    // Link first, with a constant set of exports, so the linked module's import
    // table becomes the ground-truth list of return-value commands the user's
    // code references (the compiler resolves those names; we never guess them).
    if (!webcc::compile_wasm(input_files, out_dir, cache_dir, webcc::required_wasm_exports()))
    {
        return 1;
    }

    // C. READ the linked wasm's import table (return-value command detection).
    std::string wasm_path = out_dir + "/app.wasm";
    std::set<std::string> wasm_imports;
    if (!webcc::read_wasm_imports(wasm_path, wasm_imports))
    {
        std::cerr << "[WebCC] Error: Could not read imports from " << wasm_path << std::endl;
        return 1;
    }

    // D. GENERATE JS RUNTIME (return cmds from imports; void cmds from source).
    webcc::generate_js_runtime(defs, wasm_imports, user_code, out_dir);

    // E. GENERATE HTML (Basic scaffolding).
    webcc::generate_html(out_dir, template_path);

    std::cout << "[WebCC] Success! Run 'python3 -m http.server' in " << out_dir << " to view." << std::endl;
    return 0;
}
